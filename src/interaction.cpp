#include "interaction.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

#include "scene.h"

std::vector<InteractiveObject> g_InteractiveObjects;
bool g_IsHoldingObject = false;
int g_HeldObjectIndex = -1;
float g_PickDistance = 2.0f;
bool g_LeftMouseButtonPressed = false;

static const glm::vec3 ROOM_MIN = glm::vec3(-5.0f, 0.0f, -5.0f);
static const glm::vec3 ROOM_MAX = glm::vec3(5.0f, 3.0f, 5.0f);

float RaycastSceneDistance(glm::vec3 ray_origin, glm::vec3 ray_direction, glm::vec3* out_normal)
{
    float ray_min = 0.001f;
    float ray_max = 10000.0f;
    glm::vec3 normal(0.0f);

    for (int i = 0; i < 3; ++i)
    {
        if (std::abs(ray_direction[i]) > 0.000001f)
        {
            float inv_dir = 1.0f / ray_direction[i];
            float t1 = (ROOM_MIN[i] - ray_origin[i]) * inv_dir;
            float t2 = (ROOM_MAX[i] - ray_origin[i]) * inv_dir;

            float near_t = std::min(t1, t2);
            float far_t  = std::max(t1, t2);

            if (near_t > ray_min)
                ray_min = near_t;
            if (far_t < ray_max)
            {
                ray_max = far_t;
                normal = glm::vec3(0.0f);
                if (far_t == t1)
                    normal[i] = 1.0f;
                else
                    normal[i] = -1.0f;
            }
        }
    }

    if (out_normal)
        *out_normal = normal;

    return ray_max;
}

static float MaxScaleForRoom(const InteractiveObject& obj)
{
    float max_s = 1e6f;
    for (int i = 0; i < 3; ++i)
    {
        float model_extent = obj.base_neg_offset[i] + obj.base_pos_offset[i];
        if (model_extent > 0.001f)
        {
            float room_size = ROOM_MAX[i] - ROOM_MIN[i];
            max_s = std::min(max_s, room_size / model_extent);
        }
    }
    return max_s * 0.95f;
}

static float MaxScaleAtPosition(const InteractiveObject& obj, glm::vec3 pos)
{
    float max_s = 1e6f;
    for (int i = 0; i < 3; ++i)
    {
        if (obj.base_neg_offset[i] > 0.001f)
            max_s = std::min(max_s, (pos[i] - ROOM_MIN[i]) / obj.base_neg_offset[i]);
        if (obj.base_pos_offset[i] > 0.001f)
            max_s = std::min(max_s, (ROOM_MAX[i] - pos[i]) / obj.base_pos_offset[i]);
    }
    return max_s * 0.95f;
}

static void ClampObjectToRoom(InteractiveObject& obj)
{
    float max_scale = MaxScaleForRoom(obj);
    float uniform_scale = std::max({obj.scale.x, obj.scale.y, obj.scale.z});
    if (uniform_scale > max_scale)
    {
        float factor = max_scale / uniform_scale;
        obj.scale *= factor;
    }

    glm::vec3 neg = obj.base_neg_offset * obj.scale;
    glm::vec3 pos_ext = obj.base_pos_offset * obj.scale;

    for (int i = 0; i < 3; ++i)
    {
        if (obj.position[i] - neg[i] < ROOM_MIN[i])
        {
            obj.position[i] = ROOM_MIN[i] + neg[i];
            obj.velocity[i] = 0.0f;
        }
        if (obj.position[i] + pos_ext[i] > ROOM_MAX[i])
        {
            obj.position[i] = ROOM_MAX[i] - pos_ext[i];
            obj.velocity[i] = 0.0f;
        }
    }
}

static void ClampObjectToStatics(InteractiveObject& obj)
{
    glm::vec3 obj_min = obj.position - obj.base_neg_offset * obj.scale;
    glm::vec3 obj_max = obj.position + obj.base_pos_offset * obj.scale;

    for (const auto& s : g_StaticCollidables)
    {
        float ox = std::min(obj_max.x, s.max.x) - std::max(obj_min.x, s.min.x);
        float oy = std::min(obj_max.y, s.max.y) - std::max(obj_min.y, s.min.y);
        float oz = std::min(obj_max.z, s.max.z) - std::max(obj_min.z, s.min.z);

        if (ox > 0.0f && oy > 0.0f && oz > 0.0f)
        {
            if (ox <= oy && ox <= oz)
            {
                if (obj.position.x < s.min.x)
                    obj.position.x = s.min.x - obj.base_pos_offset.x * obj.scale.x;
                else
                    obj.position.x = s.max.x + obj.base_neg_offset.x * obj.scale.x;
            }
            else if (oy <= ox && oy <= oz)
            {
                if (obj.position.y < s.min.y)
                    obj.position.y = s.min.y - obj.base_pos_offset.y * obj.scale.y;
                else
                    obj.position.y = s.max.y + obj.base_neg_offset.y * obj.scale.y;
                obj.velocity.y = 0.0f;
            }
            else
            {
                if (obj.position.z < s.min.z)
                    obj.position.z = s.min.z - obj.base_pos_offset.z * obj.scale.z;
                else
                    obj.position.z = s.max.z + obj.base_neg_offset.z * obj.scale.z;
            }

            obj_min = obj.position - obj.base_neg_offset * obj.scale;
            obj_max = obj.position + obj.base_pos_offset * obj.scale;
        }
    }
}

void HandleInteraction(glm::vec3 cPos, glm::vec3 vDir)
{
    static bool prevLeftMousePressed = false;
    bool clicked = g_LeftMouseButtonPressed && !prevLeftMousePressed;
    prevLeftMousePressed = g_LeftMouseButtonPressed;

    if (clicked)
    {
        if (!g_IsHoldingObject)
        {
            for (size_t i = 0; i < g_InteractiveObjects.size(); ++i)
            {
                glm::vec3 toObject = g_InteractiveObjects[i].position - cPos;
                float projection = glm::dot(toObject, vDir);
                glm::vec3 closestPoint = cPos + vDir * projection;
                float distanceToRay = glm::distance(g_InteractiveObjects[i].position, closestPoint);

                if (distanceToRay < 0.6f && glm::length(toObject) < 6.0f)
                {
                    g_IsHoldingObject = true;
                    g_HeldObjectIndex = i;
                    g_PickDistance = glm::length(toObject);
                    InteractiveObject& obj = g_InteractiveObjects[i];
                    obj.scale_when_picked = obj.scale;
                    obj.velocity = glm::vec3(0.0f);
                    break;
                }
            }
        }
        else
        {
            InteractiveObject& obj = g_InteractiveObjects[g_HeldObjectIndex];

            glm::vec3 hit_normal(0.0f);
            float hit_distance = RaycastSceneDistance(cPos, vDir, &hit_normal);

            if (hit_distance > 0.1f)
            {
                glm::vec3 raw_pos = cPos + vDir * hit_distance;

                float ratio = hit_distance / g_PickDistance;
                float desired_scale = obj.scale_when_picked.x * ratio;

                glm::vec3 neg = obj.base_neg_offset * desired_scale;
                glm::vec3 pos_ext = obj.base_pos_offset * desired_scale;

                glm::vec3 offset(0.0f);
                for (int i = 0; i < 3; ++i)
                {
                    if (hit_normal[i] > 0.001f)
                        offset[i] = hit_normal[i] * neg[i];
                    else if (hit_normal[i] < -0.001f)
                        offset[i] = hit_normal[i] * pos_ext[i];
                }
                obj.position = raw_pos + offset;

                float max_at_pos = MaxScaleAtPosition(obj, obj.position);
                desired_scale = std::min(desired_scale, max_at_pos);
                obj.scale = glm::vec3(desired_scale);

                neg = obj.base_neg_offset * desired_scale;
                pos_ext = obj.base_pos_offset * desired_scale;
                offset = glm::vec3(0.0f);
                for (int i = 0; i < 3; ++i)
                {
                    if (hit_normal[i] > 0.001f)
                        offset[i] = hit_normal[i] * neg[i];
                    else if (hit_normal[i] < -0.001f)
                        offset[i] = hit_normal[i] * pos_ext[i];
                }
                obj.position = raw_pos + offset;

                ClampObjectToStatics(obj);
            }

            obj.velocity = glm::vec3(0.0f);
            g_IsHoldingObject = false;
            g_HeldObjectIndex = -1;
        }
    }

    if (g_IsHoldingObject && g_HeldObjectIndex != -1)
    {
        InteractiveObject& obj = g_InteractiveObjects[g_HeldObjectIndex];
        glm::vec3 proposed = cPos + vDir * g_PickDistance;
        glm::vec3 neg = obj.base_neg_offset * obj.scale;
        glm::vec3 pos_ext = obj.base_pos_offset * obj.scale;
        for (int i = 0; i < 3; ++i)
        {
            float lo = ROOM_MIN[i] + neg[i];
            float hi = ROOM_MAX[i] - pos_ext[i];
            if (proposed[i] < lo) proposed[i] = lo;
            if (proposed[i] > hi) proposed[i] = hi;
        }

        for (size_t k = 0; k < g_InteractiveObjects.size(); ++k)
        {
            if ((int)k == g_HeldObjectIndex) continue;
            const InteractiveObject& other = g_InteractiveObjects[k];
            glm::vec3 o_min = other.position - other.base_neg_offset * other.scale;
            glm::vec3 o_max = other.position + other.base_pos_offset * other.scale;
            for (int axis = 0; axis < 3; ++axis)
            {
                float p_min = proposed[axis] - neg[axis];
                float p_max = proposed[axis] + pos_ext[axis];
                if (p_max <= o_min[axis] || p_min >= o_max[axis]) continue;

                float op_x = std::min(proposed.x + pos_ext.x, o_max.x) - std::max(proposed.x - neg.x, o_min.x);
                float op_y = std::min(proposed.y + pos_ext.y, o_max.y) - std::max(proposed.y - neg.y, o_min.y);
                float op_z = std::min(proposed.z + pos_ext.z, o_max.z) - std::max(proposed.z - neg.z, o_min.z);
                if (op_x <= 0.0f || op_y <= 0.0f || op_z <= 0.0f) continue;

                if (op_x <= op_y && op_x <= op_z)
                {
                    if (proposed.x < other.position.x)
                        proposed.x = o_min.x - pos_ext.x - 0.001f;
                    else
                        proposed.x = o_max.x + neg.x + 0.001f;
                }
                else if (op_y <= op_x && op_y <= op_z)
                {
                    if (proposed.y < other.position.y)
                        proposed.y = o_min.y - pos_ext.y - 0.001f;
                    else
                        proposed.y = o_max.y + neg.y + 0.001f;
                }
                else
                {
                    if (proposed.z < other.position.z)
                        proposed.z = o_min.z - pos_ext.z - 0.001f;
                    else
                        proposed.z = o_max.z + neg.z + 0.001f;
                }
            }
        }
        obj.position = proposed;
        obj.velocity = glm::vec3(0.0f);
    }
}

void UpdateInteractiveObjects(float deltaTime)
{
    for (size_t i = 0; i < g_InteractiveObjects.size(); ++i)
    {
        if (g_IsHoldingObject && (int)i == g_HeldObjectIndex)
            continue;

        InteractiveObject& obj = g_InteractiveObjects[i];
        obj.velocity.y -= OBJECT_GRAVITY * deltaTime;
        obj.position += obj.velocity * deltaTime;
        ClampObjectToRoom(obj);
        ClampObjectToStatics(obj);
    }

    for (size_t i = 0; i < g_InteractiveObjects.size(); ++i)
    {
        if (g_IsHoldingObject && (int)i == g_HeldObjectIndex)
            continue;

        for (size_t j = i + 1; j < g_InteractiveObjects.size(); ++j)
        {
            if (g_IsHoldingObject && (int)j == g_HeldObjectIndex)
                continue;

            InteractiveObject& a = g_InteractiveObjects[i];
            InteractiveObject& b = g_InteractiveObjects[j];

            glm::vec3 a_min = a.position - a.base_neg_offset * a.scale;
            glm::vec3 a_max = a.position + a.base_pos_offset * a.scale;
            glm::vec3 b_min = b.position - b.base_neg_offset * b.scale;
            glm::vec3 b_max = b.position + b.base_pos_offset * b.scale;

            float ox = std::min(a_max.x, b_max.x) - std::max(a_min.x, b_min.x);
            float oy = std::min(a_max.y, b_max.y) - std::max(a_min.y, b_min.y);
            float oz = std::min(a_max.z, b_max.z) - std::max(a_min.z, b_min.z);

            if (ox <= 0.0f || oy <= 0.0f || oz <= 0.0f)
                continue;

            if (ox <= oy && ox <= oz)
            {
                float sign = (a.position.x < b.position.x) ? -1.0f : 1.0f;
                float push = ox * 0.5f + 0.001f;
                a.position.x += sign * push;
                b.position.x -= sign * push;
            }
            else if (oy <= ox && oy <= oz)
            {
                float sign = (a.position.y < b.position.y) ? -1.0f : 1.0f;
                float push = oy * 0.5f + 0.001f;
                a.position.y += sign * push;
                b.position.y -= sign * push;
                a.velocity.y = 0.0f;
                b.velocity.y = 0.0f;
            }
            else
            {
                float sign = (a.position.z < b.position.z) ? -1.0f : 1.0f;
                float push = oz * 0.5f + 0.001f;
                a.position.z += sign * push;
                b.position.z -= sign * push;
            }
        }
    }
}

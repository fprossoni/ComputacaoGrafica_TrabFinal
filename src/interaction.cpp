#include "interaction.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

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
    }
}

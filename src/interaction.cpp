#include "interaction.h"

#include <algorithm>
#include <glm/glm.hpp>

#include "collisions.h"
#include "scene.h"

std::vector<InteractiveObject> g_InteractiveObjects;
bool g_IsHoldingObject = false;
int g_HeldObjectIndex = -1;
float g_PickDistance = 2.0f;
bool g_LeftMouseButtonPressed = false;

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

        for (const auto& s : g_StaticCollidables)
        {
            float p_min_x = proposed.x - neg.x;
            float p_max_x = proposed.x + pos_ext.x;
            float p_min_y = proposed.y - neg.y;
            float p_max_y = proposed.y + pos_ext.y;
            float p_min_z = proposed.z - neg.z;
            float p_max_z = proposed.z + pos_ext.z;

            float ox = std::min(p_max_x, s.max.x) - std::max(p_min_x, s.min.x);
            float oy = std::min(p_max_y, s.max.y) - std::max(p_min_y, s.min.y);
            float oz = std::min(p_max_z, s.max.z) - std::max(p_min_z, s.min.z);

            if (ox > 0.0f && oy > 0.0f && oz > 0.0f)
            {
                if (ox <= oy && ox <= oz)
                {
                    if (proposed.x < s.min.x)
                        proposed.x = s.min.x - pos_ext.x - 0.001f;
                    else
                        proposed.x = s.max.x + neg.x + 0.001f;
                }
                else if (oy <= ox && oy <= oz)
                {
                    if (proposed.y < s.min.y)
                        proposed.y = s.min.y - pos_ext.y - 0.001f;
                    else
                        proposed.y = s.max.y + neg.y + 0.001f;
                }
                else
                {
                    if (proposed.z < s.min.z)
                        proposed.z = s.min.z - pos_ext.z - 0.001f;
                    else
                        proposed.z = s.max.z + neg.z + 0.001f;
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

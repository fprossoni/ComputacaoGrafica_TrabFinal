#include "interaction.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

std::vector<InteractiveObject> g_InteractiveObjects;
bool g_IsHoldingObject = false;
int g_HeldObjectIndex = -1;
float g_PickDistance = 2.0f;
bool g_LeftMouseButtonPressed = false;

float RaycastSceneDistance(glm::vec3 ray_origin, glm::vec3 ray_direction)
{
    float ray_min = 0.001f;
    float ray_max = 10000.0f;

    glm::vec3 room_min = glm::vec3(-5.0f, 0.0f, -5.0f);
    glm::vec3 room_max = glm::vec3(5.0f, 3.0f, 5.0f);

    for (int i = 0; i < 3; ++i)
    {
        if (std::abs(ray_direction[i]) > 0.000001f)
        {
            float direction = 1.0f / ray_direction[i];
            float min_wall = (room_min[i] - ray_origin[i]) * direction;
            float max_wall = (room_max[i] - ray_origin[i]) * direction;

            if (min_wall > max_wall) std::swap(min_wall, max_wall);
            if (min_wall > ray_min) ray_min = min_wall;
            if (max_wall < ray_max) ray_max = max_wall;

            if (ray_min > ray_max)
                return -1.0f;
        }
    }

    return ray_max;
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
                    g_InteractiveObjects[i].scale_when_picked = g_InteractiveObjects[i].scale;
                    break;
                }
            }
        }
        else
        {
            g_IsHoldingObject = false;
            g_HeldObjectIndex = -1;
        }
    }

    if (g_IsHoldingObject && g_HeldObjectIndex != -1)
    {
        InteractiveObject& obj = g_InteractiveObjects[g_HeldObjectIndex];
        float hit_distance = RaycastSceneDistance(cPos, vDir);

        if (hit_distance > 0.1f)
        {
            obj.position = cPos + vDir * hit_distance;
            float ratio = hit_distance / g_PickDistance;
            obj.scale = obj.scale_when_picked * ratio;
        }
        else
        {
            obj.position = cPos + vDir * g_PickDistance;
        }
    }
}

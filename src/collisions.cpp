#include "collisions.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

#include "player.h"
#include "scene.h"

const glm::vec3 ROOM_MIN = glm::vec3(-5.0f, 0.0f, -5.0f);
const glm::vec3 ROOM_MAX = glm::vec3(5.0f, 3.0f, 5.0f);

static const float WALL_MAIN_ZMIN = -0.5f;
static const float WALL_MAIN_ZMAX =  0.885f;
static const float WALL_MAIN_YMIN =  0.0f;
static const float WALL_MAIN_YMAX =  3.0f;

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

float MaxScaleForRoom(const InteractiveObject& obj)
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

float MaxScaleAtPosition(const InteractiveObject& obj, glm::vec3 pos)
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

void ClampObjectToRoom(InteractiveObject& obj)
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

void ClampObjectToStatics(InteractiveObject& obj)
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

bool InDoorOpening(float player_z, float player_feet_y, float player_head_y)
{
    if (!(player_z >= DOOR_Z_CENTER - DOOR_Z_HALF
       && player_z <= DOOR_Z_CENTER + DOOR_Z_HALF))
        return false;

    return player_feet_y >= DOOR_Y_BOTTOM && player_head_y <= DOOR_Y_TOP;
}

bool HitsWall(float player_z, float player_feet_y, float player_head_y, float r)
{
    float pz_min = player_z - r;
    float pz_max = player_z + r;

    if (pz_max > WALL_MAIN_ZMIN && pz_min < WALL_MAIN_ZMAX)
    {
        if (player_head_y > WALL_MAIN_YMIN && player_feet_y < WALL_MAIN_YMAX)
            return true;
    }

    if (InDoorOpening(player_z, player_feet_y, player_head_y))
        return false;

    return true;
}

bool IsOnSurface(float feet_y, float px, float pz, float r)
{
    if (feet_y <= 0.001f) return true;
    for (const auto& s : g_StaticCollidables)
    {
        if (std::abs(feet_y - s.max.y) < 0.01f)
        {
            float px_min = px - r, px_max = px + r;
            float pz_min = pz - r, pz_max = pz + r;
            if (px_max > s.min.x && px_min < s.max.x
             && pz_max > s.min.z && pz_min < s.max.z)
                return true;
        }
    }
    for (size_t i = 0; i < g_InteractiveObjects.size(); ++i)
    {
        if (g_IsHoldingObject && (int)i == g_HeldObjectIndex) continue;
        const InteractiveObject& obj = g_InteractiveObjects[i];
        glm::vec3 obj_min = obj.position - obj.base_neg_offset * obj.scale;
        glm::vec3 obj_max = obj.position + obj.base_pos_offset * obj.scale;
        if (std::abs(feet_y - obj_max.y) < 0.01f)
        {
            float px_min = px - r, px_max = px + r;
            float pz_min = pz - r, pz_max = pz + r;
            if (px_max > obj_min.x && px_min < obj_max.x
             && pz_max > obj_min.z && pz_min < obj_max.z)
                return true;
        }
    }
    return false;
}

bool StandingOnThisObject(float feet_y, float px, float pz, float r,
                          const glm::vec3& obj_min, const glm::vec3& obj_max)
{
    if (std::abs(feet_y - obj_max.y) > 0.01f) return false;
    float px_min = px - r, px_max = px + r;
    float pz_min = pz - r, pz_max = pz + r;
    return px_max > obj_min.x && px_min < obj_max.x
        && pz_max > obj_min.z && pz_min < obj_max.z;
}

void PushHorizontally(float& new_x, float& new_z, float r,
                      const glm::vec3& obj_min, const glm::vec3& obj_max,
                      float player_min_y, float player_max_y)
{
    float ox = std::min(new_x + r, obj_max.x) - std::max(new_x - r, obj_min.x);
    float oz = std::min(new_z + r, obj_max.z) - std::max(new_z - r, obj_min.z);

    float oy = std::min(player_max_y, obj_max.y) - std::max(player_min_y, obj_min.y);
    if (ox <= 0.0f || oz <= 0.0f || oy <= 0.0f) return;

    if (ox < oz)
    {
        if (new_x < obj_min.x) new_x = obj_min.x - r - 0.001f;
        else                   new_x = obj_max.x + r + 0.001f;
    }
    else
    {
        if (new_z < obj_min.z) new_z = obj_min.z - r - 0.001f;
        else                   new_z = obj_max.z + r + 0.001f;
    }
}

#include "player.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

#include "scene.h"
#include "interaction.h"

glm::vec4 g_CameraPos = glm::vec4(4.6f, PLAYER_EYE_HEIGHT, 4.6f, 1.0f);

float g_CameraTheta = 3.141592f;
float g_CameraPhi = 0.0f;
float g_CameraDistance = 3.5f;

bool g_W_Pressed = false;
bool g_A_Pressed = false;
bool g_S_Pressed = false;
bool g_D_Pressed = false;

bool g_UP_Pressed = false;
bool g_LEFT_Pressed = false;
bool g_DOWN_Pressed = false;
bool g_RIGHT_Pressed = false;

bool g_SPACE_Pressed = false;
bool g_SHIFT_Pressed = false;

float g_VerticalVelocity = 0.0f;

static const float WALL_MAIN_ZMIN = -0.5f;
static const float WALL_MAIN_ZMAX =  0.885f;
static const float WALL_MAIN_YMIN =  0.0f;
static const float WALL_MAIN_YMAX =  3.0f;

static bool InDoorOpening(float player_z, float player_feet_y, float player_head_y)
{
    if (!(player_z >= DOOR_Z_CENTER - DOOR_Z_HALF
       && player_z <= DOOR_Z_CENTER + DOOR_Z_HALF))
        return false;

    return player_feet_y >= DOOR_Y_BOTTOM && player_head_y <= DOOR_Y_TOP;
}

static bool HitsWall(float player_z, float player_feet_y, float player_head_y, float r)
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

static bool IsOnSurface(float feet_y, float px, float pz, float r)
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

static bool StandingOnThisObject(float feet_y, float px, float pz, float r,
                                 const glm::vec3& obj_min, const glm::vec3& obj_max)
{
    if (std::abs(feet_y - obj_max.y) > 0.01f) return false;
    float px_min = px - r, px_max = px + r;
    float pz_min = pz - r, pz_max = pz + r;
    return px_max > obj_min.x && px_min < obj_max.x
        && pz_max > obj_min.z && pz_min < obj_max.z;
}

static void PushHorizontally(float& new_x, float& new_z, float r,
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

void UpdatePlayerPosition(glm::vec4 view_vector, glm::vec4 up, float deltaTime)
{
    float speed = SPEED;

    glm::vec4 forward = glm::normalize(glm::vec4(view_vector.x, 0.0f, view_vector.z, 0.0f));
    glm::vec4 side = glm::vec4(
        up.y*forward.z - up.z*forward.y,
        up.z*forward.x - up.x*forward.z,
        up.x*forward.y - up.y*forward.x,
        0.0f
    );

    float dx = 0.0f, dz = 0.0f;

    if (g_W_Pressed)     { dx += forward.x; dz += forward.z; }
    if (g_S_Pressed)     { dx -= forward.x; dz -= forward.z; }
    if (g_A_Pressed)     { dx += side.x;    dz += side.z;    }
    if (g_D_Pressed)     { dx -= side.x;    dz -= side.z;    }

    if (g_UP_Pressed)    { dx += forward.x / WALK_SPEED;    dz += forward.z / WALK_SPEED;    }
    if (g_DOWN_Pressed)  { dx -= forward.x / WALK_SPEED;    dz -= forward.z / WALK_SPEED;    }
    if (g_LEFT_Pressed)  { dx += side.x / WALK_SPEED;       dz += side.z / WALK_SPEED;       }
    if (g_RIGHT_Pressed) { dx -= side.x / WALK_SPEED;       dz -= side.z / WALK_SPEED;       }

    float new_x = g_CameraPos.x + dx * speed * deltaTime;
    float new_z = g_CameraPos.z + dz * speed * deltaTime;
    float r = PLAYER_RADIUS;
    float cur_feet_y = g_CameraPos.y - PLAYER_EYE_HEIGHT;
    float cur_head_y = g_CameraPos.y + (PLAYER_HEIGHT - PLAYER_EYE_HEIGHT);

    if (new_z > ROOM_Z_MAX - r) new_z = ROOM_Z_MAX - r;
    if (new_z < ROOM_Z_MIN + r) new_z = ROOM_Z_MIN + r;
    if (new_x > ROOM_X_MAX - r) new_x = ROOM_X_MAX - r;

    for (const auto& s : g_StaticCollidables)
    {
        if (StandingOnThisObject(cur_feet_y, g_CameraPos.x, g_CameraPos.z, r,
                                 s.min, s.max))
            continue;
        PushHorizontally(new_x, new_z, r, s.min, s.max, cur_feet_y, cur_head_y);
    }

    for (size_t i = 0; i < g_InteractiveObjects.size(); ++i)
    {
        if (g_IsHoldingObject && (int)i == g_HeldObjectIndex) continue;
        const InteractiveObject& obj = g_InteractiveObjects[i];
        glm::vec3 obj_min = obj.position - obj.base_neg_offset * obj.scale;
        glm::vec3 obj_max = obj.position + obj.base_pos_offset * obj.scale;

        if (StandingOnThisObject(cur_feet_y, g_CameraPos.x, g_CameraPos.z, r,
                                 obj_min, obj_max))
            continue;
        PushHorizontally(new_x, new_z, r, obj_min, obj_max, cur_feet_y, cur_head_y);
    }

    if (new_x < ROOM_X_MIN + r && g_CameraPos.x >= ROOM_X_MIN)
    {
        if (HitsWall(new_z, cur_feet_y, cur_head_y, r))
            new_x = ROOM_X_MIN + r;
    }

    g_CameraPos.x = new_x;
    g_CameraPos.z = new_z;

    if (g_SPACE_Pressed && IsOnSurface(cur_feet_y, g_CameraPos.x, g_CameraPos.z, r))
        g_VerticalVelocity = JUMP_SPEED;

    g_VerticalVelocity -= GRAVITY * deltaTime;
    g_CameraPos.y += g_VerticalVelocity * deltaTime;

    for (const auto& s : g_StaticCollidables)
    {
        float px_min = g_CameraPos.x - r, px_max = g_CameraPos.x + r;
        float pz_min = g_CameraPos.z - r, pz_max = g_CameraPos.z + r;
        if (px_max <= s.min.x || px_min >= s.max.x) continue;
        if (pz_max <= s.min.z || pz_min >= s.max.z) continue;

        float feet = g_CameraPos.y - PLAYER_EYE_HEIGHT;
        float head = g_CameraPos.y + (PLAYER_HEIGHT - PLAYER_EYE_HEIGHT);
        if (feet >= s.max.y || head <= s.min.y) continue;

        if (g_VerticalVelocity <= 0.0f)
        {
            g_CameraPos.y = s.max.y + PLAYER_EYE_HEIGHT;
            g_VerticalVelocity = 0.0f;
        }
        else
        {
            g_CameraPos.y = s.min.y - (PLAYER_HEIGHT - PLAYER_EYE_HEIGHT);
            g_VerticalVelocity = 0.0f;
        }
    }

    for (size_t i = 0; i < g_InteractiveObjects.size(); ++i)
    {
        if (g_IsHoldingObject && (int)i == g_HeldObjectIndex) continue;
        const InteractiveObject& obj = g_InteractiveObjects[i];
        glm::vec3 obj_min = obj.position - obj.base_neg_offset * obj.scale;
        glm::vec3 obj_max = obj.position + obj.base_pos_offset * obj.scale;

        float px_min = g_CameraPos.x - r, px_max = g_CameraPos.x + r;
        float pz_min = g_CameraPos.z - r, pz_max = g_CameraPos.z + r;
        if (px_max <= obj_min.x || px_min >= obj_max.x) continue;
        if (pz_max <= obj_min.z || pz_min >= obj_max.z) continue;

        float feet = g_CameraPos.y - PLAYER_EYE_HEIGHT;
        float head = g_CameraPos.y + (PLAYER_HEIGHT - PLAYER_EYE_HEIGHT);
        if (feet >= obj_max.y || head <= obj_min.y) continue;

        if (g_VerticalVelocity <= 0.0f)
        {
            g_CameraPos.y = obj_max.y + PLAYER_EYE_HEIGHT;
            g_VerticalVelocity = 0.0f;
        }
        else
        {
            g_CameraPos.y = obj_min.y - (PLAYER_HEIGHT - PLAYER_EYE_HEIGHT);
            g_VerticalVelocity = 0.0f;
        }
    }

    float feet_y = g_CameraPos.y - PLAYER_EYE_HEIGHT;
    if (feet_y < ROOM_Y_MIN)
    {
        g_CameraPos.y = ROOM_Y_MIN + PLAYER_EYE_HEIGHT;
        g_VerticalVelocity = 0.0f;
    }

    float head_y = g_CameraPos.y + (PLAYER_HEIGHT - PLAYER_EYE_HEIGHT);
    if (head_y > ROOM_Y_MAX)
    {
        g_CameraPos.y = ROOM_Y_MAX - (PLAYER_HEIGHT - PLAYER_EYE_HEIGHT);
        g_VerticalVelocity = 0.0f;
    }
}

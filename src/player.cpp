#include "player.h"

#include <glm/glm.hpp>

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

// Door frame area
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

    float new_x = g_CameraPos.x + dx * speed;
    float new_z = g_CameraPos.z + dz * speed;
    float r = PLAYER_RADIUS;

    if (new_z > ROOM_Z_MAX - r) new_z = ROOM_Z_MAX - r;
    if (new_z < ROOM_Z_MIN + r) new_z = ROOM_Z_MIN + r;

    if (new_x > ROOM_X_MAX - r) new_x = ROOM_X_MAX - r;

    float feet_y = g_CameraPos.y - PLAYER_EYE_HEIGHT;
    float head_y = g_CameraPos.y + (PLAYER_HEIGHT - PLAYER_EYE_HEIGHT);

    if (new_x < ROOM_X_MIN + r)
    {
        if (HitsWall(new_z, feet_y, head_y, r))
        {
            new_x = ROOM_X_MIN + r;
        }
    }

    g_CameraPos.x = new_x;
    g_CameraPos.z = new_z;

    bool on_ground = g_CameraPos.y - PLAYER_EYE_HEIGHT <= 0.001f;

    if (g_SPACE_Pressed && on_ground)
        g_VerticalVelocity = JUMP_SPEED;

    g_VerticalVelocity -= GRAVITY * deltaTime;
    g_CameraPos.y += g_VerticalVelocity * deltaTime;

    feet_y = g_CameraPos.y - PLAYER_EYE_HEIGHT;
    if (feet_y < ROOM_Y_MIN)
    {
        g_CameraPos.y = ROOM_Y_MIN + PLAYER_EYE_HEIGHT;
        g_VerticalVelocity = 0.0f;
    }

    head_y = g_CameraPos.y + (PLAYER_HEIGHT - PLAYER_EYE_HEIGHT);
    if (head_y > ROOM_Y_MAX)
    {
        g_CameraPos.y = ROOM_Y_MAX - (PLAYER_HEIGHT - PLAYER_EYE_HEIGHT);
        g_VerticalVelocity = 0.0f;
    }
}

#ifndef PLAYER_H
#define PLAYER_H

#include <glm/vec4.hpp>

#define SPEED 0.035f
#define WALK_SPEED 50

#define PLAYER_RADIUS 0.3f
#define PLAYER_HEIGHT 0.6f
#define PLAYER_EYE_HEIGHT 0.5f

/* Gravidade e  pulo finais
#define GRAVITY 7.0f
#define JUMP_SPEED 3.0f
*/

#define GRAVITY 7.0f
#define JUMP_SPEED 3.0f

#define ROOM_X_MIN -5.0f
#define ROOM_X_MAX  5.0f
#define ROOM_Z_MIN -5.0f
#define ROOM_Z_MAX  5.0f

#define ROOM_Y_MIN 0.0f
#define ROOM_Y_MAX 3.0f

// Door opening 
#define DOOR_Z_CENTER -4.615f
#define DOOR_Z_HALF 0.4f
#define DOOR_Y_BOTTOM 1.1f
#define DOOR_Y_TOP 2.42f

extern glm::vec4 g_CameraPos;
extern float g_CameraTheta;
extern float g_CameraPhi;
extern float g_CameraDistance;

extern bool g_W_Pressed;
extern bool g_A_Pressed;
extern bool g_S_Pressed;
extern bool g_D_Pressed;

extern bool g_UP_Pressed;
extern bool g_LEFT_Pressed;
extern bool g_DOWN_Pressed;
extern bool g_RIGHT_Pressed;

extern bool g_SPACE_Pressed;
extern bool g_SHIFT_Pressed;

extern float g_VerticalVelocity;

void UpdatePlayerPosition(glm::vec4 view_vector, glm::vec4 up, float deltaTime);

#endif

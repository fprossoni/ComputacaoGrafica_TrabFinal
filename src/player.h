#ifndef PLAYER_H
#define PLAYER_H

#include <glm/vec4.hpp>

#define SPEED 0.035f
#define LENTIDAO_CAMINHADA 50
#define FLOATING_SPEED 0.01f

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

void UpdatePlayerPosition(glm::vec4 view_vector, glm::vec4 up);

#endif

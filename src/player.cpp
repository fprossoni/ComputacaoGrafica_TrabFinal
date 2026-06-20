#include "player.h"

#include <glm/glm.hpp>

// POSICAO INICIAL DO JOGADOR
glm::vec4 g_CameraPos = glm::vec4(4.6f, 0.5f, 4.6f, 1.0f);

// POSICAO centro do mundo
//glm::vec4 g_CameraPos = glm::vec4(0.0f, 0.5f, 0.0f, 1.0f);

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

void UpdatePlayerPosition(glm::vec4 view_vector, glm::vec4 up)
{
    float speed = SPEED;

    glm::vec4 forward = glm::normalize(glm::vec4(view_vector.x, 0.0f, view_vector.z, 0.0f));
    glm::vec4 side = glm::vec4(
        up.y*forward.z - up.z*forward.y,
        up.z*forward.x - up.x*forward.z,
        up.x*forward.y - up.y*forward.x,
        0.0f
    );

    if (g_W_Pressed) g_CameraPos += forward * speed;
    if (g_S_Pressed) g_CameraPos -= forward * speed;
    if (g_A_Pressed) g_CameraPos += side * speed;
    if (g_D_Pressed) g_CameraPos -= side * speed;

    if (g_UP_Pressed) g_CameraPos += forward * (speed / LENTIDAO_CAMINHADA);
    if (g_DOWN_Pressed) g_CameraPos -= forward * (speed / LENTIDAO_CAMINHADA);
    if (g_LEFT_Pressed) g_CameraPos += side * (speed / LENTIDAO_CAMINHADA);
    if (g_RIGHT_Pressed) g_CameraPos -= side * (speed / LENTIDAO_CAMINHADA);

    if (g_SPACE_Pressed) g_CameraPos.y += FLOATING_SPEED;
    if (g_SHIFT_Pressed) g_CameraPos.y -= FLOATING_SPEED;
}

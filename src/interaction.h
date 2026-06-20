#ifndef INTERACTION_H
#define INTERACTION_H

#include <string>
#include <vector>
#include <glm/vec3.hpp>

struct InteractiveObject {
    std::string scene_name; // nome do objeto ex: "rubber_duck_toy"
    int object_id; // ID do shader (numero em define, ex: RUBBER_DUCK)
    glm::vec3 position; // posicao no mundo
    glm::vec3 scale; // escala no mundo

    glm::vec3 scale_when_picked; // tamanho inicial do objeto
};

extern std::vector<InteractiveObject> g_InteractiveObjects;
extern bool g_IsHoldingObject;
extern int g_HeldObjectIndex;
extern float g_PickDistance;
extern bool g_LeftMouseButtonPressed;

float RaycastSceneDistance(glm::vec3 ray_origin, glm::vec3 ray_direction);

void HandleInteraction(glm::vec3 cPos, glm::vec3 vDir);

#endif

#ifndef INTERACTION_H
#define INTERACTION_H

#include <string>
#include <vector>
#include <glm/vec3.hpp>

#define OBJECT_GRAVITY 5.0f

struct InteractiveObject {
    std::string scene_name; // nome do objeto ex: "rubber_duck_toy"
    int object_id; // ID do shader (numero em define, ex: RUBBER_DUCK)
    glm::vec3 position; // posicao no mundo
    glm::vec3 scale; // escala no mundo
    glm::vec3 scale_when_picked; // tamanho inicial do objeto
    glm::vec3 base_neg_offset; // unscaled distance from origin to -bbox_min
    glm::vec3 base_pos_offset; // unscaled distance from origin to bbox_max
    glm::vec3 velocity;
};

extern std::vector<InteractiveObject> g_InteractiveObjects;
extern bool g_IsHoldingObject;
extern int g_HeldObjectIndex;
extern float g_PickDistance;
extern bool g_LeftMouseButtonPressed;

void HandleInteraction(glm::vec3 cPos, glm::vec3 vDir);

void UpdateInteractiveObjects(float deltaTime);

#endif

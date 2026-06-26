#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <glm/vec3.hpp>
#include "interaction.h"

extern const glm::vec3 ROOM_MIN;
extern const glm::vec3 ROOM_MAX;

float RaycastSceneDistance(glm::vec3 ray_origin, glm::vec3 ray_direction, glm::vec3* out_normal = nullptr);

float MaxScaleForRoom(const InteractiveObject& obj);
float MaxScaleAtPosition(const InteractiveObject& obj, glm::vec3 pos);

void ClampObjectToRoom(InteractiveObject& obj);
void ClampObjectToStatics(InteractiveObject& obj);

bool InDoorOpening(float player_z, float player_feet_y, float player_head_y);
bool HitsWall(float player_z, float player_feet_y, float player_head_y, float r);
bool IsOnSurface(float feet_y, float px, float pz, float r);
bool StandingOnThisObject(float feet_y, float px, float pz, float r,
                          const glm::vec3& obj_min, const glm::vec3& obj_max);
void PushHorizontally(float& new_x, float& new_z, float r,
                      const glm::vec3& obj_min, const glm::vec3& obj_max,
                      float player_min_y, float player_max_y);

#endif

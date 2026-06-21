#ifndef SCENE_H
#define SCENE_H

#include <glad/glad.h>

#include <map>
#include <stack>
#include <string>
#include <vector>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <tiny_obj_loader.h>

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true);
};

extern std::map<std::string, SceneObject> g_VirtualScene;
extern std::stack<glm::mat4> g_MatrixStack;
extern float g_ScreenRatio;

extern GLuint g_GpuProgramID;
extern GLint g_model_uniform;
extern GLint g_view_uniform;
extern GLint g_projection_uniform;
extern GLint g_object_id_uniform;
extern GLint g_bbox_min_uniform;
extern GLint g_bbox_max_uniform;

struct CollidableAABB {
    glm::vec3 min, max;
};
extern std::vector<CollidableAABB> g_StaticCollidables;

static inline CollidableAABB ComputeWorldAABB(const glm::vec3& bmin, const glm::vec3& bmax, const glm::mat4& model)
{
    CollidableAABB aabb;
    aabb.min = glm::vec3(1e9f);
    aabb.max = glm::vec3(-1e9f);
    for (int i = 0; i < 8; ++i)
    {
        glm::vec3 c(i & 1 ? bmax.x : bmin.x,
                    i & 2 ? bmax.y : bmin.y,
                    i & 4 ? bmax.z : bmin.z);
        glm::vec4 wc = model * glm::vec4(c, 1.0f);
        aabb.min = glm::min(aabb.min, glm::vec3(wc));
        aabb.max = glm::max(aabb.max, glm::vec3(wc));
    }
    return aabb;
}

// Meus objetos
#define SPHERE 0
#define BUNNY  1
#define PLANE  2
#define CHESS_WHITE_PIECE 3
#define VELVET_FLOOR 4
#define METAL_FLOOR 5
#define RUBBER_DUCK 6
#define METAL_WALL 7
#define METAL_CEILING 8
#define CEILING_FAN 9
#define METAL_WALL_2 10
#define CAT_STATUE 11
#define FIRE_EXTINGUISHER 12
#define WOOD_TABLE 13
#define ORNATE_MIRROR 14
#define SCHOOL_CHAIR 15
#define SECURITY_CAMERA 16
#define METAL_BARREL 17
#define WET_FLOOR_SIGN 18
#define WOOD_CUBE 19
#define INDUSTRIAL_LAMP 20
#define INDUSTRIAL_LAMP_COVER 21
#define CHESS_BLACK_PIECE 22
#define CARPET 23
#define DOOR_FRAME 24

#define PI 3.141592
#define PIF 3.141592f

void BuildTrianglesAndAddToVirtualScene(ObjModel* model);
void ComputeNormals(ObjModel* model);
void LoadTextureImage(const char* filename);
void DrawVirtualObject(const char* object_name);
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

#endif

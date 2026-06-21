//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Computação Gráfica e Visualização I
//               Prof. Eduardo Gastal
//
//     CÓDIGO BASE PARA O TRABALHO FINAL
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

// Headers abaixo são específicos de C++
#include <set>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"

// Headers dos módulos refatorados
#include "scene.h"
#include "player.h"
#include "interaction.h"

void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window, float lookX, float lookY, float lookZ);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

#define COR_R 0.196f
#define COR_G 0.471f
#define COR_B 0.863f
#define COR_A 0.8f

#define SENSIBILIDADE 0.001f

glm::vec3 g_LightPos   = glm::vec3(0.0f, 2.6f, 0.0f);  // posicao luz
glm::vec3 g_LightColor = glm::vec3(1.0f, 0.98f, 0.90f); // cor da luz

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor(); //define monitor primario

    const GLFWvidmode* monitor_res = glfwGetVideoMode(primaryMonitor); //pega resolucao do monitor
    GLFWwindow* window;
    window = glfwCreateWindow(monitor_res->width, monitor_res->height, "INF01047 - 587631 - Felipe Rossoni", NULL, NULL); //monitor_res->witdth comprimento do monitor monitor_res->height altura do monitor
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, monitor_res->width, monitor_res->height); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/red_brick_diff_1k.jpg");      // TextureImage0
    LoadTextureImage("../../data/rocky_terrain_02_diff_1k.jpg"); // TextureImage1
    LoadTextureImage("../../data/textures/chess_set_pieces_white_diff_1k.jpg"); // TextureImage2
    LoadTextureImage("../../data/textures/ornate_mirror_01_diff_2k.jpg"); // TextureImage3
    LoadTextureImage("../../data/textures/metal_plate_diff_2k.jpg"); // TextureImage4
    LoadTextureImage("../../data/textures/rubber_duck_toy_diff_4k.jpg"); // TextureImage5
    LoadTextureImage("../../data/textures/box_profile_metal_sheet_diff_2k.jpg"); // TextureImage6
    LoadTextureImage("../../data/textures/Poliigon_MetalPaintedMatte_7037_BaseColor.jpg"); // TextureImage7 
    LoadTextureImage("../../data/textures/ceiling_fan_diff_1k.jpg"); // TextureImage8
    LoadTextureImage("../../data/textures/concrete_cat_statue_diff_1k.jpg"); // TextureImage9
    LoadTextureImage("../../data/textures/DefaultMaterial_albedo.jpg"); // TextureImage10
    LoadTextureImage("../../data/textures/wooden_table_02_diff_2k.jpg"); // TextureImage11
    LoadTextureImage("../../data/textures/SchoolChair_01_diff_2k.jpg"); // TextureImage12
    LoadTextureImage("../../data/textures/security_camera_02_diff_1k.jpg"); // TextureImage13
    LoadTextureImage("../../data/textures/barrel_03_diff_2k.jpg"); // TextureImage14
    LoadTextureImage("../../data/textures/WetFloorSign_01_diff_2k.jpg"); // TextureImage15
    LoadTextureImage("../../data/textures/Wooden_Toy_BaseColor.tga"); // TextureImage16
    LoadTextureImage("../../data/textures/industrial_wall_lamp_diff_1k.jpg"); // TextureImage17
    LoadTextureImage("../../data/textures/industrial_wall_lamp_glass_diff_1k.jpg"); // TextureImage18
    LoadTextureImage("../../data/textures/interior_tiles_diff_1k.jpg"); // TextureImage19
    LoadTextureImage("../../data/textures/chess_set_pieces_black_diff_1k.jpg"); // TextureImage20
    LoadTextureImage("../../data/textures/_02_-_Default.jpg"); // TextureImage21
    LoadTextureImage("../../data/textures/plastered_wall_05_diff_1k.jpg"); // TextureImage22
    LoadTextureImage("../../data/textures/painted_plaster_wall_diff_1k.jpg"); // TextureImage23

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel bunnymodel("../../data/bunny.obj");
    ComputeNormals(&bunnymodel);
    BuildTrianglesAndAddToVirtualScene(&bunnymodel);

    ObjModel planemodel("../../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel chessmodel("../../data/chess_set_1k.obj");
    ComputeNormals(&chessmodel);
    BuildTrianglesAndAddToVirtualScene(&chessmodel);

    ObjModel rubber_duck("../../data/rubber_duck_toy_4k.obj");
    ComputeNormals(&rubber_duck);
    BuildTrianglesAndAddToVirtualScene(&rubber_duck);

    ObjModel ceiling_fan("../../data/ceiling_fan_1k.obj");
    ComputeNormals(&ceiling_fan);
    BuildTrianglesAndAddToVirtualScene(&ceiling_fan);

    ObjModel cat_statue("../../data/concrete_cat_statue_1k.obj");
    ComputeNormals(&cat_statue);
    BuildTrianglesAndAddToVirtualScene(&cat_statue);

    ObjModel fire_extinguisher("../../data/FireExt.obj");
    ComputeNormals(&fire_extinguisher);
    BuildTrianglesAndAddToVirtualScene(&fire_extinguisher);

    ObjModel wood_table("../../data/wooden_table_02_2k.obj");
    ComputeNormals(&wood_table);
    BuildTrianglesAndAddToVirtualScene(&wood_table);

    ObjModel ornate_mirror("../../data/ornate_mirror_01_2k.obj");
    ComputeNormals(&ornate_mirror);
    BuildTrianglesAndAddToVirtualScene(&ornate_mirror);

    ObjModel school_chair("../../data/SchoolChair_01_2k.obj");
    ComputeNormals(&school_chair);
    BuildTrianglesAndAddToVirtualScene(&school_chair);

    ObjModel security_camera("../../data/security_camera_02_1k.obj");
    ComputeNormals(&security_camera);
    BuildTrianglesAndAddToVirtualScene(&security_camera);

    ObjModel metal_barrel("../../data/barrel_03_2k.obj");
    ComputeNormals(&metal_barrel);
    BuildTrianglesAndAddToVirtualScene(&metal_barrel);

    ObjModel wet_floor_sign("../../data/WetFloorSign_01_2k.obj");
    ComputeNormals(&wet_floor_sign);
    BuildTrianglesAndAddToVirtualScene(&wet_floor_sign);

    ObjModel wood_cube("../../data/wood_cubes.obj");
    ComputeNormals(&wood_cube);
    BuildTrianglesAndAddToVirtualScene(&wood_cube);

    ObjModel industrial_lamp("../../data/industrial_wall_lamp_1k.obj");
    ComputeNormals(&industrial_lamp);
    BuildTrianglesAndAddToVirtualScene(&industrial_lamp);

    ObjModel carpet("../../data/carpet.obj");
    ComputeNormals(&carpet);
    BuildTrianglesAndAddToVirtualScene(&carpet);

    ObjModel door_frame("../../data/door_frame.obj");
    ComputeNormals(&door_frame);
    BuildTrianglesAndAddToVirtualScene(&door_frame);

    /*============================================================================================
        OBJETOS INTERATIVOS INICIAIS
    ============================================================================================*/

    // Instancias iniciais de objetos dinamicos
        InteractiveObject duck;
        duck.scene_name = "rubber_duck_toy";
        duck.object_id = RUBBER_DUCK;
        duck.position = glm::vec3(-2.2f,0.0f,1.3f);
        duck.scale = glm::vec3(1.0f, 1.0f, 1.0f);
        duck.scale_when_picked = duck.scale;
        {
            auto it = g_VirtualScene.find("rubber_duck_toy");
            if (it != g_VirtualScene.end())
            {
                const auto& bbox_min = it->second.bbox_min;
                const auto& bbox_max = it->second.bbox_max;
                duck.base_neg_offset = glm::vec3(
                    std::max(0.0f, -bbox_min.x),
                    std::max(0.0f, -bbox_min.y),
                    std::max(0.0f, -bbox_min.z));
                duck.base_pos_offset = glm::vec3(
                    std::max(0.0f,  bbox_max.x),
                    std::max(0.0f,  bbox_max.y),
                    std::max(0.0f,  bbox_max.z));
            }
        }
        duck.velocity = glm::vec3(0.0f);

        g_InteractiveObjects.push_back(duck);

    {
        InteractiveObject f;
        f.scene_name = "Text_F_.001_Text.050";
        f.object_id = WOOD_CUBE;
        f.position = glm::vec3(-3.75f, 1.21f, -4.2f);
        f.scale = glm::vec3(4.0f, 4.0f, 4.0f);
        f.scale_when_picked = f.scale;
        auto it = g_VirtualScene.find("Text_F_.001_Text.050");
        if (it != g_VirtualScene.end()) {
            const auto& bmin = it->second.bbox_min;
            const auto& bmax = it->second.bbox_max;
            f.base_neg_offset = glm::vec3(
                std::max(0.0f, -bmin.x),
                std::max(0.0f, -bmin.y),
                std::max(0.0f, -bmin.z));
            f.base_pos_offset = glm::vec3(
                std::max(0.0f,  bmax.x),
                std::max(0.0f,  bmax.y),
                std::max(0.0f,  bmax.z));
        }
        f.velocity = glm::vec3(0.0f);
        g_InteractiveObjects.push_back(f);
    }

    /*============================================================================================
        COLIDIVEIS ESTATICOS
    ============================================================================================*/

    auto add_collidable = [](const char* name, const glm::mat4& model)
    {
        auto it = g_VirtualScene.find(name);
        if (it == g_VirtualScene.end()) return;
        g_StaticCollidables.push_back(
            ComputeWorldAABB(it->second.bbox_min, it->second.bbox_max, model));
    };

    {
        glm::mat4 M;
        M = Matrix_Translate(4.0f,0.0f,4.5f)
          * Matrix_Scale(0.35f, 0.35f, 0.35f)
          * Matrix_Rotate(PI/2, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
        add_collidable("wooden_table_02", M);

        float dist = 0.3f;
        float start_yneg = -4.8f;
        for (int laco = 0; laco < 10; ++laco) {
            M = Matrix_Translate(4.8f,0.0f,start_yneg)
              * Matrix_Scale(0.4f, 0.4f, 0.4f)
              * Matrix_Rotate(-PI/2, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
            add_collidable("SchoolChair_01", M);
            start_yneg += dist;
        }

        float start_xpos = -4.8f;
        for (int laco = 0; laco < 10; ++laco) {
            M = Matrix_Translate(start_xpos,0.0f,4.8f)
              * Matrix_Scale(0.4f, 0.4f, 0.4f)
              * Matrix_Rotate(PI, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
            add_collidable("SchoolChair_01", M);
            start_xpos += dist;
        }

        // Barril de metal
        M = Matrix_Translate(2.6f,0.0f,-3.2f) 
        * Matrix_Scale(0.42f,0.42f,0.42f);
        add_collidable("barrel_03", M);
        M = Matrix_Translate(3.0f,0.0f,-3.2f) 
        * Matrix_Scale(0.42f,0.42f,0.42f) 
        * Matrix_Rotate(PI/3, glm::vec4(0.0f,1.0f,0.0f,0.0f));
        add_collidable("barrel_03", M);
        M = Matrix_Translate(2.7f,0.0f,-2.9f) 
        * Matrix_Scale(0.42f,0.42f,0.42f) 
        * Matrix_Rotate(PI/3.5, glm::vec4(0.0f,1.0f,0.0f,0.0f));
        add_collidable("barrel_03", M);
        M = Matrix_Translate(3.0f,0.0f,-2.9f) 
        * Matrix_Scale(0.42f,0.42f,0.42f) 
        * Matrix_Rotate(PI/0.5, glm::vec4(0.0f,1.0f,0.0f,0.0f));
        add_collidable("barrel_03", M);
        M = Matrix_Translate(2.9f,0.0f,-2.6f) 
        * Matrix_Scale(0.42f,0.42f,0.42f) 
        * Matrix_Rotate(PI/6.7, glm::vec4(0.0f,1.0f,0.0f,0.0f));
        add_collidable("barrel_03", M);
        M = Matrix_Translate(2.8f,0.0f,-3.5f) 
        * Matrix_Scale(0.42f,0.42f,0.42f) 
        * Matrix_Rotate(PI/0.12, glm::vec4(0.0f,1.0f,0.0f,0.0f));
        add_collidable("barrel_03", M);

        // Placa chao molhado
        M = Matrix_Translate(2.3f,0.0f,-2.9f) 
        * Matrix_Scale(0.5f,0.5f,0.5f) 
        * Matrix_Rotate(-PI/4, glm::vec4(0.0f,1.0f,0.0f,0.0f));
        add_collidable("WetFloorSign_01", M);

        // Cubos abc
        M = Matrix_Translate(-2.12f,0.243f,2.55f) 
        * Matrix_Rotate(0.2f,glm::vec4(0.0f,1.0f,0.0f,0.0f)) 
        * Matrix_Scale(1.0f,1.0f,1.0f);
        add_collidable("Text_A_.001_Cube.005", M);
        M = Matrix_Translate(-2.1f,0.0f,2.7f) 
        * Matrix_Rotate(0.2f,glm::vec4(0.0f,1.0f,0.0f,0.0f)) 
        * Matrix_Scale(1.0f,1.0f,1.0f);
        add_collidable("Text_B_.001_Text.046", M);
        M = Matrix_Translate(-2.13f,0.0f,2.4f) 
        * Matrix_Rotate(0.2f,glm::vec4(0.0f,1.0f,0.0f,0.0f)) 
        * Matrix_Scale(1.0f,1.0f,1.0f);
        add_collidable("Text_C_.001_Text.047", M);

        // Cubos cg
        M = Matrix_Translate(-3.6f,0.0f,-4.2f) 
        * Matrix_Scale(5.0f,5.0f,5.0f);
        add_collidable("Text_C_.001_Text.047", M);
        M = Matrix_Translate(-3.2f,0.0f,-4.6f) 
        * Matrix_Scale(1.0f,1.0f,1.0f) 
        * Matrix_Rotate(-PI/4,glm::vec4(0.0f,1.0f,0.0f,0.0f));
        add_collidable("Text_G_.001_Text.051", M);

        // Corridor walls (past the door at x=-5)
        g_StaticCollidables.push_back({{-8.0f, 0.0f, -5.065f},
                                       {-5.0f, 3.0f, -4.965f}}); // left wall
        g_StaticCollidables.push_back({{-8.0f, 0.0f, -4.265f},
                                       {-5.0f, 3.0f, -4.165f}}); // right wall
        g_StaticCollidables.push_back({{-8.05f, 0.0f, -5.065f},
                                       {-7.95f, 3.0f, -4.165f}}); // back wall
    }

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    
    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        GLfloat background_color[] = { COR_R, COR_G, COR_B, COR_A };
        glClearColor(background_color[0], background_color[1], background_color[2], background_color[3]); //cor do fundo = azul claro

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().
        /*
        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);
        */

        GLint lightPosLoc = glGetUniformLocation(g_GpuProgramID, "lightPos");
        GLint viewPosLoc = glGetUniformLocation(g_GpuProgramID, "viewPos");
        GLint lightColorLoc = glGetUniformLocation(g_GpuProgramID, "lightColor");

        glm::vec3 cameraPosVec3 = glm::vec3(g_CameraPos.x, g_CameraPos.y, g_CameraPos.z);

        glUniform3fv(lightPosLoc, 1, &g_LightPos[0]);
        glUniform3fv(viewPosLoc, 1, &cameraPosVec3[0]);
        glUniform3fv(lightColorLoc, 1, &g_LightColor[0]);

        glm::vec4 view_vector = glm::vec4(cos(g_CameraPhi) * sin(g_CameraTheta),
                                          sin(g_CameraPhi),
                                          cos(g_CameraPhi) * cos(g_CameraTheta),
                                          0.0f);

        glm::vec4 up = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        
        float lookX = view_vector.x + g_CameraPos.x;
        float lookY = view_vector.y + g_CameraPos.y;
        float lookZ = view_vector.z + g_CameraPos.z;

        float current_time = (float)glfwGetTime();
        static float previous_time = current_time;
        float deltaTime = current_time - previous_time;
        previous_time = current_time;

        if (deltaTime > 0.1f) deltaTime = 0.1f;

        UpdatePlayerPosition(view_vector, up, deltaTime);

        glm::mat4 view = Matrix_Camera_View(g_CameraPos, view_vector, up);

        glm::vec3 cPos = glm::vec3(g_CameraPos.x, g_CameraPos.y, g_CameraPos.z);
        glm::vec3 vDir = glm::normalize(glm::vec3(view_vector.x, view_vector.y, view_vector.z));

        HandleInteraction(cPos, vDir);
        UpdateInteractiveObjects(deltaTime);

        

        // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
        // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        
        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.


        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -25.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = PI / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));


        /*===============
        OBJETOS DO MUNDO
        =================*/
        
        /*============================================================================================
        OBJETOS INTERATIVOS
        ============================================================================================*/

        for (size_t i = 0; i < g_InteractiveObjects.size(); ++i) {
            InteractiveObject& obj = g_InteractiveObjects[i];

            model = Matrix_Translate(obj.position.x, obj.position.y, obj.position.z)
                * Matrix_Scale(obj.scale.x, obj.scale.y, obj.scale.z);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, obj.object_id);
            DrawVirtualObject(obj.scene_name.c_str()); 
        }

        /*============================================================================================
        OBJETOS DO INICIO
        ============================================================================================*/
        
        // Pecas de xadrez em cima da mesa de madeira

        //DAMA E REI BRANCOS
        model = Matrix_Translate(4.15f,0.26f,4.7f)
        * Matrix_Scale(1.0f, 1.0f, 1.0f)
        * Matrix_Rotate(PI/3, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, CHESS_WHITE_PIECE);
        DrawVirtualObject("piece_queen_white"); 
        DrawVirtualObject("piece_king_white");
        
        // REI PRETO
        model = Matrix_Translate(4.2f,0.26f,4.5f)
        * Matrix_Scale(1.0f, 1.0f, 1.0f)
        * Matrix_Rotate(PI/2, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, CHESS_BLACK_PIECE);
        DrawVirtualObject("piece_king_white");
        
        //BISPO PRETO CAIDO
        model = Matrix_Translate(3.93f,0.5f,4.4f)
        * Matrix_Scale(1.0f, 1.0f, 1.0f)
        * Matrix_Rotate(-PI/1.9, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, CHESS_BLACK_PIECE);
        DrawVirtualObject("piece_bishop_white_01");
        DrawVirtualObject("piece_bishop_filler_white_01"); 
        
        //MESA DE MADEIRA
        model = Matrix_Translate(4.0f,0.0f,4.5f) 
        * Matrix_Scale(0.35f, 0.35f, 0.35f)
        * Matrix_Rotate(PI/2, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WOOD_TABLE);
        DrawVirtualObject("wooden_table_02");
        

        /*============================================================================================
        OBJETOS DO CANTO DE SAIDA
        ============================================================================================*/
        
        //CUBO C
        model = Matrix_Translate(-3.6f,0.0f,-4.2f)
        * Matrix_Scale(5.0f, 5.0f,5.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WOOD_CUBE);
        DrawVirtualObject("Text_C_.001_Text.047");
        
        //CUBO G
        model = Matrix_Translate(-3.2f,0.0f,-4.6f)
        * Matrix_Scale(1.0f, 1.0f,1.0f)
        * Matrix_Rotate(-PI/4, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WOOD_CUBE);
        DrawVirtualObject("Text_G_.001_Text.051");

        //DOOR FRAME DE SAIDA
        model = Matrix_Translate(-4.8f,1.70f,-5.11f)
        * Matrix_Rotate(PI/2, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f))
        * Matrix_Scale(0.7f, 0.5f, 0.5f);
         glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
         glUniform1i(g_object_id_uniform, DOOR_FRAME);
         DrawVirtualObject("door_frame"); 
        

        /*============================================================================================
        LAMPADA E CAMERAS
        ============================================================================================*/

        // LAMPADA INDUSTRIAL DA SALA
        model = Matrix_Translate(0.0f,3.0f,0.0f)
        * Matrix_Scale(1.5f, 1.5f, 1.5f)
        * Matrix_Rotate(PI/2, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, INDUSTRIAL_LAMP);
        DrawVirtualObject("industrial_lamp");
        
        // COVER LAMAPADA
        model = Matrix_Translate(0.0f,3.0f,0.0f)
        * Matrix_Scale(1.5f, 1.5f, 1.5f)
        * Matrix_Rotate(PI/2, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, INDUSTRIAL_LAMP_COVER);
        DrawVirtualObject("industrial_lamp_glass");
        
        // SUPORTE DA CAMERA 1
        model = Matrix_Translate(-5.0f,2.5f,-3.2f) 
        * Matrix_Rotate(PI/2, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f))
        * Matrix_Scale(0.7f, 0.7f,0.7f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, SECURITY_CAMERA);
        DrawVirtualObject("security_camera_mount");
        
        //CAMERA E LENTE 1 OLHANDO LEVEMENTE PARA BAIXO
        model = Matrix_Translate(-5.0f,2.5f,-3.2f)
        * Matrix_Rotate(-0.2f, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f))
        * Matrix_Rotate(PI/2, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f))
        * Matrix_Scale(0.7f, 0.7f,0.7f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, SECURITY_CAMERA);
        DrawVirtualObject("security_camera");
        DrawVirtualObject("security_camera_lens");
  
        // SUPORTE DA CAMERA 2
        model = Matrix_Translate(5.0f,2.5f,3.2f)
        * Matrix_Rotate(-PI/2, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f))
        * Matrix_Scale(0.7f, 0.7f,0.7f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, SECURITY_CAMERA);
        DrawVirtualObject("security_camera_mount");
        
        //CAMERA 2 E LENTE OLHANDO LEVEMENTE PARA BAIXO
        model = Matrix_Translate(5.0f,2.5f,3.2f) 
        * Matrix_Rotate(0.2f, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f))
        * Matrix_Rotate(-PI/2, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f))
        * Matrix_Scale(0.7f, 0.7f,0.7f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, SECURITY_CAMERA);
        DrawVirtualObject("security_camera");
        DrawVirtualObject("security_camera_lens");
        
        
        /*============================================================================================
        CADEIRAS
        ============================================================================================*/

        //CADEIRAS CANTO +X  -Y
        #define NUM_CADEIRAS 10
        int laco;
        float dist = 0.3f;
        float start_yneg = -4.8f;
  
        for(laco = 0; laco < NUM_CADEIRAS; laco++){
          model = Matrix_Translate(4.8f,0.0f,start_yneg)
          * Matrix_Scale(0.4f, 0.4f, 0.4f)
          * Matrix_Rotate(-PI/2, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
          glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
          glUniform1i(g_object_id_uniform, SCHOOL_CHAIR);
          DrawVirtualObject("SchoolChair_01");
  
          start_yneg += dist;
        }
  
        //CADEIRAS CANTO -X  +Y
        float start_xpos = -4.8f;
        for(laco = 0; laco < NUM_CADEIRAS; laco++){
          model = Matrix_Translate(start_xpos,0.0f,4.8f)
          * Matrix_Scale(0.4f, 0.4f, 0.4f)
          * Matrix_Rotate(PI, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
          glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
          glUniform1i(g_object_id_uniform, SCHOOL_CHAIR);
          DrawVirtualObject("SchoolChair_01");
  
          start_xpos += dist;
        }
       

        /*============================================================================================
        TAPETE E BRINQUEDOS
        ============================================================================================*/

       // PEAO E TORRE BRANCOS DE PÉ
       model = Matrix_Translate(-1.3f,-0.10f,2.4f)
             * Matrix_Scale(6.0f, 6.0f, 6.0f)
             * Matrix_Rotate(PI/2.7, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
       glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
       glUniform1i(g_object_id_uniform, CHESS_WHITE_PIECE);
       DrawVirtualObject("piece_pawn_white_01");
       DrawVirtualObject("piece_rook_white_01");

       // CAVALO BRANCO CAIDO
        model = Matrix_Translate(-1.55f,-0.45f,2.15f)
            * Matrix_Scale(5.0f, 5.0f, 5.0f)
            * Matrix_Rotate(PI/1.8, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f))
            * Matrix_Rotate(PI/3.5, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, CHESS_WHITE_PIECE);
        DrawVirtualObject("piece_knight_white_01");
            
        // TAPETE
        model = Matrix_Translate(-1.8f,0.0f,2.0f)
            * Matrix_Scale(3.0f, 0.5f, 3.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, CARPET);
        DrawVirtualObject("carpet");
            
        //CUBO A
        model = Matrix_Translate(-2.12f,0.243f,2.55f)
            * Matrix_Rotate(0.2f, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f))
            * Matrix_Scale(1.0f, 1.0f,1.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WOOD_CUBE);
        DrawVirtualObject("Text_A_.001_Cube.005");
        
        //CUBO B
        model = Matrix_Translate(-2.1f,0.0f,2.7f)
            * Matrix_Rotate(0.2f, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f))
            * Matrix_Scale(1.0f, 1.0f,1.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WOOD_CUBE);
        DrawVirtualObject("Text_B_.001_Text.046");
    
        //CUBO C
        model = Matrix_Translate(-2.13f,0.0f,2.4f)
            * Matrix_Rotate(0.2f, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f))
            * Matrix_Scale(1.0f, 1.0f,1.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WOOD_CUBE);
        DrawVirtualObject("Text_C_.001_Text.047");
    
        //PATO DE BORRACHA
        /*
        model = Matrix_Translate(-2.2f,0.0f,1.3f)
            * Matrix_Rotate(0.3f, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f))
            * Matrix_Scale(1.0f, 1.0f,1.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, RUBBER_DUCK);
        DrawVirtualObject("rubber_duck_toy");
        */

       
       /*============================================================================================
        CANTO DE CONSTRUCAO
        ============================================================================================*/
       
       //BARRIL DE METAL 1
       model = Matrix_Translate(2.6f,0.0f,-3.2f)
       * Matrix_Scale(0.42f, 0.42f,0.42f);
       glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_BARREL);
      DrawVirtualObject("barrel_03");
      
      //BARRIL DE METAL 2
      model = Matrix_Translate(3.0f,0.0f,-3.2f)
      * Matrix_Scale(0.42f, 0.42f,0.42f)
      * Matrix_Rotate(PI/3, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
      glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_BARREL);
      DrawVirtualObject("barrel_03");
      
      //BARRIL DE METAL 3
      model = Matrix_Translate(2.7f,0.0f,-2.9f)
      * Matrix_Scale(0.42f, 0.42f,0.42f)
      * Matrix_Rotate(PI/3.5, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
      glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_BARREL);
      DrawVirtualObject("barrel_03");
      
      //BARRIL DE METAL 4
      model = Matrix_Translate(3.0f,0.0f,-2.9f)
      * Matrix_Scale(0.42f, 0.42f,0.42f)
      * Matrix_Rotate(PI/0.5, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
      glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_BARREL);
      DrawVirtualObject("barrel_03");
      
      //BARRIL DE METAL 5
      model = Matrix_Translate(2.9f,0.0f,-2.6f)
      * Matrix_Scale(0.42f, 0.42f,0.42f)
      * Matrix_Rotate(PI/6.7, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
      glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_BARREL);
      DrawVirtualObject("barrel_03");
      
      //BARRIL DE METAL 6
      model = Matrix_Translate(2.8f,0.0f,-3.5f)
      * Matrix_Scale(0.42f, 0.42f,0.42f)
      * Matrix_Rotate(PI/0.12, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
      glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_BARREL);
      DrawVirtualObject("barrel_03");
      
      // WET FLOOR SIGN
      model = Matrix_Translate(2.3f,0.0f,-2.9f)
      * Matrix_Scale(0.5f, 0.5f,0.5f)
      * Matrix_Rotate(-PI/4, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
      glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, WET_FLOOR_SIGN);
      DrawVirtualObject("WetFloorSign_01");
      
      //EXTINTOR DE INCENDIO
      model = Matrix_Translate(2.5f,0.05f,-2.5f)
      * Matrix_Scale(0.00035f, 0.00035f, 0.00035f)
      * Matrix_Rotate(PI/4, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
      * Matrix_Rotate(PI/2, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
      glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, FIRE_EXTINGUISHER);
      DrawVirtualObject("fire_extinguisher");

      
      /*============================================================================================
      PAREDES E TETO DA SALA
      ============================================================================================*/

      // Desenhamos o plano do chão
      model = Matrix_Translate(0.0f,0.0f,0.0f) 
      * Matrix_Scale(5.0f, 1.0f, 5.0f);
      glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, PLANE);
      DrawVirtualObject("the_plane");
      
      //TETO
      model = Matrix_Translate(0.0f, 3.0f, 0.0f) 
      * Matrix_Rotate(PI, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
      * Matrix_Scale(5.0f, 0.1f, 5.0f);
      glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_CEILING);
      DrawVirtualObject("the_plane");

      //PAREDE EM -Z
      model = Matrix_Translate(0.0f, 1.8f, -5.0f)
      * Matrix_Rotate(PI / 2.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
      * Matrix_Scale(5.0f, 1.0f, 1.2f);
      glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_WALL);
      DrawVirtualObject("the_plane");
    
      //PAREDE EM -Z AUXILIAR
      model = Matrix_Translate(0.0f, 0.3f, -5.0f)
      * Matrix_Rotate(PI / 2.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
      * Matrix_Scale(5.0f, 1.0f, 0.3f);
      glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_WALL_2);
      DrawVirtualObject("the_plane");
      
      //PAREDE EM +Z
      model = Matrix_Translate(0.0f, 1.8f, 5.0f)
      * Matrix_Rotate(-PI / 2.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
      * Matrix_Scale(5.0f, 1.0f, 1.2f);
      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_WALL);
      DrawVirtualObject("the_plane");
      
      //PAREDE EM +Z AUXILIAR
      model = Matrix_Translate(0.0f, 0.3f, 5.0f)
      * Matrix_Rotate(-PI / 2.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
      * Matrix_Scale(5.0f, 1.0f, 0.3f);
      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_WALL_2);
      DrawVirtualObject("the_plane");
      
      //PAREDE EM -X
      model = Matrix_Translate(-5.0f, 1.8f, 0.385f)
      * Matrix_Rotate(PI / 2.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
      * Matrix_Rotate(-PI / 2.0f, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f))
      * Matrix_Scale(4.615f, 1.0f, 1.2f);
      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_WALL);
      DrawVirtualObject("the_plane"); //2.23y cima 1.20y baixo

      //parede em -x em cima do door frame
      model = Matrix_Translate(-5.0f, 2.615f, -4.615f)
      * Matrix_Rotate(PI / 2.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
      * Matrix_Rotate(-PI / 2.0f, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f))
      * Matrix_Scale(0.385f, 1.0f, 0.385f);
      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_WALL);
      DrawVirtualObject("the_plane"); //2.23y cima 1.20y baixo

      //parede em -x em baixo do door frame
      model = Matrix_Translate(-5.0f, 0.9f, -4.615f)
      * Matrix_Rotate(PI / 2.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
      * Matrix_Rotate(-PI / 2.0f, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f))
      * Matrix_Scale(0.385f, 1.0f, 0.3f);
      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_WALL);
      DrawVirtualObject("the_plane"); //2.23y cima 1.20y baixo

      //PAREDE EM -X AUXILIAR
      model = Matrix_Translate(-5.0f, 0.3f, 0.0f)
      * Matrix_Rotate(PI / 2.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
      * Matrix_Rotate(-PI / 2.0f, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f))
      * Matrix_Scale(5.0f, 1.0f, 0.3f);
      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_WALL_2);
      DrawVirtualObject("the_plane");
      
      //PAREDE EM +X
      model = Matrix_Translate(5.0f, 1.8f, 0.0f)
      * Matrix_Rotate(-PI / 2.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
      * Matrix_Rotate(PI / 2.0f, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f))
      * Matrix_Scale(5.0f, 1.0f, 1.2f);
      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_WALL);
      DrawVirtualObject("the_plane");

      //PAREDE EM +X AUXILIAR
      model = Matrix_Translate(5.0f, 0.3f, 0.0f) 
      * Matrix_Rotate(-PI / 2.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
      * Matrix_Rotate(PI / 2.0f, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f))
      * Matrix_Scale(5.0f, 1.0f, 0.3f);
      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, METAL_WALL_2);
      DrawVirtualObject("the_plane");

      /*============================================================================================
        CORREDOR
      ============================================================================================*/

      // Chao
      model = Matrix_Translate(-6.5f, 0.0f, -4.615f)
          * Matrix_Scale(1.5f, 1.0f, 0.4f);
      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, PLASTER_WALL);
      DrawVirtualObject("the_plane");

      // Teto
      model = Matrix_Translate(-6.5f, 3.0f, -4.615f)
          * Matrix_Rotate(PI, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
          * Matrix_Scale(1.5f, 0.1f, 0.4f);
      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, PLASTER_WALL);
      DrawVirtualObject("the_plane");

      // PArede esquerda
      model = Matrix_Translate(-6.5f, 1.5f, -5.015f)
          * Matrix_Rotate(PI / 2.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
          * Matrix_Scale(1.5f, 1.0f, 1.5f);
      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, PLASTER_WALL);
      DrawVirtualObject("the_plane");

      // Parede rdireita
      model = Matrix_Translate(-6.5f, 1.5f, -4.215f)
          * Matrix_Rotate(-PI / 2.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
          * Matrix_Scale(1.5f, 1.0f, 1.5f);
      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, PLASTER_WALL);
      DrawVirtualObject("the_plane");

      // Parede de fundo
      model = Matrix_Translate(-8.0f, 1.5f, -4.615f)
          * Matrix_Rotate(PI / 2.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))
          * Matrix_Rotate(-PI / 2.0f, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f))
          * Matrix_Scale(0.4f, 1.0f, 1.5f);
      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, PLASTER_WALL);
      DrawVirtualObject("the_plane");

      // VENTILADOR
      model = Matrix_Translate(-6.5f, 3.0f, -4.615f) 
            * Matrix_Rotate((float)glfwGetTime() * 2, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f))
            * Matrix_Scale(0.5f, 0.5f, 0.5f);
      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, CEILING_FAN);
      DrawVirtualObject("ceiling_fan");
      DrawVirtualObject("ceiling_fan_blades");

      // ESTATUA DA GATO
      model = Matrix_Translate(-7.5f, 0.02f, -4.615f)
      * Matrix_Rotate(PI / 2.0f, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f))
      * Matrix_Scale(1.0f, 1.0f, 1.0f);
      glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, CAT_STATUE);
      DrawVirtualObject("concrete_cat_statue");

      //PATO AMASSADO
        model = Matrix_Translate(-7.5f, 0.0f, -4.615f)
        * Matrix_Rotate(PI / 2.0f, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f))
        * Matrix_Scale(3.3f, 0.1f, 3.3f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, RUBBER_DUCK);
        DrawVirtualObject("rubber_duck_toy"); 
      
       /*============================================================================================
        OBJETOS nao usados
        =============================================================================================
       
       //ESPELHO
       model = Matrix_Translate(1.0f,-0.5f,1.1f)
       * Matrix_Scale(0.6f, 0.6f, 0.6f);
       glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
       glUniform1i(g_object_id_uniform, ORNATE_MIRROR);
       DrawVirtualObject("ornate_mirror_01");
       ============================================================================================*/
      
    
      // Imprimimos na tela os ângulos de Euler que controlam a rotação do
      // terceiro cubo.
      TextRendering_ShowEulerAngles(window, lookX, lookY, lookZ);
      
        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage5"), 5);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage6"), 6);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage7"), 7);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage8"), 8);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage9"), 9);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage10"), 10);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage11"), 11);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage12"), 12);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage13"), 13);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage14"), 14);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage15"), 15);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage16"), 16);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage17"), 17);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage18"), 18);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage19"), 19);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage20"), 20);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage21"), 21);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage22"), 22);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage23"), 23);
    glUseProgram(0);
}



// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados 
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{    
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;
    
        // Atualizamos parâmetros da câmera com os deslocamentos
        g_CameraTheta -= SENSIBILIDADE*dx;
        g_CameraPhi   -= SENSIBILIDADE*dy;
    
        // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = PIF/2;
        float phimin = -phimax;
    
        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;
    
        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;
    
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    

    if (g_RightMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;
    
        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_ForearmAngleZ -= 0.01f*dx;
        g_ForearmAngleX += 0.01f*dy;
    
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_MiddleMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;
    
        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_TorsoPositionX += 0.01f*dx;
        g_TorsoPositionY -= 0.01f*dy;
    
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

void Correcao_KeyCallback(int key, int action, int mod);

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // =======================
    // Não modifique esta chamada! Ela é utilizada para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    Correcao_KeyCallback(key, action, mod);
    // =======================

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    //TECLAS DE MOVIMENTO WASD
    if (key == GLFW_KEY_W) g_W_Pressed = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_A) g_A_Pressed = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_S) g_S_Pressed = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_D) g_D_Pressed = (action != GLFW_RELEASE);

    //TECLAS DE MOVIMENTO SETAS
    if (key == GLFW_KEY_UP) g_UP_Pressed = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_LEFT) g_LEFT_Pressed = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_DOWN) g_DOWN_Pressed = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_RIGHT) g_RIGHT_Pressed = (action != GLFW_RELEASE);

    // TECLAS VOO
    if (key == GLFW_KEY_SPACE) g_SPACE_Pressed = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_LEFT_SHIFT) g_SHIFT_Pressed = (action != GLFW_RELEASE);

    /*================================
    FUNCOES DE TECLAS NAO UTILIZADAS
    ==================================*/
    
    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    /*
    float delta = PI / 16; // 22.5 graus, em radianos.

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Se o usuário apertar a tecla espaço, resetamos os ângulos de Euler para zero.
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        g_AngleX = 0.0f;
        g_AngleY = 0.0f;
        g_AngleZ = 0.0f;
        g_ForearmAngleX = 0.0f;
        g_ForearmAngleZ = 0.0f;
        g_TorsoPositionX = 0.0f;
        g_TorsoPositionY = 0.0f;
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }
    */

}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.

void TextRendering_ShowEulerAngles(GLFWwindow* window, float lookX, float lookY, float lookZ)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);
    char buffer[100];

    snprintf(buffer, sizeof(buffer), "GLiminal 1.0 - Player Pos: X = %.2f | Y = %.2f | Z = %.2f\n", g_CameraPos.x, g_CameraPos.y, g_CameraPos.z);
    TextRendering_PrintString(window, buffer, -1.0f + pad/10, -1.0f + 20*pad/10, 1.0f);

    /*
    snprintf(buffer, sizeof(buffer), "Camera View - X = %.2f | Y = %.2f | Z = %.2f\n", lookX, lookY, lookZ);
    TextRendering_PrintString(window, buffer, -1.0f + pad/10, -1.0f + 10*pad/10, 1.0f);
    */
   
    snprintf(buffer, sizeof(buffer), "+"); // crosshair
    TextRendering_PrintString(window, buffer, 0.0f, 0.0f, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);
    
        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :
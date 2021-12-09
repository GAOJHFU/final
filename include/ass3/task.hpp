#pragma once
#include <ass3/shader.hpp>
#include <glm/ext.hpp>
#include <ass3/util.hpp>
#include <ass3/model.hpp>
//#include <iostream>
//#include <string>
//#include <vector>
#define SHADER "../res/shaders/"
#define TEXTURE "../res/textures/"
#define MODEL "../res/objs/"
enum TaskID
{
    NONE,
    HDR_MIRROR,
    REALTIME_CUBEMAP,
    SSAO_SHADOW,
};
class Task {
public:
    TaskID id = NONE;
    chicken3421::camera_t* camera;
    const int SCR_WIDTH = 1280;
    const int SCR_HEIGHT = 720;
    const float ZOOM = 45.f;
    virtual void TaskInit() = 0;
    virtual void TaskLoop() = 0;
    virtual void TaskEnd() = 0;
    Task(TaskID id, chicken3421::camera_t* camera) : id(id), camera(camera) {};

protected:
    unsigned int cubeVAO;
    unsigned int cubeVBO;
    unsigned int quadVAO;
    unsigned int quadVBO;
    unsigned int skyboxVAO;
    unsigned int skyboxVBO;
   
    Model ourModel[4];
    void InitModels();
    void RenderCube();
    void RenderQuad();
    void RenderSkybox();

    unsigned int CreateCubemapFBO(unsigned int* texture, int width, int height);
    unsigned int CreateFBO(unsigned int* texture, int width, int height);
    unsigned int CreateDepthFBO(unsigned int* texture, int width, int height);
    unsigned int CreateGBO(unsigned int texture[], int width, int height);
};

class TaskSSAOShadow : public Task {
public:
    TaskSSAOShadow(TaskID id,  chicken3421::camera_t* camera) : Task(id, camera) {}
    void TaskInit() override;
    void TaskLoop() override;
    void TaskEnd() override;

protected:
    void RenderScene(Shader& shader);
    std::vector<glm::vec3> GenKernel();
    unsigned int GreateNoiseTex();

    glm::vec3 lightPos;
    glm::vec3 lightColor;

    Shader depthShader;
    Shader ssaoGeoShader;
    Shader ssaoLightShader;
    Shader ssaoShader;

    unsigned int marbleTexture;
    unsigned int depthTex;
    unsigned int depthFBO;
    unsigned int GBO;
    unsigned int gTex[3];
    unsigned int ssaoFBO;
    unsigned int ssaoTex;

    unsigned int noiseTex;
    std::vector<glm::vec3> kernel;

};

class TaskHDRMirror : public Task {
public:
    TaskHDRMirror(TaskID id, chicken3421::camera_t* camera) : Task(id, camera) {}
    void TaskInit() override;
    void TaskLoop() override;
    void TaskEnd() override;
protected:
    void RenderScene(Shader& shader);
private:
    glm::vec3 lightPos;
    Shader shader;
    Shader hdrShader;
    Shader basicShader;

    // HDR
    unsigned int hdrFBO;
    unsigned int colorBuffer;
    unsigned int texture;
    std::vector<glm::vec3> lightPositions;
    std::vector<glm::vec3> lightColors;

    // Reflect
    unsigned int reflectFBO;
    unsigned int reflectTex;
};

class TaskRealtimeCubeMap : public Task {
public:
    TaskRealtimeCubeMap(TaskID id, chicken3421::camera_t* camera) : Task(id, camera) {}
    void TaskInit() override;
    void TaskLoop() override;
    void TaskEnd() override;
private:
    void DrawToSide(glm::mat4& proj, glm::mat4& view, unsigned int fbo, int side);
private:
    glm::vec3 lightPos;
    Shader shader;
    Shader skyboxShader;
    Shader rotShader;

    unsigned int FBO;
    unsigned int frameTex;
    unsigned int cubemapTex;
};
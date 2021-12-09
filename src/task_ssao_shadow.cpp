#include <ass3/task.hpp>
#include <random>
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
extern bool ssaoSwitch, shadowSwitch;

void TaskSSAOShadow::TaskInit() {
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    // depth shader
    depthShader = Shader(SHADER"5_shadow_map_depth.vs", SHADER"5_shadow_map_depth.fs", SHADER"5_shadow_map_depth.gs");

    // ssao shaders
    ssaoGeoShader = Shader(SHADER"4d_ssao_geometry.vs", SHADER"4d_ssao_geometry.fs");
    ssaoLightShader = Shader(SHADER"4d_ssao.vs", SHADER"4d_ssao_lighting.fs");
    ssaoShader = Shader(SHADER"4d_ssao.vs", SHADER"4d_ssao.fs");

    marbleTexture = Utils::LoadTexture(TEXTURE"marble3.jpg");
    depthFBO = CreateDepthFBO(&depthTex, SHADOW_WIDTH, SHADOW_HEIGHT);

    GBO = CreateGBO(gTex, SCR_WIDTH, SCR_HEIGHT);
    ssaoFBO = CreateFBO(&ssaoFBO, SCR_WIDTH, SCR_HEIGHT);
    kernel = GenKernel();
    noiseTex = GreateNoiseTex();

    ssaoLightShader.use();
    ssaoLightShader.setInt("gPosition", 0);
    ssaoLightShader.setInt("gNormal", 1);
    ssaoLightShader.setInt("gAlbedo", 2);
    ssaoLightShader.setInt("ssao", 3);
    ssaoLightShader.setInt("depthMap", 4);
    ssaoShader.use();
    ssaoShader.setInt("gPosition", 0);
    ssaoShader.setInt("gNormal", 1);
    ssaoShader.setInt("texNoise", 2);
    ssaoGeoShader.use();
    ssaoGeoShader.setInt("diffTex", 0);

    lightPos = glm::vec3(0.0f, 0.0f, 0.0f);
    lightColor = glm::vec3(0.3, 0.3, 0.3);
    
    InitModels();
}

void TaskSSAOShadow::TaskLoop() {
    lightPos.z = sin(glfwGetTime() * 0.5) * 3.0;

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float nearPlane = 1.0f, farPlane = 25.0f;
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, nearPlane, farPlane);
    std::vector<glm::mat4> shadowTransforms;
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

    // pass 1. render to depth cubemap
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    depthShader.use();
    for (unsigned int i = 0; i < 6; ++i)
        depthShader.setMat4("shadowMat[" + std::to_string(i) + "]", shadowTransforms[i]);
    depthShader.setFloat("farPlane", farPlane);
    depthShader.setVec3("lightPos", lightPos);
    RenderScene(depthShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pass 2. geometry pass, render to gbuffer
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, GBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 projection = glm::perspective(glm::radians(ZOOM), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = chicken3421::get_view(*camera);
    ssaoGeoShader.use();
    ssaoGeoShader.setMat4("projection", projection);
    ssaoGeoShader.setMat4("view", view);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, marbleTexture);  // position
    RenderScene(ssaoGeoShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pass 3. generate SSAO texture
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    ssaoShader.use();
    // Send kernel + rotation 
    for (unsigned int i = 0; i < 64; ++i)
        ssaoShader.setVec3("samples[" + std::to_string(i) + "]", kernel[i]);
    ssaoShader.setMat4("projection", projection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTex[0]);  // position
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gTex[1]);  // normal
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTex);
    RenderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pass 4. lighting pass, lighting with added shadow map and ssao
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ssaoLightShader.use();
    ssaoLightShader.setVec3("light.Position", lightPos);
    ssaoLightShader.setVec3("light.Color", lightColor);
    const float linear = 0.0f;//0.09;
    const float quadratic = 0.0f;//0.032;
    ssaoLightShader.setFloat("light.Linear", linear);
    ssaoLightShader.setFloat("light.Quadratic", quadratic);
    ssaoLightShader.setFloat("farPlane", farPlane);
    ssaoLightShader.setVec3("viewPos", camera->pos);
    ssaoLightShader.setInt("ssaoSwitch", ssaoSwitch);
    ssaoLightShader.setInt("shadowSwitch", shadowSwitch);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTex[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gTex[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gTex[2]);
    glActiveTexture(GL_TEXTURE3); // add extra SSAO texture to lighting pass
    glBindTexture(GL_TEXTURE_2D, ssaoTex);
    glActiveTexture(GL_TEXTURE4); // add depth texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthTex);
    RenderQuad();
}

void TaskSSAOShadow::TaskEnd() {
    glDeleteFramebuffers(1, &depthFBO);
    glDeleteFramebuffers(1, &GBO);
    glDeleteFramebuffers(1, &ssaoFBO);

    glDeleteTextures(1, &marbleTexture);
    glDeleteTextures(1, &ssaoTex);
    glDeleteTextures(1, &noiseTex);
    glDeleteTextures(1, &depthTex);
    for (int i = 0; i < 3; ++i) {
        glDeleteTextures(1, &gTex[i]);
    }
}


void TaskSSAOShadow::RenderScene(Shader& shader)
{
    // room cube
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(10.0f));
    shader.setMat4("model", model);
    //glDisable(GL_CULL_FACE);
    shader.setInt("reverse_normals", 1);
    RenderCube();
    shader.setInt("reverse_normals", 0);
    //glEnable(GL_CULL_FACE);
    // cubes
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(4.0f, -3.5f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.setMat4("model", model);
    RenderCube();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(4.0f, 6.0f, 2.0));
    shader.setMat4("model", model);
    ourModel[0].Draw(shader);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-6.0f, -2.0f, 0.0));
    float angle = 90.0f * sin(glfwGetTime());
    model = glm::rotate(model, glm::radians(angle), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    shader.setMat4("model", model);
    ourModel[1].Draw(shader);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-3.0f, 1.0f, 3.0));
    shader.setMat4("model", model);
    ourModel[2].Draw(shader);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.5f, 2.0f, -3.0));
    angle = 360.0f * sin(glfwGetTime());
    model = glm::rotate(model, glm::radians(angle), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    shader.setMat4("model", model);
    ourModel[3].Draw(shader);
}

std::vector<glm::vec3> TaskSSAOShadow::GenKernel() {
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    std::vector<glm::vec3> kernel;
    for (unsigned int i = 0; i < 64; ++i)
    {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / 64.0;

        scale = 0.1f + scale*scale*(1.0f - 0.1f);  //a + f * (b - a)
        sample *= scale;
        kernel.push_back(sample);
    }
    return kernel;
}

unsigned int TaskSSAOShadow::GreateNoiseTex() {
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);
        ssaoNoise.push_back(noise);
    }
    unsigned int noiseTex; 
    glGenTextures(1, &noiseTex);
    glBindTexture(GL_TEXTURE_2D, noiseTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return noiseTex;
}

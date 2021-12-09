#include <ass3/task.hpp>

extern bool hdrSwitch;
void TaskHDRMirror::TaskInit() {
    glEnable(GL_DEPTH_TEST);

    shader = Shader(SHADER"2b_hdr_lighting.vs", SHADER"2b_hdr_lighting.fs");
    hdrShader = Shader(SHADER"2b_hdr.vs", SHADER"2b_hdr.fs");
    basicShader = Shader(SHADER"basic.vs", SHADER"basic.fs");

    texture = Utils::LoadTexture(TEXTURE"brick.jpg"); 
    // HDR framebuffer
    hdrFBO = CreateFBO(&colorBuffer, SCR_WIDTH, SCR_HEIGHT);

    // Reflect framebuffer
    reflectFBO = CreateFBO(&reflectTex, SCR_WIDTH, SCR_HEIGHT);

    shader.use();
    shader.setInt("diffuseTex", 0);
    hdrShader.use();
    hdrShader.setInt("hdrBuffer", 0);

    // lighting info
    lightPositions.push_back(glm::vec3(0.0f, 0.0f, 27.0f));
    lightPositions.push_back(glm::vec3(0.0f, 0.0f, -27.0f));
    // colors
    lightColors.push_back(glm::vec3(200.0f, 200.0f, 200.0f));
    lightColors.push_back(glm::vec3(200.0f, 200.0f, 200.0f));

    InitModels();
}

void TaskHDRMirror::TaskLoop() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // pass 1. render scene into floating point framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 projection = glm::perspective(glm::radians(ZOOM), (GLfloat)SCR_WIDTH / (GLfloat)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = chicken3421::get_view(*camera);
    shader.use();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    shader.setVec3("viewPos", camera->pos);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    RenderScene(shader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // pass 2. render to mirror quad
    glBindFramebuffer(GL_FRAMEBUFFER, reflectFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RenderScene(shader);

    glm::mat4 model = glm::mat4(1.0f);
    //model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(10.0f, 0.0f, 0.0));
    model = glm::scale(model, glm::vec3(5.5f, 5.5f, 0.0f));
    basicShader.use();
    basicShader.setMat4("projection", projection);
    basicShader.setMat4("view", view);
    basicShader.setMat4("model", model);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    RenderQuad();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-10.0f, 0.0f, 0.0));
    model = glm::scale(model, glm::vec3(5.5f, 5.5f, 0.0f));
    basicShader.setMat4("model", model);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    RenderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pass 3. render to screen with HDR
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    hdrShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, reflectTex);
    hdrShader.setInt("hdr", hdrSwitch);
    hdrShader.setFloat("exposure", 1.0f);
    RenderQuad();
}

void TaskHDRMirror::TaskEnd() {
    glDeleteFramebuffers(1, &hdrFBO);
    glDeleteFramebuffers(1, &reflectFBO);
    glDeleteTextures(1, &colorBuffer);
    glDeleteTextures(1, &texture);
    glDeleteTextures(1, &reflectTex);
}

void TaskHDRMirror::RenderScene(Shader& shader)
{
    for (unsigned int i = 0; i < lightPositions.size(); i++)
    {
        shader.setVec3("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
        shader.setVec3("lights[" + std::to_string(i) + "].Color", lightColors[i]);
    }
    glm::mat4 model = glm::mat4(1.0f);
    //model = glm::translate(model, glm::vec3(0.0f, 0.0f, -25.0));
    model = glm::scale(model, glm::vec3(2.5f, 2.5f, 27.5f));
    shader.setMat4("model", model);
    shader.setInt("inverse_normals", 1);
    RenderCube();
    shader.setInt("inverse_normals", 0);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 6.0f, -2.0));
    model = glm::scale(model, glm::vec3(3.0, 3.0, 3.0));
    shader.setMat4("model", model);
    ourModel[0].Draw(shader);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-6.0f, -3.0f, 0.0));
    float angle = 90.0f * sin(glfwGetTime());
    model = glm::rotate(model, glm::radians(angle), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model = glm::scale(model, glm::vec3(3.0, 3.0, 3.0));
    shader.setMat4("model", model);
    ourModel[1].Draw(shader);
}
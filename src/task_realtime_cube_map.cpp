#include <ass3/task.hpp>

void TaskRealtimeCubeMap::TaskInit() {
    glEnable(GL_DEPTH_TEST);

    shader = Shader(SHADER"3b_realtime_cubemaps.vs", SHADER"3b_realtime_cubemaps.fs");
    skyboxShader = Shader(SHADER"3b_realtime_cubemaps_skybox.vs", SHADER"3b_realtime_cubemaps_skybox.fs");
    rotShader = Shader(SHADER"3b_realtime_cubemaps_rotation.vs", SHADER"3b_realtime_cubemaps_rotation.fs");

    std::vector<std::string> faces;
    faces.push_back(TEXTURE"skybox/right.png");
    faces.push_back(TEXTURE"skybox/left.png");
    faces.push_back(TEXTURE"skybox/top.png");
    faces.push_back(TEXTURE"skybox/bottom.png");
    faces.push_back(TEXTURE"skybox/back.png");
    faces.push_back(TEXTURE"skybox/front.png");
    FBO = CreateCubemapFBO(&frameTex, 2048, 2048);
   
    cubemapTex = Utils::LoadCubemap(faces);

    shader.use();
    shader.setInt("skybox", 0);
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    lightPos = glm::vec3(-250.0f, 250.0f, -500.0f);
}

void TaskRealtimeCubeMap::TaskLoop() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = chicken3421::get_view(*camera);
    glm::mat4 projection = glm::perspective(glm::radians(ZOOM), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

    // pass 1: Render scene into framebuffer to create cubemap
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glViewport(0, 0, 2048, 2048);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (int i = 0; i < 6; i++) {
        glm::mat4 proj, vw;
        DrawToSide(proj, vw, FBO, i);
        // draw moving cube
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
        rotShader.use();
        rotShader.setMat4("model", model);
        rotShader.setMat4("view", vw);
        rotShader.setMat4("projection", proj);
        rotShader.setVec3("lightPos", lightPos);
        rotShader.setVec3("cPos", camera->pos);
        rotShader.setFloat("time", glfwGetTime() * 2.0f);
        // rotation cube
        RenderCube();

        // draw skybox
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        glm::mat4 svw = glm::mat4(glm::mat3(vw)); // remove translation from the view matrix
        skyboxShader.setMat4("view", svw);
        skyboxShader.setMat4("projection", proj);
        // skybox cube
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);

        RenderSkybox();
        glDepthFunc(GL_LESS);       
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // pass 2: render normal scene
    shader.use();
    model = glm::mat4(1.0f);
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setVec3("cameraPos", camera->pos);
    // environment cube
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, frameTex);
    RenderCube();
    // rotation cube
    rotShader.use();
    //glm::mat4 m = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
    rotShader.setMat4("model", model);
    rotShader.setMat4("view", view);
    rotShader.setMat4("projection", projection);
    rotShader.setVec3("lightPos", lightPos);
    rotShader.setVec3("cPos", camera->pos);
    rotShader.setFloat("time", glfwGetTime() * 2.0f);
    RenderCube();
    
    // draw skybox as last
    glDepthFunc(GL_LEQUAL);
    skyboxShader.use();
    view = glm::mat4(glm::mat3(chicken3421::get_view(*camera)));
    skyboxShader.setMat4("view", view);
    skyboxShader.setMat4("projection", projection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);
    RenderSkybox();
    glDepthFunc(GL_LESS);
}

void TaskRealtimeCubeMap::TaskEnd() {
    glDeleteFramebuffers(1, &FBO);
    glDeleteTextures(1, &frameTex);
    glDeleteTextures(1, &cubemapTex);
}


void TaskRealtimeCubeMap::DrawToSide(glm::mat4& proj, glm::mat4& view, unsigned int fbo, int side) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, fbo, 0);

    //Reset perspective and view matrices
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //90 degree POV gives us a square viewport for the cubeface
    proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

    //up vectors are flipped to get correct image orientation
    switch (GL_TEXTURE_CUBE_MAP_POSITIVE_X + side) {
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        view = glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(20, 0, 0), glm::vec3(0, -1, 0));
        break;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        view = glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(-20, 0, 0), glm::vec3(0, -1, 0));
        break;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        view = glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, 20, 0), glm::vec3(0, 0, 1));
        break;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        view = glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, -20, 0), glm::vec3(0, 0, -1));
        break;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        view = glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, 0, 20), glm::vec3(0, -1, 0));
        break;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        view = glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, 0, -10), glm::vec3(0, -1, 0));
        break;
    default:
        break;
    }
}
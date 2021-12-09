#include <thread>
#include <chrono>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <chicken3421/chicken3421.hpp>
#include <ass3/shader.hpp>
#include <ass3/memes.hpp>
#include <ass3/task.hpp>

/**
 * Returns the difference in time between when this function was previously called and this call.
 * @return A float representing the difference between function calls in seconds.
 */
float time_delta();

/**
 * Returns the current time in seconds.
 * @return Returns the current time in seconds.
 */
float time_now();

/**
 * handle input function
 */
//void handle_input(GLFWwindow* window);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// Screen value
const int SCR_WIDTH = 1280;
const int SCR_HEIGHT = 720;

// Default camera values
const float ZOOM = 45.0f;

int curTask = 0;
const int totalTask = 3;

extern bool hdrSwitch = true;
extern bool ssaoSwitch = true;
extern bool shadowSwitch = true;

int main() {
    GLFWwindow *win = marcify(chicken3421::make_opengl_window(SCR_WIDTH, SCR_HEIGHT, "Assignment 3"));
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);

    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // camera
    chicken3421::camera_t camera = chicken3421::make_camera(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0, 0.0, 0.0));

    //TaskBlinnPhong task(BLINN_PHONG, &camera);

    Task* task[totalTask];
    task[0] = new TaskRealtimeCubeMap(REALTIME_CUBEMAP, &camera);
    task[1] = new TaskHDRMirror(HDR_MIRROR, &camera);
    task[2] = new TaskSSAOShadow(SSAO_SHADOW, &camera);

    for (int i = 0; i < totalTask; ++i) {
        task[i]->TaskInit();
    }
    while (!glfwWindowShouldClose(win)) {
        float dt = time_delta();
        //handle_input(win);
        glfwSetKeyCallback(win, key_callback);


        task[curTask]->TaskLoop();


        chicken3421::update_camera(camera, win, dt);
        glfwSwapBuffers(win);
        glfwPollEvents();

        // not entirely correct as a frame limiter, but close enough
        // it would be more correct if we knew how much time this frame took to render
        // and calculated the distance to the next "ideal" time to render and only slept that long
        // the current way just always sleeps for 16.67ms, so in theory we'd drop frames
        std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(1000.f / 60));
    }
    
    // ------------------------------------------------------------------------
    
    for (int i = 0; i < totalTask; ++i) {
        task[i]->TaskEnd();
    }

    for (int i = 0; i < totalTask; ++i) {
        delete task[i];
    }

    chicken3421::delete_camera(camera);
    // deleting the whole window also removes the opengl context, freeing all our memory in one fell swoop.
    chicken3421::delete_opengl_window(win);

    return EXIT_SUCCESS;
}


float time_delta() {
    static float then = time_now();
    float now = time_now();
    float dt = now - then;
    then = now;
    return dt;
}

float time_now() {
    return (float) glfwGetTime();
}


//void handle_input(GLFWwindow* window)
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    //if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    //    glfwSetWindowShouldClose(window, true);
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
        if (curTask == totalTask - 1) {
            curTask = 0;
        }
        else {
            curTask = curTask + 1;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        hdrSwitch = !hdrSwitch;
    }

    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
        ssaoSwitch = !ssaoSwitch;
    }

    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        shadowSwitch = !shadowSwitch;
    }
    
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
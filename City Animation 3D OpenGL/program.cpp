#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "stb_image.h"
#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "Bezier.h"

void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// cameras
Camera* stationaryCamera;
Camera* followingCamera;
Camera* fppCamera;
Camera* freeCamera;

Camera* activeCamera;
bool freeCameraActive;

// timing
float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

// mouse
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// lighting
Shader* PhongShaderProgram;
Shader* GouraudShaderProgram;
Shader* FlatShaderProgram;
Shader* shaderProgram;
int isDay = 1;

// reflectors
float deltaRefPos = 0.0f;

// fog
int isFogEnabled = 0;

int main()
{
    // instantiate the GLFW window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // create a window object
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "City Animation 3D", glfwGetPrimaryMonitor(), NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // enable capturing the mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    PhongShaderProgram = new Shader("Shaders/PhongShader.vs.glsl", "Shaders/PhongShader.fs.glsl");
    GouraudShaderProgram = new Shader("Shaders/GouraudShader.vs.glsl", "Shaders/GouraudShader.fs.glsl");
    FlatShaderProgram = new Shader("Shaders/flatShader.vs.glsl", "Shaders/flatShader.fs.glsl");

    shaderProgram = PhongShaderProgram;
    Shader lightShaderProgram("Shaders/lightShader.vs.glsl", "Shaders/lightShader.fs.glsl");

    // initialize cameras
    glm::vec3 startCameraPosition = glm::vec3(0.0f, 2.0f, 6.0f);
    glm::vec3 startCameraTarget = glm::vec3(0.0f, 0.0f, 3.0f);

    stationaryCamera = new Camera(startCameraPosition);
    followingCamera = new Camera(startCameraPosition);
    fppCamera = new Camera(startCameraPosition);
    freeCamera = new Camera(startCameraPosition);

    activeCamera = freeCamera;
    freeCameraActive = true;

    stationaryCamera->UpdateFront(glm::normalize(startCameraTarget - startCameraPosition));

    // load models
    Model cityModel("Resources/City/city.obj");
    Model carModel("Resources/Car/car.obj");
    Model lanternModel("Resources/Lantern/Lantern.obj");
    Model spotlightModel("Resources/Spotlight/spotlight.obj");

    // initialize Bezier surface
    BezierSurface bezierSurface = BezierSurface();

    float controlPoints[] = {
        0.0f, 0.5f, 1.0f, 0.5f,
        0.5f, 1.0f, 1.0f, 0.5f,
        0.0f, 0.5f, 1.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.0f
    };

    bezierSurface.SetBaseZValues(controlPoints);

    const int accuracy = 10;
    const int length = accuracy * accuracy * 6 * 8;
    float* bezierVertices = new float[length];
    bezierSurface.FillArrayWithValues(bezierVertices, length, accuracy);

    unsigned int bezierVBO, bezierVAO;
    glGenVertexArrays(1, &bezierVAO);
    glGenBuffers(1, &bezierVBO);

    glBindVertexArray(bezierVAO);

    glBindBuffer(GL_ARRAY_BUFFER, bezierVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bezierVertices), bezierVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //// set up vertices
    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    // configure light object
    unsigned int VBO, lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(lightVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // configure shader
    shaderProgram->use();
    shaderProgram->setInt("material.texture_diffuse", 0);
    shaderProgram->setInt("material.texture_specular", 1);

    // define initial light positions and directions
    glm::vec3 pointlightPosition = glm::vec3(-5.0f, 0.2f, 3.0f);

    glm::vec3 spotlightPosition = glm::vec3(-3.5f, 0.16f, 7.0f);
    glm::vec3 spotlightTarget = glm::vec3(0.0f, 0.05f, 1.0f);

    glm::vec3 reflector1InitialPosition = startCameraTarget + glm::vec3(-0.4f, -0.94f, -2.2f);
    glm::vec3 reflector2InitialPosition = startCameraTarget + glm::vec3(0.4f, -0.94f, -2.2f);

    glm::vec3 reflector1Position = reflector1InitialPosition;
    glm::vec3 reflector2Position = reflector2InitialPosition;

    glm::vec3 reflector1Target = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 reflector2Target = glm::vec3(0.0f, 0.0f, 1.0f);

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        // calculate frame time
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // clear color and depth buffers
        if (isDay)
            glClearColor(0.529f, 0.808f, 0.922f, 1.0f);
        else
            glClearColor(0.128f, 0.171f, 0.700f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // activate main shader and set uniforms
        shaderProgram->use();

        shaderProgram->setInt("isDay", isDay);
        shaderProgram->setVec3("viewPos", activeCamera->Position);
        shaderProgram->setFloat("material.shininess", 32.0f);

        // directional light (sun)
        shaderProgram->setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        shaderProgram->setVec3("dirLight.ambient", 0.4f, 0.4f, 0.4f);
        shaderProgram->setVec3("dirLight.diffuse", 0.8f, 0.8f, 0.8f);
        shaderProgram->setVec3("dirLight.specular", 0.8f, 0.8f, 0.8f);

        // pointlight
        shaderProgram->setVec3("pointLight.position", pointlightPosition);
        shaderProgram->setVec3("pointLight.ambient", 0.05f, 0.05f, 0.05f);
        shaderProgram->setVec3("pointLight.diffuse", 0.8f, 0.8f, 0.8f);
        shaderProgram->setVec3("pointLight.specular", 1.0f, 1.0f, 1.0f);
        shaderProgram->setFloat("pointLight.constant", 1.0f);
        shaderProgram->setFloat("pointLight.linear", 0.09f);
        shaderProgram->setFloat("pointLight.quadratic", 0.032f);

        // spotlight
        shaderProgram->setVec3("spotLights[0].position", spotlightPosition);
        shaderProgram->setVec3("spotLights[0].direction", spotlightTarget);
        shaderProgram->setVec3("spotLights[0].ambient", 0.0f, 0.0f, 0.0f);
        shaderProgram->setVec3("spotLights[0].diffuse", 1.0f, 1.0f, 1.0f);
        shaderProgram->setVec3("spotLights[0].specular", 1.0f, 1.0f, 1.0f);
        shaderProgram->setFloat("spotLights[0].constant", 1.0f);
        shaderProgram->setFloat("spotLights[0].linear", 0.09f);
        shaderProgram->setFloat("spotLights[0].quadratic", 0.032f);
        shaderProgram->setFloat("spotLights[0].cutOff", glm::cos(glm::radians(12.5f)));
        shaderProgram->setFloat("spotLights[0].outerCutOff", glm::cos(glm::radians(15.0f)));

        // car reflector 1
        shaderProgram->setVec3("spotLights[1].position", reflector1Position);
        shaderProgram->setVec3("spotLights[1].direction", reflector1Target);
        shaderProgram->setVec3("spotLights[1].ambient", 0.0f, 0.0f, 0.0f);
        shaderProgram->setVec3("spotLights[1].diffuse", 1.0f, 1.0f, 1.0f);
        shaderProgram->setVec3("spotLights[1].specular", 1.0f, 1.0f, 1.0f);
        shaderProgram->setFloat("spotLights[1].constant", 1.0f);
        shaderProgram->setFloat("spotLights[1].linear", 0.09f);
        shaderProgram->setFloat("spotLights[1].quadratic", 0.032f);
        shaderProgram->setFloat("spotLights[1].cutOff", glm::cos(glm::radians(12.5f)));
        shaderProgram->setFloat("spotLights[1].outerCutOff", glm::cos(glm::radians(15.0f)));

        // car reflector 2
        shaderProgram->setVec3("spotLights[2].position", reflector2Position);
        shaderProgram->setVec3("spotLights[2].direction", reflector2Target);
        shaderProgram->setVec3("spotLights[2].ambient", 0.0f, 0.0f, 0.0f);
        shaderProgram->setVec3("spotLights[2].diffuse", 1.0f, 1.0f, 1.0f);
        shaderProgram->setVec3("spotLights[2].specular", 1.0f, 1.0f, 1.0f);
        shaderProgram->setFloat("spotLights[2].constant", 1.0f);
        shaderProgram->setFloat("spotLights[2].linear", 0.09f);
        shaderProgram->setFloat("spotLights[2].quadratic", 0.032f);
        shaderProgram->setFloat("spotLights[2].cutOff", glm::cos(glm::radians(12.5f)));
        shaderProgram->setFloat("spotLights[2].outerCutOff", glm::cos(glm::radians(15.0f)));

        // fog parameters
        shaderProgram->setVec3("fogParams.color", 0.75f, 0.75f, 0.75f);
        shaderProgram->setFloat("fogParams.linearStart", 50.0f);
        shaderProgram->setFloat("fogParams.linearEnd", 100.0f);
        shaderProgram->setFloat("fogParams.density", 0.25f);
        shaderProgram->setInt("fogParams.equation", 2);
        shaderProgram->setInt("fogParams.isEnabled", isFogEnabled);

        // create projection matrix
        glm::mat4 projection = glm::perspective(glm::radians(activeCamera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        shaderProgram->setMat4("projection", projection);

        // create view matrix
        glm::mat4 view = activeCamera->GetViewMatrix();
        shaderProgram->setMat4("view", view);


        // render the city model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -5.0f));
        model = glm::scale(model, glm::vec3(0.001f, 0.001f, 0.001f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        shaderProgram->setMat4("model", model);
        cityModel.Draw(*shaderProgram);


        // render the car model
        model = glm::mat4(1.0f);
        model = glm::translate(model, startCameraTarget);
        model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(cos(glfwGetTime() / 20.0f) * 10.0f, sin(glfwGetTime() / 20.0f) * 10.0f, 0.0f));

        shaderProgram->setMat4("model", model);
        carModel.Draw(*shaderProgram);

        glm::vec3 carPosition = model * glm::vec4(0.0f, -1.0f, 1.0f, 1.0f);
        glm::vec3 carFront = model * glm::vec4(0.0f, -1.0f, 0.0f, 1.0f);
        glm::vec3 carBack = model * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

        reflector1Position = model * glm::vec4(reflector1InitialPosition, 1.0);
        reflector2Position = model * glm::vec4(reflector2InitialPosition, 1.0);

        glm::mat4 rotMatrix = glm::rotate(glm::mat4(1.0f), deltaRefPos, glm::vec3(0.0f, 1.0f, 0.0f));
        reflector1Target = rotMatrix * glm::vec4(carFront - carBack, 1.0f) + glm::vec4(0.0f, -0.05f, 0.0f, 1.0f);
        reflector2Target = rotMatrix * glm::vec4(carFront - carBack, 1.0f) + glm::vec4(0.0f, -0.05f, 0.0f, 1.0f);


        // update following camera
        followingCamera->UpdateFront(glm::normalize(carPosition - startCameraPosition));

        // update first person perspective camera
        fppCamera->UpdatePositionAndFront(carPosition, carFront - carBack);


        // render the lantern model
        model = glm::mat4(1.0f);
        model = glm::translate(model, pointlightPosition - glm::vec3(0.0f, 0.2f, 0.0f));
        model = glm::scale(model, glm::vec3(0.02f, 0.02f, 0.02f));

        shaderProgram->setMat4("model", model);
        lanternModel.Draw(*shaderProgram);


        // render the spotlight model
        model = glm::mat4(1.0f);
        model = glm::translate(model, spotlightPosition - glm::vec3(0.0f, 0.1f, 0.0f));
        model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        shaderProgram->setMat4("model", model);
        spotlightModel.Draw(*shaderProgram);


        // render Bezier surface
        model = glm::mat4(1.0f);
        //model = glm::translate(model, glm::vec3(0.0f, 0.1f, 0.0f));
        model = glm::scale(model, glm::vec3(2.0f, 1.0f, 1.0f));
        shaderProgram->setMat4("model", model);

        glBindVertexArray(bezierVAO);
        glDrawArrays(GL_TRIANGLES, 0, accuracy * accuracy * 6);


        // activate second shader for rendering tag cubes
        lightShaderProgram.use();
        lightShaderProgram.setMat4("projection", projection);
        lightShaderProgram.setMat4("view", view);


        // create model matrix for pointlight tag cube
        model = glm::mat4(1.0f);
        model = glm::translate(model, pointlightPosition);
        model = glm::scale(model, glm::vec3(0.025f));
        lightShaderProgram.setMat4("model", model);

        // render pointlight tag cube
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);


        // create model matrix for spotlight tag cube
        model = glm::mat4(1.0f);
        model = glm::translate(model, spotlightPosition);
        model = glm::scale(model, glm::vec3(0.025f));
        lightShaderProgram.setMat4("model", model);

        // render spotlight tag cube
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);


        // create model matrix for reflector 1 tag cube
        model = glm::mat4(1.0f);
        model = glm::translate(model, reflector1Position);
        model = glm::scale(model, glm::vec3(0.008f));
        lightShaderProgram.setMat4("model", model);

        // render reflector 1 tag cube
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);


        // create model matrix for reflector 2 tag cube
        model = glm::mat4(1.0f);
        model = glm::translate(model, reflector2Position);
        model = glm::scale(model, glm::vec3(0.008f));
        lightShaderProgram.setMat4("model", model);

        // render reflector 2 tag cube
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);


        // check and call events and swap the buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete []bezierVertices;

    delete PhongShaderProgram;
    delete GouraudShaderProgram;
    delete FlatShaderProgram;

    delete stationaryCamera;
    delete followingCamera;
    delete fppCamera;
    delete freeCamera;

    // delete OpenGL's resources
    glDeleteVertexArrays(1, &bezierVAO);
    glDeleteBuffers(1, &bezierVBO);

    glDeleteVertexArrays(1, &lightVAO);
    glDeleteBuffers(1, &VBO);

    // terminate GLFW's resources
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (freeCameraActive)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            activeCamera->ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            activeCamera->ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            activeCamera->ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            activeCamera->ProcessKeyboard(RIGHT, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
    {
        activeCamera = stationaryCamera;
        freeCameraActive = false;
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
    {
        activeCamera = followingCamera;
        freeCameraActive = false;
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
    {
        activeCamera = fppCamera;
        freeCameraActive = false;
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
    {
        activeCamera = freeCamera;
        freeCameraActive = true;
        firstMouse = true;
    }

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        shaderProgram = FlatShaderProgram;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        shaderProgram = GouraudShaderProgram;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        shaderProgram = PhongShaderProgram;

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
    {
        if (deltaRefPos < 0.5f)
            deltaRefPos += 0.005f;
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        deltaRefPos = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    {
        if (deltaRefPos > -0.5f)
            deltaRefPos -= 0.005f;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_N && action == GLFW_PRESS)
    {
        if (isDay == 1)
            isDay = 0;
        else
            isDay = 1;
    }
    if (key == GLFW_KEY_M && action == GLFW_PRESS)
    {
        if (isFogEnabled == 0)
            isFogEnabled = 1;
        else
            isFogEnabled = 0;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (!freeCameraActive)
        return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    activeCamera->ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    activeCamera->ProcessMouseScroll(static_cast<float>(yoffset));
}

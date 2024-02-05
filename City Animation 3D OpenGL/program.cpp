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

void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// settings
const unsigned int SCR_WIDTH = 1800;
const unsigned int SCR_HEIGHT = 1013;

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
glm::vec3 lightPos(1.2f, 10.0f, 2.0f);
Shader* PhongShaderProgram;
Shader* GouraudShaderProgram;
Shader* FlatShaderProgram;
Shader* shaderProgram;

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "City Animation", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
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

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model)
    //stbi_set_flip_vertically_on_load(true);

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
    //Model backpackModel("Resources/Backpack/backpack.obj");
    Model cityModel("Resources/City/city.obj");
    Model carModel("Resources/Car/car.obj");
    Model spotlightModel("Resources/Spotlight/spotlight.obj");

    //// set up cube positions
    //glm::vec3 cubePositions[] = {
    //    glm::vec3(0.0f,  0.0f,  0.0f),
    //    glm::vec3(2.0f,  5.0f, -15.0f),
    //    glm::vec3(-1.5f, -2.2f, -2.5f),
    //    glm::vec3(-3.8f, -2.0f, -12.3f),
    //    glm::vec3(2.4f, -0.4f, -3.5f),
    //    glm::vec3(-1.7f,  3.0f, -7.5f),
    //    glm::vec3(1.3f, -2.0f, -2.5f),
    //    glm::vec3(1.5f,  2.0f, -2.5f),
    //    glm::vec3(1.5f,  0.2f, -1.5f),
    //    glm::vec3(-1.3f,  1.0f, -1.5f)
    //};

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

    //// configure vertex attributes
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    //glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // configure light object
    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //// load and create textures
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //int width, height, nrChannels;
    //unsigned char* data;
    //stbi_set_flip_vertically_on_load(true);

    //unsigned int texture0;
    //glGenTextures(1, &texture0);
    //glBindTexture(GL_TEXTURE_2D, texture0);

    //data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);
    //if (data)
    //{
    //    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    //    glGenerateMipmap(GL_TEXTURE_2D);
    //}
    //else
    //{
    //    std::cout << "Failed to load texture 1" << std::endl;
    //}
    //stbi_image_free(data);

    //unsigned int texture1;
    //glGenTextures(1, &texture1);
    //glBindTexture(GL_TEXTURE_2D, texture1);

    //data = stbi_load("awesomeface.png", &width, &height, &nrChannels, 0);
    //if (data)
    //{
    //    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    //    glGenerateMipmap(GL_TEXTURE_2D);
    //}
    //else
    //{
    //    std::cout << "Failed to load texture 2" << std::endl;
    //}
    //stbi_image_free(data);

    //shaderProgram->use();
    //shaderProgram->setInt("texture0", 0);
    //shaderProgram->setInt("texture1", 1);

    // load textures (we now use a utility function to keep the code more organized)
    // -----------------------------------------------------------------------------
    //unsigned int diffuseMap = loadTexture("Resources/container2_diffuse.png");
    //unsigned int specularMap = loadTexture("Resources/container2_specular.png");

    // shader configuration
    // --------------------
    shaderProgram->use();
    shaderProgram->setInt("material.texture_diffuse", 0);
    shaderProgram->setInt("material.texture_specular", 1);

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
        glClearColor(0.529f, 0.808f, 0.922f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //// change light position
        lightPos.x = 1.0f + sin(glfwGetTime()) * 2.0f;
        //lightPos.y = sin(glfwGetTime() / 2.0f) * 1.0f;

        // bind textures
        //glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_2D, texture0);
        //glActiveTexture(GL_TEXTURE1);
        //glBindTexture(GL_TEXTURE_2D, texture1);

        // activate shaders
        shaderProgram->use();

        //shaderProgram->setVec3("objectColor", 1.0f, 0.5f, 0.31f);
        //shaderProgram->setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        //shaderProgram->setVec3("lightPos", lightPos);
        shaderProgram->setVec3("viewPos", activeCamera->Position);

        //shaderProgram->setVec3("materialAmbient", 1.0f, 0.5f, 0.31f);
        //shaderProgram->setVec3("materialDiffuse", 1.0f, 0.5f, 0.31f);
        //shaderProgram->setVec3("materialSpecular", 0.5f, 0.5f, 0.5f);
        //shaderProgram->setFloat("materialShininess", 32.0f);

        //shaderProgram->setVec3("lightAmbient", 0.2f, 0.2f, 0.2f);
        //shaderProgram->setVec3("lightDiffuse", 0.5f, 0.5f, 0.5f);
        //shaderProgram->setVec3("lightSpecular", 1.0f, 1.0f, 1.0f);

        //shaderProgram->setVec3("material.ambient", 1.0f, 0.5f, 0.31f);
        //shaderProgram->setVec3("material.diffuse", 1.0f, 0.5f, 0.31f);
        //shaderProgram->setVec3("material.specular", 0.5f, 0.5f, 0.5f);
        //shaderProgram->setFloat("material.shininess", 32.0f);

        shaderProgram->setVec3("light.position", lightPos);
        shaderProgram->setVec3("light.ambient", 0.8f, 0.8f, 0.8f);
        shaderProgram->setVec3("light.diffuse", 0.8f, 0.8f, 0.8f);
        shaderProgram->setVec3("light.specular", 0.8f, 0.8f, 0.8f);

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

        // update following camera
        followingCamera->UpdateFront(glm::normalize(carPosition - startCameraPosition));

        // update first person perspective camera
        fppCamera->UpdatePositionAndFront(carPosition, carFront - carBack);

        // render the spotlight model
        model = glm::mat4(1.0f);
        model = glm::translate(model, startCameraTarget);
        model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));

        shaderProgram->setMat4("model", model);
        spotlightModel.Draw(*shaderProgram);

        // create model matrix for light
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f));

        // render light cube
        lightShaderProgram.use();
        lightShaderProgram.setMat4("projection", projection);
        lightShaderProgram.setMat4("view", view);
        lightShaderProgram.setMat4("model", model);

        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // check and call events and swap the buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete PhongShaderProgram;
    delete GouraudShaderProgram;
    delete FlatShaderProgram;

    // delete OpenGL's resources
    glDeleteVertexArrays(1, &VAO);
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

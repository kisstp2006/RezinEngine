#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


#include <Rezin/Graphics/ShaderProgram.hpp>
#include <Rezin/Utilities/Log.hpp>
#include <Rezin/Assets/Texture/Texture.hpp>

#include <exception>

#include <iostream>
#include <cstdio>
#include <string>
#include <cmath>


using namespace std;
using namespace rezin;

void framebuffer_size_change_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0,0,width,height);
}

void process_input(GLFWwindow *window)
{
    if(glfwGetKey(window,GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}


int main()
{
    int width = 1280;
    int height = 720;
    string windowName="RezinEngine";


    float vertices[] = {
    // positions          // colors           // texture coords
     0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
     0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
    -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left
    };

    unsigned int indices[] = {
    0, 1, 3,
    1, 2, 3
    };



    if(glfwInit() != GLFW_TRUE){
        Log::Error("Failed to initialize GLFW");
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); //we set the minor and major version of opengl
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(width,height,windowName.c_str(),NULL,NULL);


    //Shutdown the engine if GLFW failed to create a window
    if(window == NULL){
        Log::Error("Failed to create GLFW window: " + windowName);
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        Log::Error("Failed to load GLAD (OpenGL)");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    Log::Info("Maximum nr of vertex attributes supported: " + std::to_string(nrAttributes));

    glViewport(0,0,width,height);
    glfwSetFramebufferSizeCallback(
    window,
    framebuffer_size_change_callback
    );

    TextureSpecification textureSpecification;
    textureSpecification.flipVertically=true;
    textureSpecification.generateMipmaps=true;
    textureSpecification.srgb=true;

    Texture2D texture1(
                       "assets/texture/missingTexture.png",
                        textureSpecification);

    Texture2D texture2(
                       "assets/texture/Happy_smiley_face.png",
                        textureSpecification);



    //Load test Texture

    int exitCode=0;
    try
{
    rezin::ShaderProgram shader(
        "assets/shaders/basic.vert",
        "assets/shaders/basic.frag"
    );

    shader.setInt("texture1",0); // set 0th uniform to ourTexture

    shader.setInt("texture2",1); // set 1th uniform to ourTexture

    unsigned int VAO, VBO, EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);



    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(indices),
        indices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        reinterpret_cast<void*>(0)
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        reinterpret_cast<void*>(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        reinterpret_cast<void*>(6 * sizeof(float))
        );

    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window))
    {
        process_input(window);

        glClearColor(0.2F, 0.5F, 1.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);


        texture1.bind(0);
        texture2.bind(1);

        shader.use();

        glBindVertexArray(VAO);
        glDrawElements(
            GL_TRIANGLES,
            6,
            GL_UNSIGNED_INT,
            nullptr
        );
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }



    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
}
catch (const std::exception& error)
{
    Log::Error(std::string("Shader hiba: ") + error.what());

    exitCode = -1;
}


    glfwDestroyWindow(window);
    glfwTerminate();

    return exitCode;
}

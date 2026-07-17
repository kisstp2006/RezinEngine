#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


#include <Rezin/Graphics/ShaderProgram.hpp>
#include <Rezin/Utilities/Log.hpp>
#include <Rezin/Assets/Texture/Texture.hpp>
#include <Rezin/Application/Application.hpp>


#include <exception>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cstdio>
#include <string>
#include <cmath>
#include <memory>
#include <utility>


using namespace std;
using namespace rezin;

namespace
{
    constexpr float cubeVertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    class SandboxApplication final : public Application
    {
        public:
            explicit SandboxApplication(
            rezin::ApplicationSpecification specification
        )
            : Application(std::move(specification))
        {
        }

        protected:
            void onStartup() override
            {
                glEnable(GL_DEPTH_TEST);

                int maximumAttributes = 0;
                glGetIntegerv(
                    GL_MAX_VERTEX_ATTRIBS,
                    &maximumAttributes
                );

                Log::Info(
                    "Maximum number of vertex attributes supported: "
                    + std::to_string(maximumAttributes)
                );

                rezin::TextureSpecification textureSpecification;
                textureSpecification.flipVertically = true;
                textureSpecification.generateMipmaps = true;
                textureSpecification.srgb = true;

                texture1_ = std::make_unique<rezin::Texture2D>(
                    "assets/texture/missingTexture.png",
                    textureSpecification
                );

                texture2_ = std::make_unique<rezin::Texture2D>(
                    "assets/texture/Happy_smiley_face.png",
                    textureSpecification
                );

                shader_ = std::make_unique<rezin::ShaderProgram>(
                    "assets/shaders/basic.vert",
                    "assets/shaders/basic.frag"
                );

                shader_->setInt("texture1", 0);
                shader_->setInt("texture2", 1);

                view_ = glm::translate(
                    glm::mat4(1.0f),
                    glm::vec3(0.0f, 0.0f, -3.0f)
                );

                updateProjection(width(), height());

                shader_->setMat4("model", model_);
                shader_->setMat4("view", view_);
                shader_->setMat4("projection", projection_);

                glGenVertexArrays(1, &vao_);
                glGenBuffers(1, &vbo_);

                glBindVertexArray(vao_);

                glBindBuffer(GL_ARRAY_BUFFER, vbo_);
                glBufferData(
                    GL_ARRAY_BUFFER,
                    sizeof(cubeVertices),
                    cubeVertices,
                    GL_STATIC_DRAW
                );

                glVertexAttribPointer(
                    0,
                    3,
                    GL_FLOAT,
                    GL_FALSE,
                    5 * sizeof(float),
                    reinterpret_cast<void*>(0)
                );
                glEnableVertexAttribArray(0);

                glVertexAttribPointer(
                    2,
                    2,
                    GL_FLOAT,
                    GL_FALSE,
                    5 * sizeof(float),
                    reinterpret_cast<void*>(3 * sizeof(float))
                );
                glEnableVertexAttribArray(2);

                glBindVertexArray(0);
            }

            void onUpdate(float deltaSeconds) override
            {
                if (
                    glfwGetKey(nativeWindow(), GLFW_KEY_ESCAPE)
                    == GLFW_PRESS
                )
                {
                    requestQuit();
                    return;
                }

                elapsedTime_ += deltaSeconds;

                model_ = glm::rotate(
                    glm::mat4(1.0f),
                    elapsedTime_ * glm::radians(50.0f),
                    glm::vec3(0.5f, 1.0f, 0.0f)
                );

                shader_->setMat4("model", model_);
            }
            void onRender() override
            {
                glClearColor(0.2f, 0.5f, 1.0f, 1.0f);
                glClear(
                    GL_COLOR_BUFFER_BIT
                    | GL_DEPTH_BUFFER_BIT
                );

                texture1_->bind(0);
                texture2_->bind(1);

                shader_->use();

                glBindVertexArray(vao_);
                glDrawArrays(GL_TRIANGLES, 0, 36);
                glBindVertexArray(0);
            }

            void onShutdown() noexcept override
            {
                if (vbo_ != 0)
                {
                    glDeleteBuffers(1, &vbo_);
                    vbo_ = 0;
                }

                if (vao_ != 0)
                {
                    glDeleteVertexArrays(1, &vao_);
                    vao_ = 0;
                }


                shader_.reset();
                texture2_.reset();
                texture1_.reset();
            }
            void onFramebufferResize(
                std::uint32_t newWidth,
                std::uint32_t newHeight
            ) override
            {
                if (newHeight == 0)
                    return;

                updateProjection(newWidth, newHeight);

                if (shader_)
                {
                    shader_->setMat4(
                        "projection",
                        projection_
                    );
                }
            }



        private:

            void updateProjection(
                std::uint32_t framebufferWidth,
                std::uint32_t framebufferHeight
            )
            {
                if (framebufferHeight == 0)
                    return;

                projection_ = glm::perspective(
                    glm::radians(45.0f),
                    static_cast<float>(framebufferWidth)
                        / static_cast<float>(framebufferHeight),
                    0.1f,
                    100.0f
                );
            }

            std::unique_ptr<rezin::ShaderProgram> shader_;
            std::unique_ptr<rezin::Texture2D> texture1_;
            std::unique_ptr<rezin::Texture2D> texture2_;

            GLuint vao_ = 0;
            GLuint vbo_ = 0;

            glm::mat4 model_{1.0f};
            glm::mat4 view_{1.0f};
            glm::mat4 projection_{1.0f};

            float elapsedTime_ = 0.0f;
        };

}



int main()
{
    try
    {
        ApplicationSpecification specification;
        specification.name="Rezin Sandbox";
        specification.resizable=false;
        specification.vsync=true;
        specification.width = 1280;
        specification.height = 720;

         SandboxApplication application(
            std::move(specification)
        );

        return application.run();


    }
    catch (const std::exception& error)
    {
        Log::Error(
            std::string("Fatal application error: ")
            + error.what()
        );

        return -1;
    }

}

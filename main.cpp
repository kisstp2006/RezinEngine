#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


#include <Rezin/Utilities/Log.hpp>
#include <Rezin/Graphics/Buffer.hpp>
#include <Rezin/Graphics/VertexArray.hpp>
#include <Rezin/Graphics/ShaderProgram.hpp>
#include <Rezin/Assets/Texture/Texture.hpp>
#include <Rezin/Application/Application.hpp>


#include <exception>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>



#include <string>
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

    glm::vec3 cubePositions[] = {
    glm::vec3( 0.0f,  0.0f,  0.0f),
    glm::vec3( 2.0f,  5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3( 2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f,  3.0f, -7.5f),
    glm::vec3( 1.3f, -2.0f, -2.5f),
    glm::vec3( 1.5f,  2.0f, -2.5f),
    glm::vec3( 1.5f,  0.2f, -1.5f),
    glm::vec3(-1.3f,  1.0f, -1.5f)
};

    //Camera position
    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    //Camera direction
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);
    //Right axis
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));
    //Up Axis
    glm::vec3 cameraUp = glm::cross(cameraDirection, cameraRight);
    //Look At
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );


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

                vertexBuffer_ = std::make_unique<VertexBuffer>(
                    cubeVertices,
                    sizeof(cubeVertices)
                );

                vertexBuffer_->setLayout({
                    {ShaderDataType::Float3, "aPos"},
                    {ShaderDataType::Float2, "aTexCoord"}
                });

                vertexArray_ = std::make_unique<VertexArray>();

                vertexArray_->addVertexBuffer(
                    *vertexBuffer_
                );


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


                const float radius = 10.0f;
                float camX = sin(glfwGetTime()/10) * radius;
                float camZ = cos(glfwGetTime()/10) * radius;

                view_ = glm::lookAt(glm::vec3(camX, 0.0, camZ), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

                shader_->setMat4("model", model_);
                shader_->setMat4("view", view_);
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
                vertexArray_->bind();

                for (unsigned int i = 0; i < 10; ++i)
                {
                    glm::mat4 model = glm::mat4(1.0f);

                    model = glm::translate(
                        model,
                        cubePositions[i]
                    );

                    const float angle = 20.0f * static_cast<float>(i);

                    model = glm::rotate(
                        model,
                        glm::radians(angle),
                        glm::vec3(1.0f, 0.3f, 0.5f)
                    );

                    shader_->setMat4("model", model);

                    glDrawArrays(
                        GL_TRIANGLES,
                        0,
                        36
                    );
                }

                VertexArray::unbind();


            }

            void onShutdown() noexcept override
            {

                vertexArray_.reset();

                vertexBuffer_.reset();

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

            std::unique_ptr<VertexBuffer> vertexBuffer_;
            std::unique_ptr<VertexArray> vertexArray_;

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

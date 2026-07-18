#include <glad/glad.h>


#include <Rezin/ECS/ECS.hpp>
#include <Rezin/Input/Input.hpp>
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

                EntityManager& entities = world_.entityManager();
                cameraEntity_ = entities.createEntity();

                TransformComponent transform;
                transform.position = glm::vec3(0.0f, 0.0f, 3.0f);

                entities.addComponentData(
                    cameraEntity_,
                    transform
                );

                CameraComponent camera;
                camera.projection = CameraProjection::Perspective;
                camera.verticalFieldOfView = 45.0f;
                camera.nearClip = 0.1f;
                camera.farClip = 100.0f;
                camera.primary = true;

                entities.addComponentData(
                    cameraEntity_,
                    camera
                );

                // The camera entity and both required components must exist
                // before the view and projection functions query them.
                updateProjection(width(), height());
                updateCameraView();

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
                if (Input::getKeyDown(KeyCode::Escape))
                {
                    requestQuit();
                    return;
                }

                updateCameraRotation();
                updateCameraMovement(deltaSeconds);
                updateCameraView();
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
                EntityManager& entities = world_.entityManager();

                if (entities.exists(cameraEntity_))
                    entities.destroyEntity(cameraEntity_);

                cameraEntity_ = {};

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
            World world_{"Main World"};
            Entity cameraEntity_;

            void updateCameraMovement(float deltaSeconds)
            {
                TransformComponent& transform =
                    world_.entityManager()
                        .getComponentData<TransformComponent>(
                            cameraEntity_
                        );

                const float movement =
                    cameraMovementSpeed_ * deltaSeconds;

                // Convert the camera's local axes into world-space directions.
                const glm::vec3 cameraFront =
                    transform.rotation
                    * glm::vec3(0.0f, 0.0f, -1.0f);

                const glm::vec3 cameraRight =
                    transform.rotation
                    * glm::vec3(1.0f, 0.0f, 0.0f);

                if (Input::getKey(KeyCode::W))
                    transform.position += cameraFront * movement;

                if (Input::getKey(KeyCode::S))
                    transform.position -= cameraFront * movement;

                if (Input::getKey(KeyCode::A))
                    transform.position -= cameraRight * movement;

                if (Input::getKey(KeyCode::D))
                    transform.position += cameraRight * movement;
            }

            void updateCameraRotation()
            {
                if (!Input::getMouseButton(MouseButton::Right))
                    return;

                const glm::vec2 mouseMovement =
                    Input::mouseDelta();

                cameraYaw_ +=
                    mouseMovement.x * mouseSensitivity_;

                cameraPitch_ +=
                    mouseMovement.y * mouseSensitivity_;

                cameraPitch_ = glm::clamp(
                    cameraPitch_,
                    -89.0f,
                    89.0f
                );

                glm::vec3 cameraDirection;

                cameraDirection.x =
                    glm::cos(glm::radians(cameraYaw_))
                    * glm::cos(glm::radians(cameraPitch_));

                cameraDirection.y =
                    glm::sin(glm::radians(cameraPitch_));

                cameraDirection.z =
                    glm::sin(glm::radians(cameraYaw_))
                    * glm::cos(glm::radians(cameraPitch_));

                cameraDirection =
                    glm::normalize(cameraDirection);

                TransformComponent& transform =
                    world_.entityManager()
                        .getComponentData<TransformComponent>(
                            cameraEntity_
                        );

                // Build a quaternion that points the local forward axis toward
                // the calculated world-space direction.
                transform.rotation = glm::quatLookAtRH(
                    cameraDirection,
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );
            }

            void updateCameraView()
            {
                const TransformComponent& transform =
                    world_.entityManager()
                        .getComponentData<TransformComponent>(
                            cameraEntity_
                        );

                const glm::vec3 cameraFront =
                    transform.rotation
                    * glm::vec3(0.0f, 0.0f, -1.0f);

                const glm::vec3 cameraUp =
                    transform.rotation
                    * glm::vec3(0.0f, 1.0f, 0.0f);

                view_ = glm::lookAt(
                    transform.position,
                    transform.position + cameraFront,
                    cameraUp
                );

                shader_->setMat4("view", view_);
            }

            void updateProjection(
                std::uint32_t framebufferWidth,
                std::uint32_t framebufferHeight
            )
            {
                if (framebufferHeight == 0)
                    return;

                const CameraComponent& camera =
                    world_.entityManager()
                        .getComponentData<CameraComponent>(
                            cameraEntity_
                        );

                const float aspectRatio =
                    static_cast<float>(framebufferWidth)
                    / static_cast<float>(framebufferHeight);

                if (
                    camera.projection
                    == CameraProjection::Perspective
                )
                {
                    projection_ = glm::perspective(
                        glm::radians(camera.verticalFieldOfView),
                        aspectRatio,
                        camera.nearClip,
                        camera.farClip
                    );
                }
                else
                {
                    const float halfHeight =
                        camera.orthographicSize * 0.5f;

                    const float halfWidth =
                        halfHeight * aspectRatio;

                    projection_ = glm::ortho(
                        -halfWidth,
                        halfWidth,
                        -halfHeight,
                        halfHeight,
                        camera.nearClip,
                        camera.farClip
                    );
                }
            }

            std::unique_ptr<ShaderProgram> shader_;
            std::unique_ptr<Texture2D> texture1_;
            std::unique_ptr<Texture2D> texture2_;

            std::unique_ptr<VertexBuffer> vertexBuffer_;
            std::unique_ptr<VertexArray> vertexArray_;

            glm::mat4 view_{1.0f};
            glm::mat4 projection_{1.0f};

            float cameraYaw_ = -90.0f;
            float cameraPitch_ = 0.0f;

            static constexpr float cameraMovementSpeed_ = 2.5f;
            static constexpr float mouseSensitivity_ = 0.1f;
    };

}

int main()
{
    try
    {
        ApplicationSpecification specification;
        specification.name="Rezin Sandbox";
        specification.resizable=true;
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

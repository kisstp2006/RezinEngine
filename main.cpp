#include <glad/glad.h>

#include <Rezin/ECS/ECS.hpp>
#include <Rezin/Input/Input.hpp>
#include <Rezin/Utilities/Log.hpp>
#include <Rezin/Graphics/Buffer.hpp>
#include <Rezin/Graphics/VertexArray.hpp>
#include <Rezin/Assets/Texture/Texture.hpp>
#include <Rezin/Graphics/ShaderProgram.hpp>
#include <Rezin/Application/Application.hpp>

#include <exception>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <memory>
#include <utility>


using namespace rezin;

namespace
{
    // Each vertex stores position.xyz, normal.xyz, and textureCoordinates.xy.
    constexpr float cubeVertices[] = {
        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        // Left face
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        // Right face
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
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



                objectShader_ = std::make_unique<rezin::ShaderProgram>(
                    "assets/shaders/lighting.vert",
                    "assets/shaders/lighting.frag"
                );

                lightShader_ = std::make_unique<rezin::ShaderProgram>(
                    "assets/shaders/lightCube.vert",
                    "assets/shaders/lightCube.frag"
                );

                // Specular light stays white while ambient and diffuse colors
                // are animated every frame in updateLightColor().
                objectShader_->setVec3(
                    "light.specular",
                    glm::vec3(1.0f)
                );
                objectShader_->setVec3(
                    "light.position",
                    lightPosition_
                );

                objectShader_->setVec3(
                    "material.specular",
                    glm::vec3(0.5f, 0.5f, 0.5f)
                );

                objectShader_->setFloat("material.shininess", 32.0f);

                diffuseMap_ = std::make_unique<Texture2D>(
                    "assets/texture/missingTexture.png"
                );

                objectShader_->setInt("material.diffuse", 0);



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

                objectShader_->setMat4("projection", projection_);
                lightShader_->setMat4("projection", projection_);

                vertexBuffer_ = std::make_unique<VertexBuffer>(
                    cubeVertices,
                    sizeof(cubeVertices)
                );

                vertexBuffer_->setLayout({
                    {ShaderDataType::Float3, "aPosition"},
                    {ShaderDataType::Float3, "aNormal"},
                    {ShaderDataType::Float2, "aTextureCoordinates"}
                });

                vertexArray_ = std::make_unique<VertexArray>();

                vertexArray_->addVertexBuffer(
                    *vertexBuffer_
                );

                // The light cube shares immutable vertex data with the scene
                // cubes, but owns separate VAO state. Future changes to object
                // attributes will therefore not alter the light's VAO.
                lightVertexArray_ = std::make_unique<VertexArray>();
                lightVertexArray_->addVertexBuffer(
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
                updateLightColor(deltaSeconds);
            }
            void onRender() override
            {
                glClearColor(0.2f, 0.5f, 1.0f, 1.0f);
                glClear(
                    GL_COLOR_BUFFER_BIT
                    | GL_DEPTH_BUFFER_BIT
                );

                diffuseMap_->bind(0);

                objectShader_->use();
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

                    objectShader_->setMat4("model", model);

                    glDrawArrays(
                        GL_TRIANGLES,
                        0,
                        36
                    );
                }

                // Render a small white cube at the light's world-space position.
                glm::mat4 lightModel{1.0f};
                lightModel = glm::translate(
                    lightModel,
                    lightPosition_
                );
                lightModel = glm::scale(
                    lightModel,
                    glm::vec3(0.2f)
                );

                lightShader_->use();
                lightShader_->setMat4("model", lightModel);
                lightVertexArray_->bind();

                glDrawArrays(
                    GL_TRIANGLES,
                    0,
                    36
                );

                VertexArray::unbind();


            }

            void onShutdown() noexcept override
            {
                EntityManager& entities = world_.entityManager();

                if (entities.exists(cameraEntity_))
                    entities.destroyEntity(cameraEntity_);

                cameraEntity_ = {};

                lightVertexArray_.reset();
                vertexArray_.reset();

                vertexBuffer_.reset();

                lightShader_.reset();
                objectShader_.reset();
            }
            void onFramebufferResize(
                std::uint32_t newWidth,
                std::uint32_t newHeight
            ) override
            {
                if (newHeight == 0)
                    return;

                updateProjection(newWidth, newHeight);

                if (objectShader_ && lightShader_)
                {
                    objectShader_->setMat4(
                        "projection",
                        projection_
                    );
                    lightShader_->setMat4(
                        "projection",
                        projection_
                    );
                }
            }



        private:
            World world_{"Main World"};
            Entity cameraEntity_;

            void updateLightColor(float deltaSeconds)
            {
                // Application supplies frame time, so sandbox code does not
                // need to access GLFW's platform timer directly.
                elapsedTimeSeconds_ += deltaSeconds;

                glm::vec3 lightColor;
                lightColor.x = std::sin(elapsedTimeSeconds_ * 2.0f);
                lightColor.y = std::sin(elapsedTimeSeconds_ * 0.7f);
                lightColor.z = std::sin(elapsedTimeSeconds_ * 1.3f);

                const glm::vec3 diffuseColor =
                    lightColor * glm::vec3(0.5f);
                const glm::vec3 ambientColor =
                    diffuseColor * glm::vec3(0.2f);

                objectShader_->setVec3("light.ambient", ambientColor);
                objectShader_->setVec3("light.diffuse", diffuseColor);
            }

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

                objectShader_->setMat4("view", view_);
                objectShader_->setVec3("viewPos", transform.position);
                objectShader_->setInt("material.diffuse", 0);
                                        diffuseMap_->bind(0);

                lightShader_->setMat4("view", view_);
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

            std::unique_ptr<ShaderProgram> objectShader_;
            std::unique_ptr<ShaderProgram> lightShader_;
            std::unique_ptr<Texture2D> diffuseMap_;

            std::unique_ptr<VertexBuffer> vertexBuffer_;
            std::unique_ptr<VertexArray> vertexArray_;
            std::unique_ptr<VertexArray> lightVertexArray_;

            glm::mat4 view_{1.0f};
            glm::mat4 projection_{1.0f};

            glm::vec3 lightPosition_{1.2f, 1.0f, 2.0f};

            float elapsedTimeSeconds_ = 0.0f;

            float cameraYaw_ = -90.0f;
            float cameraPitch_ = 0.0f;

            static constexpr float cameraMovementSpeed_ = 4.5f;
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

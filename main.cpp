#include <Rezin/ECS/ECS.hpp>
#include <Rezin/Input/Input.hpp>
#include <Rezin/Utilities/Log.hpp>
#include <Rezin/Assets/Model/Model.hpp>
#include <Rezin/Graphics/Renderer.hpp>
#include <Rezin/Graphics/ShaderProgram.hpp>
#include <Rezin/Application/Application.hpp>
#include <Rezin/Assets/Model/ModelImporter.hpp>

#include <exception>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <string>
#include <memory>
#include <random>
#include <utility>


using namespace rezin;

namespace
{
    constexpr std::size_t importedCubeCount = 10;

    constexpr unsigned int pointLightCount = 4;
    const glm::vec3 pointLightPositions[pointLightCount] = {
        glm::vec3( 0.7f,  0.2f,   2.0f),
        glm::vec3( 2.3f, -3.3f,  -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3( 0.0f,  0.0f,  -3.0f)
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
                const RendererCapabilities& rendererCapabilities =
                    Renderer::capabilities();

                Log::Info(
                    "Maximum number of vertex attributes supported: "
                    + std::to_string(
                        rendererCapabilities.maximumVertexAttributes
                    )
                );



                objectShader_ = std::make_unique<rezin::ShaderProgram>(
                    "assets/shaders/lighting.vert",
                    "assets/shaders/lighting.frag"
                );

                lightShader_ = std::make_unique<rezin::ShaderProgram>(
                    "assets/shaders/lightCube.vert",
                    "assets/shaders/lightCube.frag"
                );

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

                // Adding a spot light to the camera entity makes the light use
                // the same position and rotation as the camera automatically.
                LightComponent cameraLight;
                cameraLight.type = LightType::Spot;
                cameraLight.ambientIntensity = 0.0f;
                cameraLight.diffuseIntensity = 1.0f;
                cameraLight.specularIntensity = 1.0f;

                entities.addComponentData(
                    cameraEntity_,
                    cameraLight
                );

                directionalLightEntity_ = entities.createEntity();

                TransformComponent directionalTransform;
                directionalTransform.rotation = glm::quatLookAtRH(
                    glm::normalize(glm::vec3(-0.2f, -1.0f, -0.3f)),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );

                LightComponent directionalLight;
                directionalLight.type = LightType::Directional;
                directionalLight.ambientIntensity = 0.05f;
                directionalLight.diffuseIntensity = 0.4f;
                directionalLight.specularIntensity = 0.5f;

                entities.addComponentData(
                    directionalLightEntity_,
                    directionalTransform
                );
                entities.addComponentData(
                    directionalLightEntity_,
                    directionalLight
                );

                for (std::size_t index = 0; index < pointLightCount; ++index)
                {
                    pointLightEntities_[index] = entities.createEntity();

                    TransformComponent pointTransform;
                    pointTransform.position = pointLightPositions[index];

                    LightComponent pointLight;
                    pointLight.type = LightType::Point;
                    pointLight.ambientIntensity = 0.05f;
                    pointLight.diffuseIntensity = 0.8f;
                    pointLight.specularIntensity = 1.0f;

                    entities.addComponentData(
                        pointLightEntities_[index],
                        pointTransform
                    );
                    entities.addComponentData(
                        pointLightEntities_[index],
                        pointLight
                    );
                }

                // World owns the system. The shader must outlive the system
                // because LightSystem stores a reference to it.
                world_.getOrCreateSystem<LightSystem>(*objectShader_);

                // The camera entity and both required components must exist
                // before the view and projection functions query them.
                updateProjection(width(), height());
                updateCameraView();

                objectShader_->setMat4("projection", projection_);
                lightShader_->setMat4("projection", projection_);

                ModelRenderSpecification modelSpecification;

                modelSpecification.fallbackDiffuseTexture =
                    "assets/texture/missingTexture.png";

                modelSpecification.fallbackSpecularTexture =
                    "assets/texture/missingTexture_specular.png";

                modelSpecification.diffuseTextureSlot = 0;
                modelSpecification.specularTextureSlot = 1;
                modelSpecification.defaultShininess = 32.0f;

                model_ = std::make_unique<Model>(
                    ModelImporter::load(
                        "assets/model/Cube.fbx"
                    ),
                    modelSpecification
                );

                generateImportedCubeTransforms();
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

                // Systems run after gameplay has updated component data and
                // before rendering consumes the resulting shader uniforms.
                world_.update(deltaSeconds);
            }
            void onRender() override
            {
                if (model_)
                {
                    for (const glm::mat4& modelTransform
                         : importedCubeTransforms_)
                    {
                        objectShader_->setMat4(
                            "model",
                            modelTransform
                        );
                        model_->draw(*objectShader_);
                    }
                }

                lightShader_->use();

                // Each point light gets a small visible marker cube. The
                // directional light and camera flashlight have no marker. The
                // same imported geometry is reused without material binding.
                for (unsigned int index = 0; index < pointLightCount; ++index)
                {
                    const TransformComponent& lightTransform =
                        world_.entityManager()
                            .getComponentData<TransformComponent>(
                                pointLightEntities_[index]
                            );

                    glm::mat4 lightModel{1.0f};
                    lightModel = glm::translate(
                        lightModel,
                        lightTransform.position
                    );
                    lightModel = glm::scale(
                        lightModel,
                        glm::vec3(0.001f)
                    );

                    lightShader_->setMat4("model", lightModel);
                    model_->drawGeometry();
                }
            }

            void onShutdown() noexcept override
            {
                EntityManager& entities = world_.entityManager();

                if (entities.exists(directionalLightEntity_))
                    entities.destroyEntity(directionalLightEntity_);

                directionalLightEntity_ = {};

                for (Entity& pointLightEntity : pointLightEntities_)
                {
                    if (entities.exists(pointLightEntity))
                        entities.destroyEntity(pointLightEntity);

                    pointLightEntity = {};
                }

                if (entities.exists(cameraEntity_))
                    entities.destroyEntity(cameraEntity_);

                cameraEntity_ = {};

                // Destroy systems before releasing resources referenced by
                // those systems, including objectShader_.
                world_.shutdown();

                // Model owns OpenGL meshes and textures, so release it while
                // the application still has a valid OpenGL context.
                model_.reset();

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
            Entity directionalLightEntity_;
            std::array<Entity, pointLightCount> pointLightEntities_{};
            std::unique_ptr<Model> model_;
            std::array<glm::mat4, importedCubeCount>
                importedCubeTransforms_{};

            void generateImportedCubeTransforms()
            {
                std::mt19937 randomGenerator{
                    std::random_device{}()
                };

                std::uniform_real_distribution<float> horizontalPosition(
                    -7.0f,
                    7.0f
                );
                std::uniform_real_distribution<float> verticalPosition(
                    -3.5f,
                    3.5f
                );
                std::uniform_real_distribution<float> depthPosition(
                    -22.0f,
                    -3.0f
                );
                std::uniform_real_distribution<float> rotationAngle(
                    0.0f,
                    360.0f
                );
                std::uniform_real_distribution<float> axisComponent(
                    -1.0f,
                    1.0f
                );
                std::uniform_real_distribution<float> relativeScale(
                    0.65f,
                    1.35f
                );

                for (glm::mat4& transform : importedCubeTransforms_)
                {
                    const glm::vec3 position{
                        horizontalPosition(randomGenerator),
                        verticalPosition(randomGenerator),
                        depthPosition(randomGenerator)
                    };

                    glm::vec3 rotationAxis{
                        axisComponent(randomGenerator),
                        axisComponent(randomGenerator),
                        axisComponent(randomGenerator)
                    };

                    if (glm::dot(rotationAxis, rotationAxis) < 0.0001f)
                        rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
                    else
                        rotationAxis = glm::normalize(rotationAxis);

                    transform = glm::mat4(1.0f);
                    transform = glm::translate(transform, position);
                    transform = glm::rotate(
                        transform,
                        glm::radians(rotationAngle(randomGenerator)),
                        rotationAxis
                    );

                    // Cube.fbx is 200 units wide. A base scale of 0.005
                    // converts it to a one-unit cube before random variation.
                    const float modelScale =
                        0.005f * relativeScale(randomGenerator);

                    transform = glm::scale(
                        transform,
                        glm::vec3(modelScale)
                    );
                }
            }

            void updateLightColor(float deltaSeconds)
            {
                // Application supplies frame time, so sandbox code does not
                // need to access GLFW's platform timer directly.
                elapsedTimeSeconds_ += deltaSeconds;

                glm::vec3 lightColor;
                lightColor.x = std::sin(elapsedTimeSeconds_ * 2.0f);
                lightColor.y = std::sin(elapsedTimeSeconds_ * 0.7f);
                lightColor.z = std::sin(elapsedTimeSeconds_ * 1.3f);

                // Change component data instead of talking to the shader. The
                // LightSystem reads this value later in the same frame.
                LightComponent& cameraLight =
                    world_.entityManager()
                        .getComponentData<LightComponent>(cameraEntity_);

                cameraLight.color = lightColor;
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

            glm::mat4 view_{1.0f};
            glm::mat4 projection_{1.0f};

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
        specification.renderer.clearColor =
            glm::vec4(0.2f, 0.5f, 1.0f, 1.0f);
        specification.renderer.depthTesting = true;
        specification.renderer.depthWrite = true;
        specification.renderer.depthFunction =
            DepthCompareFunction::Less;
        specification.renderer.depthBufferBits = 24;

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

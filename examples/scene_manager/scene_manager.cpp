#include "scene_manager.h"

const char* modelPath = {};

scene_manager::scene_manager() : aph::BaseApp("scene_manager") {}

void scene_manager::init()
{
    setupWindow();
    setupRenderer();
    setupScene();
}

void scene_manager::run()
{
    while(!m_window->shouldClose())
    {
        static float deltaTime = {};
        auto         timer     = aph::Timer(deltaTime);
        m_window->pollEvents();

        // update scene object
        m_modelNode->rotate(1.0f * deltaTime, {0.0f, 1.0f, 0.0f});
        m_camController->updateProj();
        m_camController->updateView();
        m_camController->updateMovement(deltaTime);

        // update resource data
        m_sceneRenderer->update(deltaTime);
        m_uiRenderer->update(deltaTime);

        // draw and submit
        m_sceneRenderer->beginFrame();
        m_sceneRenderer->recordDrawSceneCommands();
        m_sceneRenderer->endFrame();
    }
}

void scene_manager::finish()
{
    m_sceneRenderer->idleDevice();
    m_sceneRenderer->cleanupResources();
    m_uiRenderer->cleanup();
    m_sceneRenderer->cleanup();
}

void scene_manager::setupWindow()
{
    m_window = aph::Window::Create(1440, 768);

    m_window->setCursorPosCallback([=](double xposIn, double yposIn) { this->mouseHandleDerive(xposIn, yposIn); });

    m_window->setFramebufferSizeCallback([=](int width, int height) {
        // this->m_framebufferResized = true;
    });

    m_window->setKeyCallback(
        [=](int key, int scancode, int action, int mods) { this->keyboardHandleDerive(key, scancode, action, mods); });
}

void scene_manager::setupScene()
{
    // scene global argument setup
    {
        m_scene = aph::Scene::Create(aph::SceneType::DEFAULT);
        m_scene->setAmbient(glm::vec4(0.2f));
    }

    // scene camera
    {
        auto camera = m_scene->createPerspectiveCamera(m_window->getAspectRatio(), 60.0f, 0.1f, 60.0f);

        m_camController = aph::CameraController::Create(camera);
        m_camController->rotate({0.0f, 180.0f, 0.0f});
        m_camController->m_position      = {0.0f, 0.0f, -3.0f};
        m_camController->m_flipY         = true;
        m_camController->m_movementSpeed = {2.5f};
        m_camController->m_rotationSpeed = {0.1f};

        // camera 1 (main)
        m_cameraNode = m_scene->getRootNode()->createChildNode();
        m_cameraNode->attachObject<aph::Camera>(camera);
        m_scene->setMainCamera(camera);

        // // camera 2
        // m_scene->getRootNode()->createChildNode()->attachObject(camera);
    }

    // lights
    {
        auto dirLight         = m_scene->createLight();
        dirLight->m_color     = {1.0f, 1.0f, 1.0f};
        dirLight->m_direction = {0.2f, 1.0f, 0.3f};
        dirLight->m_type      = {aph::LightType::DIRECTIONAL};

        // light1
        m_directionalLightNode = m_scene->getRootNode()->createChildNode();
        m_directionalLightNode->attachObject<aph::Light>(dirLight);

        // #light 2
        auto pointLight        = m_scene->createLight();
        pointLight->m_color    = {1.0f, 0.7f, 0.7f};
        pointLight->m_position = {0.0f, 0.0f, 0.0f};
        pointLight->m_type     = {aph::LightType::POINT};

        m_pointLightNode = m_scene->getRootNode()->createChildNode();
        m_pointLightNode->attachObject<aph::Light>(pointLight);
    }

    // load from gltf file
    {
        if(modelPath) { m_modelNode = m_scene->createMeshesFromFile(modelPath); }
        else { m_modelNode = m_scene->createMeshesFromFile(aph::AssetManager::GetModelDir() / "DamagedHelmet.glb"); }
        m_modelNode->rotate(180.0f, {0.0f, 1.0f, 0.0f});

        auto model2 = m_scene->createMeshesFromFile(aph::AssetManager::GetModelDir() / "DamagedHelmet.glb");
        model2->rotate(180.0f, {0.0f, 1.0f, 0.0f});
        model2->translate({3.0, 1.0, 1.0});
    }

    {
        m_sceneRenderer->setScene(m_scene);
        m_sceneRenderer->setUIRenderer(m_uiRenderer);
        m_sceneRenderer->setShadingModel(aph::ShadingModel::PBR);
        m_sceneRenderer->loadResources();
    }
}

void scene_manager::setupRenderer()
{
    aph::RenderConfig config{
        .enableDebug = true,
        .enableUI    = true,
        .maxFrames   = 2,
        .sampleCount = aph::SAMPLE_COUNT_4_BIT,
    };

    m_sceneRenderer = aph::IRenderer::Create<aph::VulkanSceneRenderer>(m_window, config);
    m_uiRenderer    = std::make_unique<aph::VulkanUIRenderer>(m_sceneRenderer.get());
}

void scene_manager::keyboardHandleDerive(int key, int scancode, int action, int mods)
{
    using namespace aph;
    auto camera = m_cameraNode->getObject<aph::Camera>();
    if(action == APH_PRESS)
    {
        switch(key)
        {
        case APH_KEY_ESCAPE: m_window->close(); break;
        case APH_KEY_1: m_window->toggleCurosrVisibility(); break;
        case APH_KEY_W: m_camController->move(aph::Direction::UP, true); break;
        case APH_KEY_A: m_camController->move(aph::Direction::LEFT, true); break;
        case APH_KEY_S: m_camController->move(aph::Direction::DOWN, true); break;
        case APH_KEY_D: m_camController->move(aph::Direction::RIGHT, true); break;
        }
    }

    if(action == APH_RELEASE)
    {
        switch(key)
        {
        case APH_KEY_W: m_camController->move(aph::Direction::UP, false); break;
        case APH_KEY_A: m_camController->move(aph::Direction::LEFT, false); break;
        case APH_KEY_S: m_camController->move(aph::Direction::DOWN, false); break;
        case APH_KEY_D: m_camController->move(aph::Direction::RIGHT, false); break;
        }
    }
}

void scene_manager::mouseHandleDerive(double xposIn, double yposIn)
{
    const float dx = m_window->getCursorXpos() - xposIn;
    const float dy = m_window->getCursorYpos() - yposIn;

    auto camera = m_cameraNode->getObject<aph::Camera>();
    m_camController->rotate({dy, -dx, 0.0f});
}

int main(int argc, char** argv)
{
    scene_manager app;

    if(argc > 1) { modelPath = argv[1]; }

    app.init();
    app.run();
    app.finish();
}

#include "scene_manager.h"
#include <argparse/argparse.hpp>

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
        m_cameraController->update(deltaTime);

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
    m_uiRenderer->cleanup();
    m_sceneRenderer->cleanup();
}

void scene_manager::setupWindow()
{
    m_window = aph::Window::Create(m_options.windowWidth, m_options.windowHeight);

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
        auto camera        = m_scene->createPerspectiveCamera(m_window->getAspectRatio(), 60.0f, 0.1f, 60.0f);
        m_cameraController = aph::CameraController::Create(camera);

        // camera 1 (main)
        m_cameraNode = m_scene->getRootNode()->createChildNode();
        m_cameraNode->attachObject<aph::Camera>(camera);
        m_scene->setMainCamera(camera);


        // // camera 2
        // m_scene->getRootNode()->createChildNode()->attachObject(camera);
    }

    // lights
    {
        // light1
        auto dirLight          = m_scene->createDirLight({0.2f, 1.0f, 0.3f});
        m_directionalLightNode = m_scene->getRootNode()->createChildNode();
        m_directionalLightNode->attachObject<aph::Light>(dirLight);

        // #light 2
        auto pointLight  = m_scene->createPointLight({0.0f, 0.0f, 0.0f}, {1.0f, 0.7f, 0.7f});
        m_pointLightNode = m_scene->getRootNode()->createChildNode();
        m_pointLightNode->attachObject<aph::Light>(pointLight);
    }

    // load from gltf file
    {
        if(!m_options.modelPath.empty()) { m_modelNode = m_scene->createMeshesFromFile(m_options.modelPath); }
        else { m_modelNode = m_scene->createMeshesFromFile(aph::AssetManager::GetModelDir() / "DamagedHelmet.glb"); }
        m_modelNode->rotate(180.0f, {0.0f, 1.0f, 0.0f});

        auto* model2 = m_scene->createMeshesFromFile(aph::AssetManager::GetModelDir() / "DamagedHelmet.glb");
        model2->rotate(180.0f, {0.0f, 1.0f, 0.0f});
        model2->translate({3.0, 1.0, 1.0});
    }

    {
        m_sceneRenderer->load(m_scene.get());
    }
}

void scene_manager::setupRenderer()
{
    aph::RenderConfig config{
        .enableDebug = true,
        .enableUI    = true,
        .maxFrames   = 2,
    };

    m_sceneRenderer = aph::IRenderer::Create<aph::VulkanSceneRenderer>(m_window, config);
    m_uiRenderer    = std::make_unique<aph::VulkanUIRenderer>(m_sceneRenderer.get());
    m_sceneRenderer->setUIRenderer(m_uiRenderer);
}

void scene_manager::keyboardHandleDerive(int key, int scancode, int action, int mods)
{
    using namespace aph;
    if(action == aph::input::STATUS_PRESS)
    {
        switch(key)
        {
        case aph::input::KEY_ESCAPE: m_window->close(); break;
        case aph::input::KEY_W: m_cameraController->move(aph::Direction::UP, true); break;
        case aph::input::KEY_A: m_cameraController->move(aph::Direction::LEFT, true); break;
        case aph::input::KEY_S: m_cameraController->move(aph::Direction::DOWN, true); break;
        case aph::input::KEY_D: m_cameraController->move(aph::Direction::RIGHT, true); break;
        }
    }

    if(action == aph::input::STATUS_RELEASE)
    {
        switch(key)
        {
        case aph::input::KEY_W: m_cameraController->move(aph::Direction::UP, false); break;
        case aph::input::KEY_A: m_cameraController->move(aph::Direction::LEFT, false); break;
        case aph::input::KEY_S: m_cameraController->move(aph::Direction::DOWN, false); break;
        case aph::input::KEY_D: m_cameraController->move(aph::Direction::RIGHT, false); break;
        }
    }
}

void scene_manager::mouseHandleDerive(double xposIn, double yposIn)
{
    if(m_window->getMouseButtonStatus(aph::input::MOUSE_BUTTON_RIGHT) != aph::input::STATUS_PRESS)
    {
        m_window->setCursorVisibility(true);
        return;
    }

    m_window->setCursorVisibility(false);
    const float dx = m_window->getCursorX() - xposIn;
    const float dy = m_window->getCursorY() - yposIn;

    auto camera = m_cameraNode->getObject<aph::Camera>();
    m_cameraController->rotate({dy, -dx, 0.0f});
}

int main(int argc, char** argv)
{
    argparse::ArgumentParser program("program_name");

    program.add_argument("--width").help("window width").scan<'u', uint32_t>().default_value(1440U);
    program.add_argument("--height").help("window height").scan<'u', uint32_t>().default_value(900U);
    program.add_argument("--model").help("load model from files.").default_value("");
    try
    {
        program.parse_args(argc, argv);
    }
    catch(const std::runtime_error& err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    scene_manager app;

    app.m_options.modelPath    = program.get<std::string>("--model");
    app.m_options.windowWidth  = program.get<uint32_t>("--width");
    app.m_options.windowHeight = program.get<uint32_t>("--height");

    app.init();
    app.run();
    app.finish();
}

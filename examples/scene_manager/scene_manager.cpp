#include "scene_manager.h"
#include "renderer/renderer.h"
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
    while(m_window->update())
    {
        static float deltaTime = {};
        auto         timer     = aph::Timer(deltaTime);

        // update scene object
        m_modelNode->rotate(1.0f * deltaTime, {0.0f, 1.0f, 0.0f});
        m_cameraController->update(deltaTime);

        // update resource data
        m_sceneRenderer->update(deltaTime);

        // draw and submit
        m_sceneRenderer->beginFrame();
        m_sceneRenderer->recordAll();
        m_sceneRenderer->endFrame();
    }
}

void scene_manager::finish()
{
    m_sceneRenderer->getDevice()->waitIdle();
    m_sceneRenderer->cleanup();
}

void scene_manager::setupWindow()
{
    aph::Logger::Get()->info("init window: [%d, %d]", m_options.windowWidth, m_options.windowHeight);

    m_window = aph::WSI::Create(m_options.windowWidth, m_options.windowHeight);

    m_window->registerEventHandler<aph::MouseButtonEvent>([this](const aph::MouseButtonEvent& e){
        return onMouseBtn(e);
    });
    m_window->registerEventHandler<aph::KeyboardEvent>([this](const aph::KeyboardEvent& e){
        return onKeyDown(e);
    });
    m_window->registerEventHandler<aph::MouseMoveEvent>([this](const aph::MouseMoveEvent& e){
        return onMouseMove(e);
    });
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
        auto* camera       = m_scene->createPerspectiveCamera(m_window->getAspectRatio(), 60.0f, 0.1f, 60.0f);
        m_cameraController = aph::CameraController::Create(camera);

        // camera 1 (main)
        m_cameraNode = m_scene->getRootNode()->createChildNode();
        m_cameraNode->attachObject<aph::Camera>(camera);
        m_scene->setMainCamera(camera);
    }

    // lights
    {
        // light1
        auto* dirLight         = m_scene->createDirLight({0.2f, 1.0f, 0.3f});
        m_directionalLightNode = m_scene->getRootNode()->createChildNode();
        m_directionalLightNode->attachObject<aph::Light>(dirLight);

        // #light 2
        auto* pointLight = m_scene->createPointLight({0.0f, 0.0f, 0.0f}, {1.0f, 0.7f, 0.7f});
        m_pointLightNode = m_scene->getRootNode()->createChildNode();
        m_pointLightNode->attachObject<aph::Light>(pointLight);
    }

    // load from gltf file
    {
        if(!m_options.modelPath.empty()) { m_modelNode = m_scene->createMeshesFromFile(m_options.modelPath); }
        else { m_modelNode = m_scene->createMeshesFromFile(aph::AssetManager::GetModelDir() / "DamagedHelmet.glb"); }
        m_modelNode->rotate(180.0f, {0.0f, 1.0f, 0.0f});

        // auto* model2 = m_scene->createMeshesFromFile(aph::AssetManager::GetModelDir() / "DamagedHelmet.glb");
        // model2->rotate(180.0f, {0.0f, 1.0f, 0.0f});
        // model2->translate({3.0, 1.0, 1.0});
    }

    {
        m_sceneRenderer->load(m_scene.get());
    }
}

void scene_manager::setupRenderer()
{
    aph::RenderConfig config{
        .flags     = aph::RENDER_CFG_ALL,
        .maxFrames = 2,
    };

    aph::Logger::Get()->info("init renderer: max frames %d", config.maxFrames);
    m_sceneRenderer = aph::IRenderer::Create<aph::vk::SceneRenderer>(m_window, config);
}

bool scene_manager::onKeyDown(const aph::KeyboardEvent & event)
{
    using namespace aph;
    if(event.m_state == aph::KeyState::Pressed)
    {
        switch(event.m_key)
        {
        case Key::Escape: m_window->close(); break;
        case Key::W: m_cameraController->move(Direction::UP, true); break;
        case Key::A: m_cameraController->move(Direction::LEFT, true); break;
        case Key::S: m_cameraController->move(Direction::DOWN, true); break;
        case Key::D: m_cameraController->move(Direction::RIGHT, true); break;
        default: break;
        }
    }

    if(event.m_state == aph::KeyState::Released)
    {
        switch(event.m_key)
        {
        case Key::W: m_cameraController->move(Direction::UP, false); break;
        case Key::A: m_cameraController->move(Direction::LEFT, false); break;
        case Key::S: m_cameraController->move(Direction::DOWN, false); break;
        case Key::D: m_cameraController->move(Direction::RIGHT, false); break;
        default: break;
        }
    }

    return true;
}

bool scene_manager::onMouseBtn(const aph::MouseButtonEvent& event) {
    if (event.m_button == aph::MouseButton::Right)
    {
        m_cameraController->setCursorEnabled(event.m_pressed);
    }
    return true;
}

bool scene_manager::onMouseMove(const aph::MouseMoveEvent& event)
{
    m_cameraController->rotate({event.m_deltaY, -event.m_deltaX, 0.0f});
    return true;
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

#include "scene_manager.h"
#include "renderer/renderer.h"

scene_manager::scene_manager() : aph::BaseApp("scene_manager")
{
}

void scene_manager::init()
{
    setupWindow();
    setupRenderer();
    setupScene();
}

void scene_manager::run()
{
    while(m_wsi->update())
    {
        static float deltaTime = {};
        auto         timer     = aph::Timer(deltaTime);

        // update scene object
        m_modelNode->rotate(1.0f * deltaTime, {0.0f, 1.0f, 0.0f});
        m_cameraController->update(deltaTime);

        // update resource data
        m_renderer->update(deltaTime);

        // draw and submit
        m_renderer->beginFrame();
        m_renderer->recordAll();
        m_renderer->endFrame();
    }
}

void scene_manager::finish()
{
    m_renderer->getDevice()->waitIdle();
    m_renderer->cleanup();
}

void scene_manager::setupWindow()
{
    m_wsi = aph::WSI::Create(m_options.windowWidth, m_options.windowHeight);

    m_wsi->registerEventHandler<aph::MouseButtonEvent>(
        [this](const aph::MouseButtonEvent& e) { return onMouseBtn(e); });
    m_wsi->registerEventHandler<aph::KeyboardEvent>([this](const aph::KeyboardEvent& e) { return onKeyDown(e); });
    m_wsi->registerEventHandler<aph::MouseMoveEvent>([this](const aph::MouseMoveEvent& e) { return onMouseMove(e); });
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
        auto* camera       = m_scene->createPerspectiveCamera(m_wsi->getAspectRatio(), 60.0f, 0.1f, 60.0f);
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
        if(!m_options.modelPath.empty())
        {
            m_modelNode = m_scene->createMeshesFromFile(m_options.modelPath);
        }
        else
        {
            auto modelPath = aph::asset::GetModelDir() / "DamagedHelmet.glb";
            m_modelNode    = m_scene->createMeshesFromFile(modelPath);
        }
        m_modelNode->rotate(180.0f, {0.0f, 1.0f, 0.0f});

        auto* model2 = m_scene->createMeshesFromFile(aph::asset::GetModelDir() / "DamagedHelmet.glb");
        model2->rotate(180.0f, {0.0f, 1.0f, 0.0f});
        model2->translate({3.0, 1.0, 1.0});
    }

    {
        m_renderer->load(m_scene.get());
    }
}

void scene_manager::setupRenderer()
{
    aph::RenderConfig config{
        .flags     = aph::RENDER_CFG_ALL,
        .maxFrames = 1,
    };

    m_renderer = aph::IRenderer::Create<aph::vk::SceneRenderer>(m_wsi.get(), config);
}

bool scene_manager::onKeyDown(const aph::KeyboardEvent& event)
{
    using namespace aph;
    if(event.m_state == aph::KeyState::Pressed)
    {
        switch(event.m_key)
        {
        case Key::Escape:
            m_wsi->close();
            break;
        case Key::W:
            m_cameraController->move(Direction::UP, true);
            break;
        case Key::A:
            m_cameraController->move(Direction::LEFT, true);
            break;
        case Key::S:
            m_cameraController->move(Direction::DOWN, true);
            break;
        case Key::D:
            m_cameraController->move(Direction::RIGHT, true);
            break;
        default:
            break;
        }
    }

    if(event.m_state == aph::KeyState::Released)
    {
        switch(event.m_key)
        {
        case Key::W:
            m_cameraController->move(Direction::UP, false);
            break;
        case Key::A:
            m_cameraController->move(Direction::LEFT, false);
            break;
        case Key::S:
            m_cameraController->move(Direction::DOWN, false);
            break;
        case Key::D:
            m_cameraController->move(Direction::RIGHT, false);
            break;
        default:
            break;
        }
    }

    return true;
}

bool scene_manager::onMouseBtn(const aph::MouseButtonEvent& event)
{
    if(event.m_button == aph::MouseButton::Right)
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
    scene_manager app;

    // parse command
    {
        int               exitCode;
        aph::CLICallbacks cbs;
        cbs.add("--width", [&](aph::CLIParser& parser) { app.m_options.windowWidth = parser.nextUint(); });
        cbs.add("--height", [&](aph::CLIParser& parser) { app.m_options.windowHeight = parser.nextUint(); });
        cbs.add("--model", [&](aph::CLIParser& parser) { app.m_options.modelPath = parser.nextString(); });
        cbs.m_errorHandler = [&]() { CM_LOG_ERR("Failed to parse CLI arguments."); };
        if(!aph::parseCliFiltered(std::move(cbs), argc, argv, exitCode))
        {
            return exitCode;
        }
    }

    try
    {
        app.init();
        app.run();
        app.finish();
    }
    catch(const aph::TracedException& ex)
    {
        CM_LOG_ERR("%s\n", ex.what());
    }
}

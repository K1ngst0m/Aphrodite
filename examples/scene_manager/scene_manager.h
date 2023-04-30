#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

#include "aph_core.hpp"
#include "aph_renderer.hpp"
#include "renderer/api/vulkan/sceneRenderer.h"

class scene_manager : public aph::BaseApp
{
public:
    scene_manager();

    void init() override;
    void run() override;
    void finish() override;

    struct
    {
        std::string modelPath    = {};
        uint32_t    windowWidth  = {};
        uint32_t    windowHeight = {};
    } m_options;

private:
    void setupWindow();
    void setupRenderer();
    void setupScene();

    bool onKeyDown(const aph::KeyboardEvent & event);
    bool onMouseMove(const aph::MouseMoveEvent & event);
    bool onMouseBtn(const aph::MouseButtonEvent& event);

private:
    std::shared_ptr<aph::Scene> m_scene                = {};
    aph::SceneNode*             m_modelNode            = {};
    aph::SceneNode*             m_pointLightNode       = {};
    aph::SceneNode*             m_directionalLightNode = {};
    aph::SceneNode*             m_cameraNode           = {};

    std::unique_ptr<aph::CameraController> m_cameraController = {};

    std::unique_ptr<aph::vk::SceneRenderer> m_sceneRenderer = {};

    std::shared_ptr<aph::WSI> m_window = {};
};

#endif  // SCENE_MANAGER_H_

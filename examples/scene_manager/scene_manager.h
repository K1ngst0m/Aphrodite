#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

#include "aph_core.hpp"
#include "aph_renderer.hpp"

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

    void keyboardHandleDerive(int key, int scancode, int action, int mods);
    void mouseHandleDerive(double xposIn, double yposIn);

private:
    std::shared_ptr<aph::Scene> m_scene                = {};
    aph::SceneNode*             m_modelNode            = {};
    aph::SceneNode*             m_pointLightNode       = {};
    aph::SceneNode*             m_directionalLightNode = {};
    aph::SceneNode*             m_cameraNode           = {};

    std::unique_ptr<aph::CameraController> m_cameraController = {};

    std::unique_ptr<aph::VulkanSceneRenderer> m_sceneRenderer = {};
    std::unique_ptr<aph::VulkanUIRenderer>    m_uiRenderer    = {};

    std::shared_ptr<aph::Window> m_window = {};
};

#endif  // SCENE_MANAGER_H_

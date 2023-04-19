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

private:
    void setupWindow();
    void setupRenderer();

    void keyboardHandleDerive(int key, int scancode, int action, int mods);
    void mouseHandleDerive(double xposIn, double yposIn);

    void setupScene();

private:
    std::shared_ptr<aph::SceneNode> m_modelNode            = {};
    std::shared_ptr<aph::SceneNode> m_pointLightNode       = {};
    std::shared_ptr<aph::SceneNode> m_directionalLightNode = {};
    std::shared_ptr<aph::SceneNode> m_cameraNode           = {};

    std::unique_ptr<aph::VulkanSceneRenderer> m_sceneRenderer = {};
    std::unique_ptr<aph::VulkanUIRenderer>    m_uiRenderer    = {};

    std::shared_ptr<aph::Scene>  m_scene  = {};
    std::shared_ptr<aph::Window> m_window = {};
};

#endif  // SCENE_MANAGER_H_

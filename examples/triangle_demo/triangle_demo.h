#ifndef SCENE_MANAGER_H_
#define SCENE_MANAGER_H_

#include "aph_core.hpp"
#include "aph_renderer.hpp"

class triangle_demo : public aph::BaseApp
{
public:
    triangle_demo();

    void init() override;
    void load() override;
    void run() override;
    void unload() override;
    void finish() override;

private:
    aph::vk::Buffer*        m_pVB      = {};
    aph::vk::Buffer*        m_pIB      = {};
    aph::vk::ShaderProgram* m_pProgram = {};

private:
    std::unique_ptr<aph::vk::Renderer> m_renderer        = {};
    aph::WSI*                          m_pWSI            = {};
    aph::vk::Device*                   m_pDevice         = {};
    aph::ResourceLoader*               m_pResourceLoader = {};
    aph::vk::SwapChain*                m_pSwapChain      = {};

private:
    aph::Timer m_timer;
};

#endif  // SCENE_MANAGER_H_

#ifndef BASE_TEXTURE_H_
#define BASE_TEXTURE_H_

#include "aph_core.hpp"
#include "aph_renderer.hpp"

class basic_texture : public aph::BaseApp
{
public:
    basic_texture();

    void init() override;
    void load() override;
    void run() override;
    void unload() override;
    void finish() override;

    struct
    {
        uint32_t windowWidth  = {800};
        uint32_t windowHeight = {800};
    } m_options;

private:
    aph::vk::Buffer*        m_pVB        = {};
    aph::vk::Buffer*        m_pIB        = {};
    aph::vk::Buffer*        m_pMatBuffer = {};
    aph::vk::Sampler*       m_pSampler   = {};
    aph::vk::Image*         m_pImage     = {};
    aph::vk::ShaderProgram* m_pProgram   = {};

private:
    std::unique_ptr<aph::vk::Renderer> m_renderer        = {};
    aph::WSI*                          m_pWSI            = {};
    aph::vk::Device*                   m_pDevice         = {};
    aph::ResourceLoader*               m_pResourceLoader = {};
    aph::vk::SwapChain*                m_pSwapChain      = {};

    glm::mat4 m_modelMatrix{1.0f};
private:
    aph::Timer m_timer;
};

#endif  // SCENE_MANAGER_H_

#ifndef BASE_TEXTURE_H_
#define BASE_TEXTURE_H_

#include "aph_core.hpp"
#include "aph_renderer.hpp"

class basic_texture : public aph::BaseApp
{
public:
    basic_texture();

    void init() override;
    void run() override;
    void finish() override;

    struct
    {
        uint32_t windowWidth  = {1440};
        uint32_t windowHeight = {900};
    } m_options;

private:
    aph::vk::Pipeline*      m_pPipeline  = {};
    aph::vk::Buffer*        m_pVB        = {};
    aph::vk::Buffer*        m_pIB        = {};
    aph::vk::Sampler*       m_pSampler   = {};
    aph::vk::Image*         m_pImage     = {};
    aph::vk::DescriptorSet* m_pTextureSet = {};

private:
    std::unique_ptr<aph::WSI>          m_wsi      = {};
    std::unique_ptr<aph::vk::Renderer> m_renderer = {};
    aph::vk::Device*                   m_pDevice  = {};
};

#endif  // SCENE_MANAGER_H_

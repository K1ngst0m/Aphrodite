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

    struct
    {
        uint32_t windowWidth  = {1440};
        uint32_t windowHeight = {900};
    } m_options;

private:
    aph::vk::Pipeline*      m_pPipeline      = {};
    aph::vk::Buffer*        m_pVB            = {};
    aph::vk::Buffer*        m_pIB            = {};

private:
    std::unique_ptr<aph::WSI>          m_wsi      = {};
    std::unique_ptr<aph::vk::Renderer> m_renderer = {};
    aph::vk::Device* m_pDevice = {};
};

#endif  // SCENE_MANAGER_H_

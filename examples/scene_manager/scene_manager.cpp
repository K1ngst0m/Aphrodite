#include "scene_manager.h"

void scene_manager::drawFrame() {
    float currentFrame    = glfwGetTime();
    m_frameData.deltaTime = currentFrame - m_frameData.lastFrame;
    m_frameData.lastFrame = currentFrame;

    renderer->prepareFrame();
    renderer->submitFrame();
    updateUniformBuffer();
}

void scene_manager::updateUniformBuffer() {
    m_sceneManager.update();
    m_sceneRenderer[renderer->m_imageIdx]->update();
}

void scene_manager::initDerive() {
    loadScene();
    buildCommands();
}

void scene_manager::loadScene() {
    // scene global argument setup
    {
        m_sceneManager.setAmbient(glm::vec4(0.2f));
    }

    {
        m_camera = m_sceneManager.createCamera((float)m_windowData.width / m_windowData.height);
        m_camera->setType(vkl::CameraType::FIRSTPERSON);
        m_camera->setPosition({0.0f, 1.0f, 3.0f, 1.0f});
        m_camera->setPerspective(60.0f, (float)m_windowData.width / (float)m_windowData.height, 0.1f, 256.0f);
        m_camera->setMovementSpeed(2.5f);
        m_camera->setRotationSpeed(0.1f);

        auto *node = m_sceneManager.getRootNode()->createChildNode();
        node->attachObject(m_camera);
    }

    {
        m_pointLight = m_sceneManager.createLight();
        m_pointLight->setPosition({1.2f, 1.0f, 2.0f, 1.0f});
        m_pointLight->setDiffuse({0.5f, 0.5f, 0.5f, 1.0f});
        m_pointLight->setSpecular({1.0f, 1.0f, 1.0f, 1.0f});
        m_pointLight->setType(vkl::LightType::POINT);

        auto *node = m_sceneManager.getRootNode()->createChildNode();
        node->attachObject(m_pointLight);
    }

    {
        m_directionalLight = m_sceneManager.createLight();
        m_directionalLight->setDirection({-0.2f, -1.0f, -0.3f, 1.0f});
        m_directionalLight->setDiffuse({0.5f, 0.5f, 0.5f, 1.0f});
        m_directionalLight->setSpecular({1.0f, 1.0f, 1.0f, 1.0f});
        m_directionalLight->setType(vkl::LightType::DIRECTIONAL);

        auto *node = m_sceneManager.getRootNode()->createChildNode();
        node->attachObject(m_directionalLight);
    }

    {
        m_model                  = m_sceneManager.createEntity(modelDir / "Sponza/glTF/Sponza.gltf");
        auto *node = m_sceneManager.getRootNode()->createChildNode();
        node->attachObject(m_model);
    }

    {
        // glm::mat4 modelTransform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
        // modelTransform           = glm::rotate(modelTransform, 3.14f, glm::vec3(0.0f, 1.0f, 0.0f));
        // auto helmet_model        = m_sceneManager.createEntity(modelDir / "FlightHelmet/glTF/FlightHelmet.gltf");

        // auto *node = m_sceneManager.getRootNode()->createChildNode(modelTransform);
        // node->attachObject(helmet_model);
    }

    {
        m_sceneRenderer.resize(renderer->m_commandBuffers.size());
        for (size_t i = 0; i < m_sceneRenderer.size(); i++) {
            m_sceneRenderer[i] = std::make_shared<vkl::VulkanSceneRenderer>(&m_sceneManager, renderer->m_commandBuffers[i], renderer->m_device, renderer->m_queues.graphics, renderer->m_queues.transfer);
            m_sceneRenderer[i]->setRenderer(renderer.get());
            m_sceneRenderer[i]->loadResources();
        }
    }

    m_deletionQueue.push_function([&]() {
        for (auto &sceneRenderer : m_sceneRenderer) {
            sceneRenderer->cleanupResources();
        }
    });
}

void scene_manager::buildCommands() {
    for (uint32_t idx = 0; idx < renderer->m_commandBuffers.size(); idx++) {
        renderer->recordCommandBuffer(&m_windowData, [&](VkCommandBuffer commandBuffer) {
            m_sceneRenderer[idx]->drawScene();
        }, idx);
    }
}

int main() {
    scene_manager app;

    app.vkl::vklApp::init();
    app.vkl::vklApp::run();
    app.vkl::vklApp::finish();
}

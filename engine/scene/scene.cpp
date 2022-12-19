#include "scene.h"
#include "camera.h"
#include "entity.h"
#include "light.h"
#include "common/assetManager.h"

namespace vkl
{
std::unique_ptr<Scene> Scene::Create(SceneManagerType type)
{
    switch(type)
    {
    case SceneManagerType::DEFAULT:
    {
        auto instance = std::make_unique<Scene>();
        instance->m_rootNode = std::make_shared<SceneNode>(nullptr);
        return instance;
    }
    default:
    {
        assert("scene manager type not support.");
        return {};
    }
    }
}

std::shared_ptr<Camera> Scene::createCamera(float aspectRatio)
{
    auto camera = Object::Create<Camera>();
    camera->setAspectRatio(aspectRatio);
    m_cameras[camera->getId()] = camera;
    return camera;
}

std::shared_ptr<Light> Scene::createLight()
{
    auto light = Object::Create<Light>();
    m_lights[light->getId()] = light;
    return light;
}

std::shared_ptr<Entity> Scene::createEntity()
{
    auto entity = Object::Create<Entity>();
    m_entities[entity->getId()] = entity;
    return entity;
}

std::shared_ptr<Mesh> Scene::createMesh()
{
    auto mesh = Object::Create<Mesh>();
    m_meshes[mesh->getId()] = mesh;
    return mesh;
}
}  // namespace vkl

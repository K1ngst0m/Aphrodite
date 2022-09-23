#ifndef ENTITYMANUALLOADER_H_
#define ENTITYMANUALLOADER_H_

#include "entity.h"

namespace vkl {
class EntityManualLoader : public EntityLoader{
public:
    EntityManualLoader(Entity *entity);
    void load() override;

private:
    std::vector<VertexLayout> _vertices;
    std::vector<uint32_t>     _indices;
    std::vector<Image *>      _images;
    std::vector<SubEntity *>  _subEntityList;
    std::vector<Material>     _materials;
};
}

#endif // ENTITYMANUALLOADER_H_

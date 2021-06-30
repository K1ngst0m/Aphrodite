//
// Created by npchitman on 6/29/21.
//

#ifndef Aphrodite_ENGINE_UNIFORMBUFFER_H
#define Aphrodite_ENGINE_UNIFORMBUFFER_H

#include "Aphrodite/Core/Base.h"

namespace Aph {
    class UniformBuffer {
    public:
        virtual ~UniformBuffer() = default;

        virtual void SetData(const void* data, uint32_t size, uint32_t offset/* = 0*/) = 0;

        static Ref<UniformBuffer> Create(uint32_t size, uint32_t binding);
    };
}


#endif//Aphrodite_ENGINE_UNIFORMBUFFER_H

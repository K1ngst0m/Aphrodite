#include "passResource.h"

namespace aph
{

PassResource::PassResource(Type type)
    : m_type(type)
{
}

void PassResource::addWritePass(RenderPass* pPass)
{
    m_writePasses.insert(pPass);
}

void PassResource::addReadPass(RenderPass* pPass)
{
    m_readPasses.insert(pPass);
}

void PassResource::addAccessFlags(::vk::AccessFlagBits2 flag)
{
    m_accessFlags |= flag;
}

void PassResource::addFlags(PassResourceFlags flag)
{
    m_flags |= flag;
}

auto PassResource::getReadPasses() const -> const HashSet<RenderPass*>&
{
    return m_readPasses;
}

auto PassResource::getWritePasses() const -> const HashSet<RenderPass*>&
{
    return m_writePasses;
}

auto PassResource::getType() const -> PassResource::Type
{
    return m_type;
}

auto PassResource::getFlags() const -> PassResourceFlags
{
    return m_flags;
}

auto PassResource::getAccessFlags() const -> ::vk::AccessFlags2
{
    return m_accessFlags;
}

auto PassResource::getName() const -> const std::string&
{
    return m_name;
}

void PassResource::setName(std::string name)
{
    m_name = std::move(name);
}

PassImageResource::PassImageResource(Type type)
    : PassResource(type)
{
}

void PassImageResource::setInfo(const RenderPassAttachmentInfo& info)
{
    m_info = info;
}

void PassImageResource::addUsage(ImageUsageFlags usage)
{
    m_usage |= usage;
}

auto PassImageResource::getInfo() const -> const RenderPassAttachmentInfo&
{
    return m_info;
}

auto PassImageResource::getUsage() const -> ImageUsageFlags
{
    return m_usage;
}

PassBufferResource::PassBufferResource(Type type)
    : PassResource(type)
{
}

void PassBufferResource::addInfo(const vk::BufferCreateInfo& info)
{
    m_info = info;
}

void PassBufferResource::addUsage(BufferUsageFlags usage)
{
    m_usage |= usage;
}

auto PassBufferResource::getInfo() const -> const vk::BufferCreateInfo&
{
    return m_info;
}

auto PassBufferResource::getUsage() const -> BufferUsageFlags
{
    return m_usage;
}
} // namespace aph

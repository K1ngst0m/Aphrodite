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
const HashSet<RenderPass*>& PassResource::getReadPasses() const
{
    return m_readPasses;
}
const HashSet<RenderPass*>& PassResource::getWritePasses() const
{
    return m_writePasses;
}
PassResource::Type PassResource::getType() const
{
    return m_type;
}
PassResourceFlags PassResource::getFlags() const
{
    return m_flags;
}
::vk::AccessFlags2 PassResource::getAccessFlags() const
{
    return m_accessFlags;
}
const std::string& PassResource::getName() const
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
const RenderPassAttachmentInfo& PassImageResource::getInfo() const
{
    return m_info;
}
ImageUsageFlags PassImageResource::getUsage() const
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
const vk::BufferCreateInfo& PassBufferResource::getInfo() const
{
    return m_info;
}
BufferUsageFlags PassBufferResource::getUsage() const
{
    return m_usage;
}
} // namespace aph

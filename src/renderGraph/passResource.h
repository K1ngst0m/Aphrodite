#pragma once

#include "api/vulkan/device.h"
#include "common/enum.h"

namespace aph
{
class RenderPass;
enum class PassResourceFlagBits
{
    None = 0,
    External = (1 << 0),
    Shared = (1 << 1), // Resource is shared across frames
};
using PassResourceFlags = Flags<PassResourceFlagBits>;

class PassResource
{
public:
    virtual ~PassResource() = default;

    enum class Type
    {
        Image,
        Buffer,
    };

    PassResource(Type type);

    void addWritePass(RenderPass* pPass);
    void addReadPass(RenderPass* pPass);
    void addAccessFlags(::vk::AccessFlagBits2 flag);
    void addFlags(PassResourceFlags flag);

    const HashSet<RenderPass*>& getReadPasses() const;
    const HashSet<RenderPass*>& getWritePasses() const;

    Type getType() const;
    PassResourceFlags getFlags() const;
    ::vk::AccessFlags2 getAccessFlags() const;

    const std::string& getName() const;

    void setName(std::string name);

protected:
    Type m_type;
    HashSet<RenderPass*> m_writePasses;
    HashSet<RenderPass*> m_readPasses;
    ::vk::AccessFlags2 m_accessFlags = {};
    PassResourceFlags m_flags = PassResourceFlagBits::None;
    std::string m_name;
};

struct RenderPassAttachmentInfo
{
    vk::ImageCreateInfo createInfo = {};
    vk::AttachmentInfo attachmentInfo = {};
};

class PassImageResource : public PassResource
{
public:
    PassImageResource(Type type);
    void setInfo(const RenderPassAttachmentInfo& info);
    void addUsage(ImageUsageFlags usage);

    const RenderPassAttachmentInfo& getInfo() const;
    ImageUsageFlags getUsage() const;

private:
    RenderPassAttachmentInfo m_info;
    ImageUsageFlags m_usage = {};
};

class PassBufferResource : public PassResource
{
public:
    PassBufferResource(Type type);
    void addInfo(const vk::BufferCreateInfo& info);
    void addUsage(BufferUsageFlags usage);

    const vk::BufferCreateInfo& getInfo() const;
    BufferUsageFlags getUsage() const;

private:
    vk::BufferCreateInfo m_info;
    BufferUsageFlags m_usage;
};

} // namespace aph

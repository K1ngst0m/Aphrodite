#pragma once

#include "api/vulkan/device.h"
#include "common/enum.h"

namespace aph
{
class RenderPass;
enum class PassResourceFlagBits : uint8_t
{
    eNone     = 0,
    eExternal = (1 << 0),
    eShared   = (1 << 1), // Resource is shared across frames
};
using PassResourceFlags = Flags<PassResourceFlagBits>;

class PassResource
{
public:
    virtual ~PassResource() = default;

    enum class Type : uint8_t
    {
        eImage,
        eBuffer,
    };

    explicit PassResource(Type type);

    void addWritePass(RenderPass* pPass);
    void addReadPass(RenderPass* pPass);
    void addAccessFlags(::vk::AccessFlagBits2 flag);
    void addFlags(PassResourceFlags flag);

    auto getReadPasses() const -> const HashSet<RenderPass*>&;
    auto getWritePasses() const -> const HashSet<RenderPass*>&;

    auto getType() const -> Type;
    auto getFlags() const -> PassResourceFlags;
    auto getAccessFlags() const -> ::vk::AccessFlags2;

    auto getName() const -> const std::string&;

    void setName(std::string name);

protected:
    Type m_type;
    HashSet<RenderPass*> m_writePasses;
    HashSet<RenderPass*> m_readPasses;
    ::vk::AccessFlags2 m_accessFlags = {};
    PassResourceFlags m_flags        = PassResourceFlagBits::eNone;
    std::string m_name;
};

struct RenderPassAttachmentInfo
{
    vk::ImageCreateInfo createInfo    = {};
    vk::AttachmentInfo attachmentInfo = {};
};

class PassImageResource : public PassResource
{
public:
    explicit PassImageResource(Type type);
    void setInfo(const RenderPassAttachmentInfo& info);
    void addUsage(ImageUsageFlags usage);

    auto getInfo() const -> const RenderPassAttachmentInfo&;
    auto getUsage() const -> ImageUsageFlags;

private:
    RenderPassAttachmentInfo m_info;
    ImageUsageFlags m_usage = {};
};

class PassBufferResource : public PassResource
{
public:
    explicit PassBufferResource(Type type);
    void addInfo(const vk::BufferCreateInfo& info);
    void addUsage(BufferUsageFlags usage);

    auto getInfo() const -> const vk::BufferCreateInfo&;
    auto getUsage() const -> BufferUsageFlags;

private:
    vk::BufferCreateInfo m_info;
    BufferUsageFlags m_usage;
};

} // namespace aph

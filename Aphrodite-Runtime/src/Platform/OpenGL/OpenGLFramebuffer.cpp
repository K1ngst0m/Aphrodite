//
// Created by npchitman on 6/24/21.
//

#include "Platform/OpenGL/OpenGLFramebuffer.h"

#include <glad/glad.h>

#include <utility>

#include "pch.h"

namespace Aph {
    static const uint32_t s_MaxFramebufferSize = 8192;

    namespace Utils {
        static GLenum TextureTarget(bool multiSampled) {
            return multiSampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
        }

        static void CreateTextures(bool multiSampled,
                                   uint32_t* outID,
                                   uint32_t count) {
            glCreateTextures(TextureTarget(multiSampled), static_cast<GLsizei>(count), outID);
        }

        static void BindTexture(bool multiSampled,
                                uint32_t id) {
            glBindTexture(TextureTarget(multiSampled), id);
        }

        static void AttachColorTexture(uint32_t id,
                                       int samples,
                                       GLint internalFormat,
                                       GLenum format,
                                       uint32_t width, uint32_t height,
                                       int index) {
            bool multisampled = samples > 1;
            if (multisampled) {
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
                                        samples, internalFormat,
                                        static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                                        GL_FALSE);
            } else {
                glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
                             static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, format, GL_UNSIGNED_BYTE, nullptr);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget(multisampled), id, 0);
        }

        static void AttachDepthTexture(uint32_t id, uint32_t samples, GLenum format, GLenum attachmentType, uint32_t width, uint32_t height) {
            bool multisampled = samples > 1;
            if (multisampled) {
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
            } else {
                glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }

            glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, TextureTarget(multisampled), id, 0);
        }

        static bool IsDepthFormat(FramebufferTextureFormat format) {
            switch (format) {
                case FramebufferTextureFormat::DEPTH24STENCIL8:
                    return true;
                case FramebufferTextureFormat::None:
                case FramebufferTextureFormat::RGBA8:
                case FramebufferTextureFormat::RED_INTEGER:
                default:
                    return false;
            }
        }

        static GLenum AphroditeFBTextureFormatToGL(FramebufferTextureFormat format) {
            switch (format) {
                case FramebufferTextureFormat::RGBA8:
                    return GL_RGBA8;
                case FramebufferTextureFormat::RED_INTEGER:
                    return GL_RED_INTEGER;
                case FramebufferTextureFormat::None:
                case FramebufferTextureFormat::DEPTH24STENCIL8:
                default:
                    APH_CORE_ASSERT(false);
                    return 0;
            }
        }
    }// namespace Utils

    OpenGLFramebuffer::OpenGLFramebuffer(FramebufferSpecification spec)
        : m_Specification(std::move(spec)) {
        for (auto spec_item : m_Specification.Attachments.Attachments) {
            if (!Utils::IsDepthFormat(spec_item.TextureFormat))
                m_ColorAttachmentSpecifications.emplace_back(spec_item);
            else
                m_DepthAttachmentSpecification = spec_item;
        }
        Invalidate();
    }

    OpenGLFramebuffer::~OpenGLFramebuffer() {
        glDeleteFramebuffers(1, &m_RendererID);
        glDeleteTextures(static_cast<GLsizei>(m_ColorAttachments.size()), m_ColorAttachments.data());
        glDeleteTextures(1, &m_DepthAttachment);
    }

    void OpenGLFramebuffer::Invalidate() {
        if (m_RendererID) {
            glDeleteFramebuffers(1, &m_RendererID);
            glDeleteTextures(static_cast<GLsizei>(m_ColorAttachments.size()), m_ColorAttachments.data());
            glDeleteTextures(1, &m_DepthAttachment);
        }

        glCreateFramebuffers(1, &m_RendererID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

        bool multisample = m_Specification.Samples > 1;

        // Attachments
        if (!m_ColorAttachmentSpecifications.empty()) {
            m_ColorAttachments.resize(m_ColorAttachmentSpecifications.size());
            Utils::CreateTextures(multisample, m_ColorAttachments.data(), m_ColorAttachments.size());

            for (size_t i = 0; i < m_ColorAttachments.size(); i++) {
                Utils::BindTexture(multisample, m_ColorAttachments[i]);
                switch (m_ColorAttachmentSpecifications[i].TextureFormat) {
                    case FramebufferTextureFormat::RGBA8:
                        Utils::AttachColorTexture(m_ColorAttachments[i], static_cast<int>(m_Specification.Samples), GL_RGBA8, GL_RGBA, m_Specification.Width, m_Specification.Height, static_cast<int>(i));
                        break;
                    case FramebufferTextureFormat::None:
                    case FramebufferTextureFormat::DEPTH24STENCIL8:
                        break;
                    case FramebufferTextureFormat::RED_INTEGER:
                        Utils::AttachColorTexture(m_ColorAttachments[i], static_cast<int>(m_Specification.Samples), GL_R32I, GL_RED_INTEGER, m_Specification.Width, m_Specification.Height, i);
                        break;
                }
            }
        }

        if (m_DepthAttachmentSpecification.TextureFormat != FramebufferTextureFormat::None) {
            Utils::CreateTextures(multisample, &m_DepthAttachment, 1);
            Utils::BindTexture(multisample, m_DepthAttachment);
            switch (m_DepthAttachmentSpecification.TextureFormat) {
                case FramebufferTextureFormat::DEPTH24STENCIL8:
                    Utils::AttachDepthTexture(m_DepthAttachment, m_Specification.Samples,
                                              GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT,
                                              m_Specification.Width, m_Specification.Height);
                    break;
                case FramebufferTextureFormat::None:
                case FramebufferTextureFormat::RGBA8:
                case FramebufferTextureFormat::RED_INTEGER:
                    break;
            }
        }

        if (m_ColorAttachments.size() > 1) {
            APH_CORE_ASSERT(m_ColorAttachments.size() <= 4);
            GLenum buffers[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
            glDrawBuffers(static_cast<GLsizei>(m_ColorAttachments.size()), buffers);
        } else if (m_ColorAttachments.empty()) {
            // Only depth-pass
            glDrawBuffer(GL_NONE);
        }


        APH_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::Bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
        glViewport(0, 0, static_cast<int>(m_Specification.Width), static_cast<int>(m_Specification.Height));
    }

    void OpenGLFramebuffer::UnBind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize) {
            APH_CORE_WARN("Attempted to resize framebuffer to {0}, {1}", width, height);
            return;
        }

        m_Specification.Width = width;
        m_Specification.Height = height;

        Invalidate();
    }

    int OpenGLFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y) {
        APH_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size());

        glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
        int pixelData;
        glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
        return pixelData;
    }

    void OpenGLFramebuffer::ClearAttachment(uint32_t attachmentIndex, int value) {
        APH_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size());

        auto& spec = m_ColorAttachmentSpecifications[attachmentIndex];
        glClearTexImage(m_ColorAttachments[attachmentIndex], 0, Utils::AphroditeFBTextureFormatToGL(spec.TextureFormat), GL_INT, &value);
    }


}// namespace Aph
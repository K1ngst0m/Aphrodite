#ifndef GPU_RESOURCE_H_
#define GPU_RESOURCE_H_

#include "common/common.h"

namespace aph
{
#define MAX_DESCRIPTION_SIZE 256U

enum BaseType
{
    BASE_TYPE_BOOL   = 0,
    BASE_TYPE_CHAR   = 1,
    BASE_TYPE_INT    = 2,
    BASE_TYPE_UINT   = 3,
    BASE_TYPE_UINT64 = 4,
    BASE_TYPE_HALF   = 5,
    BASE_TYPE_FLOAT  = 6,
    BASE_TYPE_DOUBLE = 7,
    BASE_TYPE_STRUCT = 8,
};

enum PipelineResourceType
{
    PIPELINE_RESOURCE_TYPE_INPUT                  = 0,
    PIPELINE_RESOURCE_TYPE_OUTPUT                 = 1,
    PIPELINE_RESOURCE_TYPE_SAMPLER                = 2,
    PIPELINE_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER = 3,
    PIPELINE_RESOURCE_TYPE_SAMPLED_IMAGE          = 4,
    PIPELINE_RESOURCE_TYPE_STORAGE_IMAGE          = 5,
    PIPELINE_RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER   = 6,
    PIPELINE_RESOURCE_TYPE_STORAGE_TEXEL_BUFFER   = 7,
    PIPELINE_RESOURCE_TYPE_UNIFORM_BUFFER         = 8,
    PIPELINE_RESOURCE_TYPE_STORAGE_BUFFER         = 9,
    PIPELINE_RESOURCE_TYPE_INPUT_ATTACHMENT       = 10,
    PIPELINE_RESOURCE_TYPE_PUSH_CONSTANT_BUFFER   = 11,
};

enum BufferUsageFlagBits
{
    BUFFER_USAGE_TRANSFER_SRC_BIT          = 0x00000001,
    BUFFER_USAGE_TRANSFER_DST_BIT          = 0x00000002,
    BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT  = 0x00000004,
    BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT  = 0x00000008,
    BUFFER_USAGE_UNIFORM_BUFFER_BIT        = 0x00000010,
    BUFFER_USAGE_STORAGE_BUFFER_BIT        = 0x00000020,
    BUFFER_USAGE_INDEX_BUFFER_BIT          = 0x00000040,
    BUFFER_USAGE_VERTEX_BUFFER_BIT         = 0x00000080,
    BUFFER_USAGE_INDIRECT_BUFFER_BIT       = 0x00000100,
    BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT = 0x00020000,
    BUFFER_USAGE_FLAG_BITS_MAX_ENUM        = 0x7FFFFFFF
};
using BufferUsageFlags = uint32_t;

enum ImageUsageFlagBits
{
    IMAGE_USAGE_TRANSFER_SRC_BIT             = 0x00000001,
    IMAGE_USAGE_TRANSFER_DST_BIT             = 0x00000002,
    IMAGE_USAGE_SAMPLED_BIT                  = 0x00000004,
    IMAGE_USAGE_STORAGE_BIT                  = 0x00000008,
    IMAGE_USAGE_COLOR_ATTACHMENT_BIT         = 0x00000010,
    IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000020,
    IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT     = 0x00000040,
    IMAGE_USAGE_INPUT_ATTACHMENT_BIT         = 0x00000080,
    IMAGE_USAGE_FLAG_BITS_MAX_ENUM           = 0x7FFFFFFF
};
using ImageUsageFlags = uint32_t;

enum MemoryPropertyFlagBits
{
    MEMORY_PROPERTY_DEVICE_LOCAL_BIT        = 0x00000001,
    MEMORY_PROPERTY_HOST_VISIBLE_BIT        = 0x00000002,
    MEMORY_PROPERTY_HOST_COHERENT_BIT       = 0x00000004,
    MEMORY_PROPERTY_HOST_CACHED_BIT         = 0x00000008,
    MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT    = 0x00000010,
    MEMORY_PROPERTY_PROTECTED_BIT           = 0x00000020,
    MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD = 0x00000040,
    MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD = 0x00000080,
    MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV     = 0x00000100,
    MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM      = 0x7FFFFFFF
};
using MemoryPropertyFlags = uint32_t;

enum Format
{
    FORMAT_UNDEFINED                                      = 0,
    FORMAT_R4G4_UNORM_PACK8                               = 1,
    FORMAT_R4G4B4A4_UNORM_PACK16                          = 2,
    FORMAT_B4G4R4A4_UNORM_PACK16                          = 3,
    FORMAT_R5G6B5_UNORM_PACK16                            = 4,
    FORMAT_B5G6R5_UNORM_PACK16                            = 5,
    FORMAT_R5G5B5A1_UNORM_PACK16                          = 6,
    FORMAT_B5G5R5A1_UNORM_PACK16                          = 7,
    FORMAT_A1R5G5B5_UNORM_PACK16                          = 8,
    FORMAT_R8_UNORM                                       = 9,
    FORMAT_R8_SNORM                                       = 10,
    FORMAT_R8_USCALED                                     = 11,
    FORMAT_R8_SSCALED                                     = 12,
    FORMAT_R8_UINT                                        = 13,
    FORMAT_R8_SINT                                        = 14,
    FORMAT_R8_SRGB                                        = 15,
    FORMAT_R8G8_UNORM                                     = 16,
    FORMAT_R8G8_SNORM                                     = 17,
    FORMAT_R8G8_USCALED                                   = 18,
    FORMAT_R8G8_SSCALED                                   = 19,
    FORMAT_R8G8_UINT                                      = 20,
    FORMAT_R8G8_SINT                                      = 21,
    FORMAT_R8G8_SRGB                                      = 22,
    FORMAT_R8G8B8_UNORM                                   = 23,
    FORMAT_R8G8B8_SNORM                                   = 24,
    FORMAT_R8G8B8_USCALED                                 = 25,
    FORMAT_R8G8B8_SSCALED                                 = 26,
    FORMAT_R8G8B8_UINT                                    = 27,
    FORMAT_R8G8B8_SINT                                    = 28,
    FORMAT_R8G8B8_SRGB                                    = 29,
    FORMAT_B8G8R8_UNORM                                   = 30,
    FORMAT_B8G8R8_SNORM                                   = 31,
    FORMAT_B8G8R8_USCALED                                 = 32,
    FORMAT_B8G8R8_SSCALED                                 = 33,
    FORMAT_B8G8R8_UINT                                    = 34,
    FORMAT_B8G8R8_SINT                                    = 35,
    FORMAT_B8G8R8_SRGB                                    = 36,
    FORMAT_R8G8B8A8_UNORM                                 = 37,
    FORMAT_R8G8B8A8_SNORM                                 = 38,
    FORMAT_R8G8B8A8_USCALED                               = 39,
    FORMAT_R8G8B8A8_SSCALED                               = 40,
    FORMAT_R8G8B8A8_UINT                                  = 41,
    FORMAT_R8G8B8A8_SINT                                  = 42,
    FORMAT_R8G8B8A8_SRGB                                  = 43,
    FORMAT_B8G8R8A8_UNORM                                 = 44,
    FORMAT_B8G8R8A8_SNORM                                 = 45,
    FORMAT_B8G8R8A8_USCALED                               = 46,
    FORMAT_B8G8R8A8_SSCALED                               = 47,
    FORMAT_B8G8R8A8_UINT                                  = 48,
    FORMAT_B8G8R8A8_SINT                                  = 49,
    FORMAT_B8G8R8A8_SRGB                                  = 50,
    FORMAT_A8B8G8R8_UNORM_PACK32                          = 51,
    FORMAT_A8B8G8R8_SNORM_PACK32                          = 52,
    FORMAT_A8B8G8R8_USCALED_PACK32                        = 53,
    FORMAT_A8B8G8R8_SSCALED_PACK32                        = 54,
    FORMAT_A8B8G8R8_UINT_PACK32                           = 55,
    FORMAT_A8B8G8R8_SINT_PACK32                           = 56,
    FORMAT_A8B8G8R8_SRGB_PACK32                           = 57,
    FORMAT_A2R10G10B10_UNORM_PACK32                       = 58,
    FORMAT_A2R10G10B10_SNORM_PACK32                       = 59,
    FORMAT_A2R10G10B10_USCALED_PACK32                     = 60,
    FORMAT_A2R10G10B10_SSCALED_PACK32                     = 61,
    FORMAT_A2R10G10B10_UINT_PACK32                        = 62,
    FORMAT_A2R10G10B10_SINT_PACK32                        = 63,
    FORMAT_A2B10G10R10_UNORM_PACK32                       = 64,
    FORMAT_A2B10G10R10_SNORM_PACK32                       = 65,
    FORMAT_A2B10G10R10_USCALED_PACK32                     = 66,
    FORMAT_A2B10G10R10_SSCALED_PACK32                     = 67,
    FORMAT_A2B10G10R10_UINT_PACK32                        = 68,
    FORMAT_A2B10G10R10_SINT_PACK32                        = 69,
    FORMAT_R16_UNORM                                      = 70,
    FORMAT_R16_SNORM                                      = 71,
    FORMAT_R16_USCALED                                    = 72,
    FORMAT_R16_SSCALED                                    = 73,
    FORMAT_R16_UINT                                       = 74,
    FORMAT_R16_SINT                                       = 75,
    FORMAT_R16_SFLOAT                                     = 76,
    FORMAT_R16G16_UNORM                                   = 77,
    FORMAT_R16G16_SNORM                                   = 78,
    FORMAT_R16G16_USCALED                                 = 79,
    FORMAT_R16G16_SSCALED                                 = 80,
    FORMAT_R16G16_UINT                                    = 81,
    FORMAT_R16G16_SINT                                    = 82,
    FORMAT_R16G16_SFLOAT                                  = 83,
    FORMAT_R16G16B16_UNORM                                = 84,
    FORMAT_R16G16B16_SNORM                                = 85,
    FORMAT_R16G16B16_USCALED                              = 86,
    FORMAT_R16G16B16_SSCALED                              = 87,
    FORMAT_R16G16B16_UINT                                 = 88,
    FORMAT_R16G16B16_SINT                                 = 89,
    FORMAT_R16G16B16_SFLOAT                               = 90,
    FORMAT_R16G16B16A16_UNORM                             = 91,
    FORMAT_R16G16B16A16_SNORM                             = 92,
    FORMAT_R16G16B16A16_USCALED                           = 93,
    FORMAT_R16G16B16A16_SSCALED                           = 94,
    FORMAT_R16G16B16A16_UINT                              = 95,
    FORMAT_R16G16B16A16_SINT                              = 96,
    FORMAT_R16G16B16A16_SFLOAT                            = 97,
    FORMAT_R32_UINT                                       = 98,
    FORMAT_R32_SINT                                       = 99,
    FORMAT_R32_SFLOAT                                     = 100,
    FORMAT_R32G32_UINT                                    = 101,
    FORMAT_R32G32_SINT                                    = 102,
    FORMAT_R32G32_SFLOAT                                  = 103,
    FORMAT_R32G32B32_UINT                                 = 104,
    FORMAT_R32G32B32_SINT                                 = 105,
    FORMAT_R32G32B32_SFLOAT                               = 106,
    FORMAT_R32G32B32A32_UINT                              = 107,
    FORMAT_R32G32B32A32_SINT                              = 108,
    FORMAT_R32G32B32A32_SFLOAT                            = 109,
    FORMAT_R64_UINT                                       = 110,
    FORMAT_R64_SINT                                       = 111,
    FORMAT_R64_SFLOAT                                     = 112,
    FORMAT_R64G64_UINT                                    = 113,
    FORMAT_R64G64_SINT                                    = 114,
    FORMAT_R64G64_SFLOAT                                  = 115,
    FORMAT_R64G64B64_UINT                                 = 116,
    FORMAT_R64G64B64_SINT                                 = 117,
    FORMAT_R64G64B64_SFLOAT                               = 118,
    FORMAT_R64G64B64A64_UINT                              = 119,
    FORMAT_R64G64B64A64_SINT                              = 120,
    FORMAT_R64G64B64A64_SFLOAT                            = 121,
    FORMAT_B10G11R11_UFLOAT_PACK32                        = 122,
    FORMAT_E5B9G9R9_UFLOAT_PACK32                         = 123,
    FORMAT_D16_UNORM                                      = 124,
    FORMAT_X8_D24_UNORM_PACK32                            = 125,
    FORMAT_D32_SFLOAT                                     = 126,
    FORMAT_S8_UINT                                        = 127,
    FORMAT_D16_UNORM_S8_UINT                              = 128,
    FORMAT_D24_UNORM_S8_UINT                              = 129,
    FORMAT_D32_SFLOAT_S8_UINT                             = 130,
    FORMAT_BC1_RGB_UNORM_BLOCK                            = 131,
    FORMAT_BC1_RGB_SRGB_BLOCK                             = 132,
    FORMAT_BC1_RGBA_UNORM_BLOCK                           = 133,
    FORMAT_BC1_RGBA_SRGB_BLOCK                            = 134,
    FORMAT_BC2_UNORM_BLOCK                                = 135,
    FORMAT_BC2_SRGB_BLOCK                                 = 136,
    FORMAT_BC3_UNORM_BLOCK                                = 137,
    FORMAT_BC3_SRGB_BLOCK                                 = 138,
    FORMAT_BC4_UNORM_BLOCK                                = 139,
    FORMAT_BC4_SNORM_BLOCK                                = 140,
    FORMAT_BC5_UNORM_BLOCK                                = 141,
    FORMAT_BC5_SNORM_BLOCK                                = 142,
    FORMAT_BC6H_UFLOAT_BLOCK                              = 143,
    FORMAT_BC6H_SFLOAT_BLOCK                              = 144,
    FORMAT_BC7_UNORM_BLOCK                                = 145,
    FORMAT_BC7_SRGB_BLOCK                                 = 146,
    FORMAT_ETC2_R8G8B8_UNORM_BLOCK                        = 147,
    FORMAT_ETC2_R8G8B8_SRGB_BLOCK                         = 148,
    FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK                      = 149,
    FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK                       = 150,
    FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK                      = 151,
    FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK                       = 152,
    FORMAT_EAC_R11_UNORM_BLOCK                            = 153,
    FORMAT_EAC_R11_SNORM_BLOCK                            = 154,
    FORMAT_EAC_R11G11_UNORM_BLOCK                         = 155,
    FORMAT_EAC_R11G11_SNORM_BLOCK                         = 156,
    FORMAT_ASTC_4x4_UNORM_BLOCK                           = 157,
    FORMAT_ASTC_4x4_SRGB_BLOCK                            = 158,
    FORMAT_ASTC_5x4_UNORM_BLOCK                           = 159,
    FORMAT_ASTC_5x4_SRGB_BLOCK                            = 160,
    FORMAT_ASTC_5x5_UNORM_BLOCK                           = 161,
    FORMAT_ASTC_5x5_SRGB_BLOCK                            = 162,
    FORMAT_ASTC_6x5_UNORM_BLOCK                           = 163,
    FORMAT_ASTC_6x5_SRGB_BLOCK                            = 164,
    FORMAT_ASTC_6x6_UNORM_BLOCK                           = 165,
    FORMAT_ASTC_6x6_SRGB_BLOCK                            = 166,
    FORMAT_ASTC_8x5_UNORM_BLOCK                           = 167,
    FORMAT_ASTC_8x5_SRGB_BLOCK                            = 168,
    FORMAT_ASTC_8x6_UNORM_BLOCK                           = 169,
    FORMAT_ASTC_8x6_SRGB_BLOCK                            = 170,
    FORMAT_ASTC_8x8_UNORM_BLOCK                           = 171,
    FORMAT_ASTC_8x8_SRGB_BLOCK                            = 172,
    FORMAT_ASTC_10x5_UNORM_BLOCK                          = 173,
    FORMAT_ASTC_10x5_SRGB_BLOCK                           = 174,
    FORMAT_ASTC_10x6_UNORM_BLOCK                          = 175,
    FORMAT_ASTC_10x6_SRGB_BLOCK                           = 176,
    FORMAT_ASTC_10x8_UNORM_BLOCK                          = 177,
    FORMAT_ASTC_10x8_SRGB_BLOCK                           = 178,
    FORMAT_ASTC_10x10_UNORM_BLOCK                         = 179,
    FORMAT_ASTC_10x10_SRGB_BLOCK                          = 180,
    FORMAT_ASTC_12x10_UNORM_BLOCK                         = 181,
    FORMAT_ASTC_12x10_SRGB_BLOCK                          = 182,
    FORMAT_ASTC_12x12_UNORM_BLOCK                         = 183,
    FORMAT_ASTC_12x12_SRGB_BLOCK                          = 184,
    FORMAT_G8B8G8R8_422_UNORM                             = 1000156000,
    FORMAT_B8G8R8G8_422_UNORM                             = 1000156001,
    FORMAT_G8_B8_R8_3PLANE_420_UNORM                      = 1000156002,
    FORMAT_G8_B8R8_2PLANE_420_UNORM                       = 1000156003,
    FORMAT_G8_B8_R8_3PLANE_422_UNORM                      = 1000156004,
    FORMAT_G8_B8R8_2PLANE_422_UNORM                       = 1000156005,
    FORMAT_G8_B8_R8_3PLANE_444_UNORM                      = 1000156006,
    FORMAT_R10X6_UNORM_PACK16                             = 1000156007,
    FORMAT_R10X6G10X6_UNORM_2PACK16                       = 1000156008,
    FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16             = 1000156009,
    FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16         = 1000156010,
    FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16         = 1000156011,
    FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16     = 1000156012,
    FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16      = 1000156013,
    FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16     = 1000156014,
    FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16      = 1000156015,
    FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16     = 1000156016,
    FORMAT_R12X4_UNORM_PACK16                             = 1000156017,
    FORMAT_R12X4G12X4_UNORM_2PACK16                       = 1000156018,
    FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16             = 1000156019,
    FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16         = 1000156020,
    FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16         = 1000156021,
    FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16     = 1000156022,
    FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16      = 1000156023,
    FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16     = 1000156024,
    FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16      = 1000156025,
    FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16     = 1000156026,
    FORMAT_G16B16G16R16_422_UNORM                         = 1000156027,
    FORMAT_B16G16R16G16_422_UNORM                         = 1000156028,
    FORMAT_G16_B16_R16_3PLANE_420_UNORM                   = 1000156029,
    FORMAT_G16_B16R16_2PLANE_420_UNORM                    = 1000156030,
    FORMAT_G16_B16_R16_3PLANE_422_UNORM                   = 1000156031,
    FORMAT_G16_B16R16_2PLANE_422_UNORM                    = 1000156032,
    FORMAT_G16_B16_R16_3PLANE_444_UNORM                   = 1000156033,
    FORMAT_G8_B8R8_2PLANE_444_UNORM                       = 1000330000,
    FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16      = 1000330001,
    FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16      = 1000330002,
    FORMAT_G16_B16R16_2PLANE_444_UNORM                    = 1000330003,
    FORMAT_A4R4G4B4_UNORM_PACK16                          = 1000340000,
    FORMAT_A4B4G4R4_UNORM_PACK16                          = 1000340001,
    FORMAT_ASTC_4x4_SFLOAT_BLOCK                          = 1000066000,
    FORMAT_ASTC_5x4_SFLOAT_BLOCK                          = 1000066001,
    FORMAT_ASTC_5x5_SFLOAT_BLOCK                          = 1000066002,
    FORMAT_ASTC_6x5_SFLOAT_BLOCK                          = 1000066003,
    FORMAT_ASTC_6x6_SFLOAT_BLOCK                          = 1000066004,
    FORMAT_ASTC_8x5_SFLOAT_BLOCK                          = 1000066005,
    FORMAT_ASTC_8x6_SFLOAT_BLOCK                          = 1000066006,
    FORMAT_ASTC_8x8_SFLOAT_BLOCK                          = 1000066007,
    FORMAT_ASTC_10x5_SFLOAT_BLOCK                         = 1000066008,
    FORMAT_ASTC_10x6_SFLOAT_BLOCK                         = 1000066009,
    FORMAT_ASTC_10x8_SFLOAT_BLOCK                         = 1000066010,
    FORMAT_ASTC_10x10_SFLOAT_BLOCK                        = 1000066011,
    FORMAT_ASTC_12x10_SFLOAT_BLOCK                        = 1000066012,
    FORMAT_ASTC_12x12_SFLOAT_BLOCK                        = 1000066013,
    FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG                    = 1000054000,
    FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG                    = 1000054001,
    FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG                    = 1000054002,
    FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG                    = 1000054003,
    FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG                     = 1000054004,
    FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG                     = 1000054005,
    FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG                     = 1000054006,
    FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG                     = 1000054007,
    FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT                      = FORMAT_ASTC_4x4_SFLOAT_BLOCK,
    FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT                      = FORMAT_ASTC_5x4_SFLOAT_BLOCK,
    FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT                      = FORMAT_ASTC_5x5_SFLOAT_BLOCK,
    FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT                      = FORMAT_ASTC_6x5_SFLOAT_BLOCK,
    FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT                      = FORMAT_ASTC_6x6_SFLOAT_BLOCK,
    FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT                      = FORMAT_ASTC_8x5_SFLOAT_BLOCK,
    FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT                      = FORMAT_ASTC_8x6_SFLOAT_BLOCK,
    FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT                      = FORMAT_ASTC_8x8_SFLOAT_BLOCK,
    FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT                     = FORMAT_ASTC_10x5_SFLOAT_BLOCK,
    FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT                     = FORMAT_ASTC_10x6_SFLOAT_BLOCK,
    FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT                     = FORMAT_ASTC_10x8_SFLOAT_BLOCK,
    FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT                    = FORMAT_ASTC_10x10_SFLOAT_BLOCK,
    FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT                    = FORMAT_ASTC_12x10_SFLOAT_BLOCK,
    FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT                    = FORMAT_ASTC_12x12_SFLOAT_BLOCK,
    FORMAT_G8B8G8R8_422_UNORM_KHR                         = FORMAT_G8B8G8R8_422_UNORM,
    FORMAT_B8G8R8G8_422_UNORM_KHR                         = FORMAT_B8G8R8G8_422_UNORM,
    FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR                  = FORMAT_G8_B8_R8_3PLANE_420_UNORM,
    FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR                   = FORMAT_G8_B8R8_2PLANE_420_UNORM,
    FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR                  = FORMAT_G8_B8_R8_3PLANE_422_UNORM,
    FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR                   = FORMAT_G8_B8R8_2PLANE_422_UNORM,
    FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR                  = FORMAT_G8_B8_R8_3PLANE_444_UNORM,
    FORMAT_R10X6_UNORM_PACK16_KHR                         = FORMAT_R10X6_UNORM_PACK16,
    FORMAT_R10X6G10X6_UNORM_2PACK16_KHR                   = FORMAT_R10X6G10X6_UNORM_2PACK16,
    FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR         = FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
    FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR     = FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
    FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR     = FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
    FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR = FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
    FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR  = FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
    FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR = FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
    FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR  = FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
    FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR = FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
    FORMAT_R12X4_UNORM_PACK16_KHR                         = FORMAT_R12X4_UNORM_PACK16,
    FORMAT_R12X4G12X4_UNORM_2PACK16_KHR                   = FORMAT_R12X4G12X4_UNORM_2PACK16,
    FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR         = FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
    FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR     = FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
    FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR     = FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
    FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR = FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
    FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR  = FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
    FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR = FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
    FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR  = FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
    FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR = FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
    FORMAT_G16B16G16R16_422_UNORM_KHR                     = FORMAT_G16B16G16R16_422_UNORM,
    FORMAT_B16G16R16G16_422_UNORM_KHR                     = FORMAT_B16G16R16G16_422_UNORM,
    FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR               = FORMAT_G16_B16_R16_3PLANE_420_UNORM,
    FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR                = FORMAT_G16_B16R16_2PLANE_420_UNORM,
    FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR               = FORMAT_G16_B16_R16_3PLANE_422_UNORM,
    FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR                = FORMAT_G16_B16R16_2PLANE_422_UNORM,
    FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR               = FORMAT_G16_B16_R16_3PLANE_444_UNORM,
    FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT                   = FORMAT_G8_B8R8_2PLANE_444_UNORM,
    FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT  = FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16,
    FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT  = FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16,
    FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT                = FORMAT_G16_B16R16_2PLANE_444_UNORM,
    FORMAT_A4R4G4B4_UNORM_PACK16_EXT                      = FORMAT_A4R4G4B4_UNORM_PACK16,
    FORMAT_A4B4G4R4_UNORM_PACK16_EXT                      = FORMAT_A4B4G4R4_UNORM_PACK16,
    FORMAT_MAX_ENUM                                       = 0x7FFFFFFF
};

enum ImageTiling
{
    IMAGE_TILING_OPTIMAL                 = 0,
    IMAGE_TILING_LINEAR                  = 1,
    IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT = 1000158000,
    IMAGE_TILING_MAX_ENUM                = 0x7FFFFFFF
};

enum ImageViewType
{
    IMAGE_VIEW_TYPE_1D         = 0,
    IMAGE_VIEW_TYPE_2D         = 1,
    IMAGE_VIEW_TYPE_3D         = 2,
    IMAGE_VIEW_TYPE_CUBE       = 3,
    IMAGE_VIEW_TYPE_1D_ARRAY   = 4,
    IMAGE_VIEW_TYPE_2D_ARRAY   = 5,
    IMAGE_VIEW_TYPE_CUBE_ARRAY = 6,
    IMAGE_VIEW_TYPE_MAX_ENUM   = 0x7FFFFFFF
};

enum ImageViewDimension
{
    IMAGE_VIEW_DIMENSION_1D          = 0,
    IMAGE_VIEW_DIMENSION_2D          = 1,
    IMAGE_VIEW_DIMENSION_3D          = 2,
    IMAGE_VIEW_DIMENSION_CUBE        = 3,
    IMAGE_VIEW_DIMENSION_1D_ARRAY    = 4,
    IMAGE_VIEW_DIMENSION_2D_ARRAY    = 5,
    IMAGE_VIEW_DIMENSION_CUBE_ARRAY  = 6,
    IMAGE_VIEW_DIMENSION_BEGIN_RANGE = IMAGE_VIEW_DIMENSION_1D,
    IMAGE_VIEW_DIMENSION_END_RANGE   = IMAGE_VIEW_DIMENSION_CUBE_ARRAY,
    IMAGE_VIEW_DIMENSION_NUM         = (IMAGE_VIEW_DIMENSION_CUBE_ARRAY - IMAGE_VIEW_DIMENSION_1D + 1),
    IMAGE_VIEW_DIMENSION_MAX_ENUM    = 0x7FFFFFFF
};

enum ImageType
{
    IMAGE_TYPE_1D       = 0,
    IMAGE_TYPE_2D       = 1,
    IMAGE_TYPE_3D       = 2,
    IMAGE_TYPE_MAX_ENUM = 0x7FFFFFFF
};

enum SampleCountFlagBits
{
    SAMPLE_COUNT_1_BIT              = 0x00000001,
    SAMPLE_COUNT_2_BIT              = 0x00000002,
    SAMPLE_COUNT_4_BIT              = 0x00000004,
    SAMPLE_COUNT_8_BIT              = 0x00000008,
    SAMPLE_COUNT_16_BIT             = 0x00000010,
    SAMPLE_COUNT_32_BIT             = 0x00000020,
    SAMPLE_COUNT_64_BIT             = 0x00000040,
    SAMPLE_COUNT_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
};
using SampleCountFlags = uint32_t;

struct Extent2D
{
    uint32_t width;
    uint32_t height;
};

struct Extent3D
{
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};

enum ComponentSwizzle
{
    COMPONENT_SWIZZLE_IDENTITY = 0,
    COMPONENT_SWIZZLE_ZERO     = 1,
    COMPONENT_SWIZZLE_ONE      = 2,
    COMPONENT_SWIZZLE_R        = 3,
    COMPONENT_SWIZZLE_G        = 4,
    COMPONENT_SWIZZLE_B        = 5,
    COMPONENT_SWIZZLE_A        = 6,
    COMPONENT_SWIZZLE_MAX_ENUM = 0x7FFFFFFF
};

struct ComponentMapping
{
    ComponentSwizzle r;
    ComponentSwizzle g;
    ComponentSwizzle b;
    ComponentSwizzle a;
};

struct ImageSubresourceRange
{
    uint32_t baseMipLevel = 0;
    uint32_t levelCount = 1;
    uint32_t baseArrayLayer = 0;
    uint32_t layerCount = 1;
};

enum Result
{
    SUCCESS                                            = 0,
    NOT_READY                                          = 1,
    TIMEOUT                                            = 2,
    EVENT_SET                                          = 3,
    EVENT_RESET                                        = 4,
    INCOMPLETE                                         = 5,
    ERROR_OUT_OF_HOST_MEMORY                           = -1,
    ERROR_OUT_OF_DEVICE_MEMORY                         = -2,
    ERROR_INITIALIZATION_FAILED                        = -3,
    ERROR_DEVICE_LOST                                  = -4,
    ERROR_MEMORY_MAP_FAILED                            = -5,
    ERROR_LAYER_NOT_PRESENT                            = -6,
    ERROR_EXTENSION_NOT_PRESENT                        = -7,
    ERROR_FEATURE_NOT_PRESENT                          = -8,
    ERROR_INCOMPATIBLE_DRIVER                          = -9,
    ERROR_TOO_MANY_OBJECTS                             = -10,
    ERROR_FORMAT_NOT_SUPPORTED                         = -11,
    ERROR_FRAGMENTED_POOL                              = -12,
    ERROR_UNKNOWN                                      = -13,
    ERROR_OUT_OF_POOL_MEMORY                           = -1000069000,
    ERROR_INVALID_EXTERNAL_HANDLE                      = -1000072003,
    ERROR_FRAGMENTATION                                = -1000161000,
    ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS               = -1000257000,
    PIPELINE_COMPILE_REQUIRED                          = 1000297000,
    ERROR_SURFACE_LOST_KHR                             = -1000000000,
    ERROR_NATIVE_WINDOW_IN_USE_KHR                     = -1000000001,
    SUBOPTIMAL_KHR                                     = 1000001003,
    ERROR_OUT_OF_DATE_KHR                              = -1000001004,
    ERROR_INCOMPATIBLE_DISPLAY_KHR                     = -1000003001,
    ERROR_VALIDATION_FAILED_EXT                        = -1000011001,
    ERROR_INVALID_SHADER_NV                            = -1000012000,
    ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT = -1000158000,
    ERROR_NOT_PERMITTED_KHR                            = -1000174001,
    ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT          = -1000255000,
    THREAD_IDLE_KHR                                    = 1000268000,
    THREAD_DONE_KHR                                    = 1000268001,
    OPERATION_DEFERRED_KHR                             = 1000268002,
    OPERATION_NOT_DEFERRED_KHR                         = 1000268003,
    ERROR_COMPRESSION_EXHAUSTED_EXT                    = -1000338000,
    ERROR_OUT_OF_POOL_MEMORY_KHR                       = ERROR_OUT_OF_POOL_MEMORY,
    ERROR_INVALID_EXTERNAL_HANDLE_KHR                  = ERROR_INVALID_EXTERNAL_HANDLE,
    ERROR_FRAGMENTATION_EXT                            = ERROR_FRAGMENTATION,
    ERROR_NOT_PERMITTED_EXT                            = ERROR_NOT_PERMITTED_KHR,
    ERROR_INVALID_DEVICE_ADDRESS_EXT                   = ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS,
    ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR           = ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS,
    PIPELINE_COMPILE_REQUIRED_EXT                      = PIPELINE_COMPILE_REQUIRED,
    ERROR_PIPELINE_COMPILE_REQUIRED_EXT                = PIPELINE_COMPILE_REQUIRED,
    RESULT_MAX_ENUM                                    = 0x7FFFFFFF
};

enum ShaderStageFlagBits
{
    SHADER_STAGE_VERTEX_BIT                  = 0x00000001,
    SHADER_STAGE_TESSELLATION_CONTROL_BIT    = 0x00000002,
    SHADER_STAGE_TESSELLATION_EVALUATION_BIT = 0x00000004,
    SHADER_STAGE_GEOMETRY_BIT                = 0x00000008,
    SHADER_STAGE_FRAGMENT_BIT                = 0x00000010,
    SHADER_STAGE_COMPUTE_BIT                 = 0x00000020,
    SHADER_STAGE_ALL_GRAPHICS                = 0x0000001F,
    SHADER_STAGE_ALL                         = 0x7FFFFFFF,
    SHADER_STAGE_RAYGEN_BIT_KHR              = 0x00000100,
    SHADER_STAGE_ANY_HIT_BIT_KHR             = 0x00000200,
    SHADER_STAGE_CLOSEST_HIT_BIT_KHR         = 0x00000400,
    SHADER_STAGE_MISS_BIT_KHR                = 0x00000800,
    SHADER_STAGE_INTERSECTION_BIT_KHR        = 0x00001000,
    SHADER_STAGE_CALLABLE_BIT_KHR            = 0x00002000,
    SHADER_STAGE_TASK_BIT_NV                 = 0x00000040,
    SHADER_STAGE_MESH_BIT_NV                 = 0x00000080,
    SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI  = 0x00004000,
    SHADER_STAGE_RAYGEN_BIT_NV               = SHADER_STAGE_RAYGEN_BIT_KHR,
    SHADER_STAGE_ANY_HIT_BIT_NV              = SHADER_STAGE_ANY_HIT_BIT_KHR,
    SHADER_STAGE_CLOSEST_HIT_BIT_NV          = SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
    SHADER_STAGE_MISS_BIT_NV                 = SHADER_STAGE_MISS_BIT_KHR,
    SHADER_STAGE_INTERSECTION_BIT_NV         = SHADER_STAGE_INTERSECTION_BIT_KHR,
    SHADER_STAGE_CALLABLE_BIT_NV             = SHADER_STAGE_CALLABLE_BIT_KHR,
    SHADER_STAGE_FLAG_BITS_MAX_ENUM          = 0x7FFFFFFF
};
using ShaderStageFlags = uint32_t;

enum AccessFlagBits
{
    ACCESS_INDIRECT_COMMAND_READ_BIT                     = 0x00000001,
    ACCESS_INDEX_READ_BIT                                = 0x00000002,
    ACCESS_VERTEX_ATTRIBUTE_READ_BIT                     = 0x00000004,
    ACCESS_UNIFORM_READ_BIT                              = 0x00000008,
    ACCESS_INPUT_ATTACHMENT_READ_BIT                     = 0x00000010,
    ACCESS_SHADER_READ_BIT                               = 0x00000020,
    ACCESS_SHADER_WRITE_BIT                              = 0x00000040,
    ACCESS_COLOR_ATTACHMENT_READ_BIT                     = 0x00000080,
    ACCESS_COLOR_ATTACHMENT_WRITE_BIT                    = 0x00000100,
    ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT             = 0x00000200,
    ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT            = 0x00000400,
    ACCESS_TRANSFER_READ_BIT                             = 0x00000800,
    ACCESS_TRANSFER_WRITE_BIT                            = 0x00001000,
    ACCESS_HOST_READ_BIT                                 = 0x00002000,
    ACCESS_HOST_WRITE_BIT                                = 0x00004000,
    ACCESS_MEMORY_READ_BIT                               = 0x00008000,
    ACCESS_MEMORY_WRITE_BIT                              = 0x00010000,
    ACCESS_NONE                                          = 0,
    ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT              = 0x02000000,
    ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT       = 0x04000000,
    ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT      = 0x08000000,
    ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT            = 0x00100000,
    ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT     = 0x00080000,
    ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR           = 0x00200000,
    ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR          = 0x00400000,
    ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT             = 0x01000000,
    ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR = 0x00800000,
    ACCESS_COMMAND_PREPROCESS_READ_BIT_NV                = 0x00020000,
    ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV               = 0x00040000,
    ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV                = ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR,
    ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV            = ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
    ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV           = ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
    ACCESS_NONE_KHR                                      = ACCESS_NONE,
    ACCESS_FLAG_BITS_MAX_ENUM                            = 0x7FFFFFFF
};
using AccessFlags = uint32_t;

struct DummyCreateInfo
{
};

template <typename T_Handle, typename T_CreateInfo = DummyCreateInfo>
class ResourceHandle
{
public:
    T_Handle&       getHandle() { return m_handle; }
    const T_Handle& getHandle() const { return m_handle; }
    T_CreateInfo&   getCreateInfo() { return m_createInfo; }

protected:
    T_Handle     m_handle{};
    T_CreateInfo m_createInfo{};
};

}  // namespace aph

#endif  // RESOURCE_H_

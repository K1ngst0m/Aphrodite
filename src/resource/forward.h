#pragma once

#include "common/logger.h"

namespace aph
{
// Forward declaration of logging functions for the resource loader module
GENERATE_LOG_FUNCS(LOADER)

// Forward declare Asset classes
class BufferAsset;
class ImageAsset;
class GeometryAsset;
class ShaderAsset;

// Forward declare Loader classes
class BufferLoader;
class ImageLoader;
class GeometryLoader;
class ShaderLoader;
class ResourceLoader;

// Forward declare Load info structures
struct BufferLoadInfo;
struct ImageLoadInfo;
struct GeometryLoadInfo;
struct ShaderLoadInfo;
struct BufferUpdateInfo;
struct ResourceLoaderCreateInfo;

// Forward declare other related classes
struct LoadRequest;
} // namespace aph
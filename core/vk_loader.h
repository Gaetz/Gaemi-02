﻿#pragma once

#include <vk_types.h>
#include <unordered_map>
#include <filesystem>

struct GeoSurface
{
    uint32_t startIndex;
    uint32_t count;
};

struct MeshAsset
{
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers meshBuffers;
};

// Forward declaration
class Engine;

std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGltfMeshes(Engine* engine, std::filesystem::path filePath);

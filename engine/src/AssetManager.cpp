#include "Arc/AssetManager.hpp"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace Arc
{
    namespace
    {
        const cgltf_accessor* findAttribute(const cgltf_primitive& primitive, cgltf_attribute_type type)
        {
            for(cgltf_size index = 0; index < primitive.attributes_count; ++index)
            {
                const cgltf_attribute& attribute = primitive.attributes[index];
                if(attribute.type == type)
                {
                    return attribute.data;
                }
            }

            return nullptr;
        }

        TextureSlot readTextureSlot(const cgltf_texture_view& textureView, const std::filesystem::path& modelDirectory, bool srgb)
        {
            TextureSlot slot{};
            slot.srgb = srgb;

            if(textureView.texture == nullptr || textureView.texture->image == nullptr)
            {
                return slot;
            }

            const cgltf_image* image = textureView.texture->image;
            slot.debugName = image->name != nullptr ? image->name : "glTF image";

            if(image->uri != nullptr)
            {
                slot.path = (modelDirectory / image->uri).lexically_normal().string();
                return slot;
            }

            if(image->buffer_view != nullptr && image->buffer_view->buffer != nullptr && image->buffer_view->buffer->data != nullptr)
            {
                const auto* bufferData = static_cast<const std::uint8_t*>(image->buffer_view->buffer->data);
                const std::uint8_t* begin = bufferData + image->buffer_view->offset;
                const std::uint8_t* end = begin + image->buffer_view->size;
                slot.encodedData.assign(begin, end);
            }

            return slot;
        }

        Material readMaterial(const cgltf_material* material, const std::filesystem::path& modelDirectory)
        {
            Material result{};

            if(material == nullptr)
            {
                return result;
            }

            if(material->has_pbr_metallic_roughness)
            {
                const cgltf_pbr_metallic_roughness& pbr = material->pbr_metallic_roughness;
                result.baseColor = {
                    pbr.base_color_factor[0],
                    pbr.base_color_factor[1],
                    pbr.base_color_factor[2],
                    pbr.base_color_factor[3]
                };
                result.metallic = pbr.metallic_factor;
                result.roughness = pbr.roughness_factor;
                result.albedoTexture = readTextureSlot(pbr.base_color_texture, modelDirectory, true);
                result.metallicRoughnessTexture = readTextureSlot(pbr.metallic_roughness_texture, modelDirectory, false);
            }

            result.normalTexture = readTextureSlot(material->normal_texture, modelDirectory, false);
            result.aoTexture = readTextureSlot(material->occlusion_texture, modelDirectory, false);
            result.emissiveTexture = readTextureSlot(material->emissive_texture, modelDirectory, true);

            const float emissiveMax = std::max(std::max(material->emissive_factor[0], material->emissive_factor[1]), material->emissive_factor[2]);
            result.emissive = material->has_emissive_strength ? material->emissive_strength.emissive_strength : emissiveMax;

            return result;
        }

        std::string meshName(const cgltf_mesh& mesh, cgltf_size primitiveIndex)
        {
            const std::string baseName = mesh.name != nullptr ? mesh.name : "Mesh";
            return baseName + "." + std::to_string(static_cast<unsigned long long>(primitiveIndex));
        }

        void generateTangents(MeshData& meshData)
        {
            std::vector<Vec3> accumulatedTangents(meshData.vertices.size());

            for(std::size_t index = 0; index + 2 < meshData.indices.size(); index += 3)
            {
                MeshVertex& v0 = meshData.vertices[meshData.indices[index]];
                MeshVertex& v1 = meshData.vertices[meshData.indices[index + 1]];
                MeshVertex& v2 = meshData.vertices[meshData.indices[index + 2]];

                const Vec3 edge1 = v1.position - v0.position;
                const Vec3 edge2 = v2.position - v0.position;
                const Vec2 deltaUv1{ v1.uv.x - v0.uv.x, v1.uv.y - v0.uv.y };
                const Vec2 deltaUv2{ v2.uv.x - v0.uv.x, v2.uv.y - v0.uv.y };
                const float determinant = deltaUv1.x * deltaUv2.y - deltaUv2.x * deltaUv1.y;

                if(std::abs(determinant) <= 0.000001f)
                {
                    continue;
                }

                const float inverseDeterminant = 1.0f / determinant;
                const Vec3 tangent = normalize({
                    inverseDeterminant * (deltaUv2.y * edge1.x - deltaUv1.y * edge2.x),
                    inverseDeterminant * (deltaUv2.y * edge1.y - deltaUv1.y * edge2.y),
                    inverseDeterminant * (deltaUv2.y * edge1.z - deltaUv1.y * edge2.z)
                });

                accumulatedTangents[meshData.indices[index]] += tangent;
                accumulatedTangents[meshData.indices[index + 1]] += tangent;
                accumulatedTangents[meshData.indices[index + 2]] += tangent;
            }

            for(std::size_t index = 0; index < meshData.vertices.size(); ++index)
            {
                const Vec3 normal = meshData.vertices[index].normal;
                Vec3 tangent = accumulatedTangents[index];

                if(length(tangent) <= 0.00001f)
                {
                    tangent = std::abs(normal.y) < 0.99f ? normalize(cross({ 0.0f, 1.0f, 0.0f }, normal)) : Vec3{ 1.0f, 0.0f, 0.0f };
                }
                else
                {
                    tangent = normalize(tangent - normal * dot(normal, tangent));
                }

                meshData.vertices[index].tangent = { tangent.x, tangent.y, tangent.z, 1.0f };
            }
        }
    }

    ModelData AssetManager::loadGltfModel(const std::string& path) const
    {
        cgltf_options options{};
        cgltf_data* data = nullptr;

        cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);
        if(result != cgltf_result_success)
        {
            throw std::runtime_error("Failed to parse glTF file: " + path);
        }

        result = cgltf_load_buffers(&options, data, path.c_str());
        if(result != cgltf_result_success)
        {
            cgltf_free(data);
            throw std::runtime_error("Failed to load glTF buffers: " + path);
        }

        result = cgltf_validate(data);
        if(result != cgltf_result_success)
        {
            cgltf_free(data);
            throw std::runtime_error("Invalid glTF file: " + path);
        }

        ModelData model{};
        const std::filesystem::path modelPath(path);
        const std::filesystem::path modelDirectory = modelPath.parent_path();
        model.name = modelPath.stem().string();

        for(cgltf_size meshIndex = 0; meshIndex < data->meshes_count; ++meshIndex)
        {
            const cgltf_mesh& mesh = data->meshes[meshIndex];

            for(cgltf_size primitiveIndex = 0; primitiveIndex < mesh.primitives_count; ++primitiveIndex)
            {
                const cgltf_primitive& primitive = mesh.primitives[primitiveIndex];
                if(primitive.type != cgltf_primitive_type_triangles)
                {
                    continue;
                }

                const cgltf_accessor* positions = findAttribute(primitive, cgltf_attribute_type_position);
                const cgltf_accessor* normals = findAttribute(primitive, cgltf_attribute_type_normal);
                const cgltf_accessor* texcoords = findAttribute(primitive, cgltf_attribute_type_texcoord);
                const cgltf_accessor* tangents = findAttribute(primitive, cgltf_attribute_type_tangent);
                if(positions == nullptr)
                {
                    continue;
                }

                MeshData meshData{};
                meshData.name = meshName(mesh, primitiveIndex);
                meshData.material = readMaterial(primitive.material, modelDirectory);
                meshData.vertices.reserve(positions->count);

                for(cgltf_size vertexIndex = 0; vertexIndex < positions->count; ++vertexIndex)
                {
                    cgltf_float position[3]{};
                    cgltf_float normal[3]{ 0.0f, 1.0f, 0.0f };
                    cgltf_float texcoord[2]{};
                    cgltf_float tangent[4]{ 1.0f, 0.0f, 0.0f, 1.0f };

                    cgltf_accessor_read_float(positions, vertexIndex, position, 3);
                    if(normals != nullptr)
                    {
                        cgltf_accessor_read_float(normals, vertexIndex, normal, 3);
                    }
                    if(texcoords != nullptr)
                    {
                        cgltf_accessor_read_float(texcoords, vertexIndex, texcoord, 2);
                    }
                    if(tangents != nullptr)
                    {
                        cgltf_accessor_read_float(tangents, vertexIndex, tangent, 4);
                    }

                    meshData.vertices.push_back({
                        { position[0], position[1], position[2] },
                        normalize({ normal[0], normal[1], normal[2] }),
                        { texcoord[0], texcoord[1] },
                        { tangent[0], tangent[1], tangent[2], tangent[3] }
                    });
                }

                if(primitive.indices != nullptr)
                {
                    meshData.indices.reserve(primitive.indices->count);
                    for(cgltf_size index = 0; index < primitive.indices->count; ++index)
                    {
                        meshData.indices.push_back(static_cast<std::uint32_t>(cgltf_accessor_read_index(primitive.indices, index)));
                    }
                }
                else
                {
                    meshData.indices.reserve(positions->count);
                    for(cgltf_size index = 0; index < positions->count; ++index)
                    {
                        meshData.indices.push_back(static_cast<std::uint32_t>(index));
                    }
                }

                if(!meshData.vertices.empty() && !meshData.indices.empty())
                {
                    if(tangents == nullptr)
                    {
                        generateTangents(meshData);
                    }

                    model.meshes.push_back(std::move(meshData));
                }
            }
        }

        cgltf_free(data);

        if(model.meshes.empty())
        {
            throw std::runtime_error("glTF file did not contain supported triangle meshes: " + path);
        }

        return model;
    }
}

/*
* Copyright (c) 2018 NVIDIA CORPORATION. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  * Neither the name of NVIDIA CORPORATION nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <OptixUtils/OptiXMesh.h>
#include <OptixUtils/sutil.h>

#include <Resources/Resources/BowMesh.h>
#include <Resources/Resources/BowMaterial.h>
#include <Resources/Resources/BowImage.h>

#include <Resources/ResourceManagers/BowMeshManager.h>
#include <Resources/ResourceManagers/BowMaterialManager.h>
#include <Resources/ResourceManagers/BowImageManager.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace optix {
	float3 make_float3(const float* a)
	{
		return make_float3(a[0], a[1], a[2]);
	}
}


namespace
{

	struct MeshBuffers
	{
		optix::Buffer tri_indices;
		optix::Buffer mat_indices;
		optix::Buffer positions;
		optix::Buffer normals;
		optix::Buffer tangents;
		optix::Buffer bitangents;
		optix::Buffer texcoords;
	};

	void createMaterialPrograms(
		optix::Context         context,
		bool                   use_textures,
		optix::Program&        closest_hit,
		optix::Program&        any_hit
		)
	{
		const char *ptx = sutil::getPtxString("phong.cu");

		if (!closest_hit)
			closest_hit = context->createProgramFromPTXString(ptx, use_textures ? "closest_hit_radiance_textured" : "closest_hit_radiance");

		if (!any_hit)
			any_hit = context->createProgramFromPTXString(ptx, "any_hit_shadow");
	}

	optix::Material createOptiXMaterial(
		optix::Context			context,
		optix::Program			closest_hit,
		optix::Program			any_hit,
		const bow::Material*	mat_params,
		bool					use_textures
		)
	{
		optix::Material mat = context->createMaterial();
		mat->setClosestHitProgram(0u, closest_hit);
		mat->setAnyHitProgram(1u, any_hit);

		if (use_textures)
		{
			mat["Kd_map"]->setTextureSampler(sutil::loadTexture(context, mat_params->diffuse_texname, optix::make_float3(mat_params->diffuse[0], mat_params->diffuse[1], mat_params->diffuse[2])));
			mat["Ks_map"]->setTextureSampler(sutil::loadTexture(context, mat_params->specular_texname, optix::make_float3(mat_params->specular[0], mat_params->specular[1], mat_params->specular[2])));
			mat["Kn_map"]->setTextureSampler(sutil::loadTexture(context, mat_params->bump_texname, optix::make_float3(0.5f, 0.5f, 1.0f)));
			mat["Tr_map"]->setTextureSampler(sutil::loadTexture(context, mat_params->alpha_texname, optix::make_float3(mat_params->dissolve, mat_params->dissolve, mat_params->dissolve)));
			mat["Pm_map"]->setTextureSampler(sutil::loadTexture(context, mat_params->metallic_texname, optix::make_float3(mat_params->metallic, mat_params->metallic, mat_params->metallic)));
			mat["Ke_map"]->setTextureSampler(sutil::loadTexture(context, mat_params->emissive_texname, optix::make_float3(mat_params->emission[0], mat_params->emission[1], mat_params->emission[2])));
		}
		else
		{
			mat["Kd_map"]->setTextureSampler(sutil::loadTexture(context, "", optix::make_float3(mat_params->diffuse[0], mat_params->diffuse[1], mat_params->diffuse[2])));
			mat["Ks_map"]->setTextureSampler(sutil::loadTexture(context, "", optix::make_float3(mat_params->specular[0], mat_params->specular[1], mat_params->specular[2])));
			mat["Kn_map"]->setTextureSampler(sutil::loadTexture(context, "", optix::make_float3(0.5f, 0.5f, 1.0f)));
			mat["Tr_map"]->setTextureSampler(sutil::loadTexture(context, "", optix::make_float3(mat_params->dissolve, mat_params->dissolve, mat_params->dissolve)));
			mat["Pm_map"]->setTextureSampler(sutil::loadTexture(context, "", optix::make_float3(mat_params->metallic, mat_params->metallic, mat_params->metallic)));
			mat["Ke_map"]->setTextureSampler(sutil::loadTexture(context, "", optix::make_float3(mat_params->emission[0], mat_params->emission[1], mat_params->emission[2])));
		}

		mat["index_of_refraction"]->setFloat(mat_params->ior);
		mat["absorption_coefficien"]->setFloat(1.0f);

		return mat;
	}


	optix::Program createBoundingBoxProgram(optix::Context context)
	{
		return context->createProgramFromPTXString(sutil::getPtxString("triangle_mesh.cu"), "mesh_bounds");
	}


	optix::Program createIntersectionProgram(optix::Context context)
	{
		return context->createProgramFromPTXString(sutil::getPtxString("triangle_mesh.cu"), "mesh_intersect_refine");
	}

	void translateMeshToOptiX(const bow::MeshPtr& mesh, MeshBuffers& buffers, OptiXMesh& optix_mesh)
	{
		int32_t* tri_indices = reinterpret_cast<int32_t*>(buffers.tri_indices->map());
		int32_t* mat_indices = reinterpret_cast<int32_t*>(buffers.mat_indices->map());
		float*	positions = reinterpret_cast<float*>  (buffers.positions->map());
		float*	normals = reinterpret_cast<float*>  (mesh->HasNormals() ? buffers.normals->map() : 0);
		float*	tangents = reinterpret_cast<float*>  (mesh->HasNormals() && mesh->HasTextureCoordinates() ? buffers.tangents->map() : 0);
		float*	bitangents = reinterpret_cast<float*>  (mesh->HasNormals() && mesh->HasTextureCoordinates() ? buffers.bitangents->map() : 0);
		float*	texcoords = reinterpret_cast<float*>  (mesh->HasTextureCoordinates() ? buffers.texcoords->map() : 0);

		memcpy(tri_indices, &mesh->GetIndices()[0], mesh->GetNumIndices() * sizeof(int32_t));
		memset(mat_indices, 0, mesh->GetNumTriangles() * sizeof(int32_t));
		for (unsigned int i = 0; i < mesh->GetNumSubMeshes(); i++)
		{
			bow::SubMesh* subMesh = mesh->GetSubMesh(i);
			for (int32_t j = 0; j < (subMesh->GetNumIndices() / 3); j++)
			{
				memcpy(mat_indices + (subMesh->GetStartIndex() / 3) + j, &i, sizeof(int32_t));
			}
		}

		memcpy(positions, &mesh->GetVertices()[0], mesh->GetNumVertices() * sizeof(bow::Vector3<float>));

		if (mesh->HasNormals())
		{
			memcpy(normals, &mesh->GetNormals()[0], mesh->GetNumVertices() * sizeof(bow::Vector3<float>));

			if (mesh->HasTextureCoordinates())
			{
				memcpy(tangents, &mesh->GetTangents()[0], mesh->GetNumVertices() * sizeof(bow::Vector3<float>));
				memcpy(bitangents, &mesh->GetBitangents()[0], mesh->GetNumVertices() * sizeof(bow::Vector3<float>));
			}
		}

		if (mesh->HasTextureCoordinates())
			memcpy(texcoords, &mesh->GetTexCoords()[0], mesh->GetNumTexCoords() * sizeof(bow::Vector2<float>));


		bow::Vector3<float> bbox_min;
		bow::Vector3<float> bbox_max;
		mesh->GetBoundigBox(bbox_min, bbox_max);

		optix::Context ctx = optix_mesh.context;
		optix_mesh.bbox_min = optix::make_float3(bbox_min.x, bbox_min.y, bbox_min.z);
		optix_mesh.bbox_max = optix::make_float3(bbox_max.x, bbox_max.y, bbox_max.z);
		optix_mesh.num_triangles = mesh->GetNumTriangles();

		std::vector<bow::Material> materials;
		{
			std::vector<bow::MaterialCollectionPtr> materialCollections;
			std::vector<std::string> materialFiles = mesh->GetMaterialFiles();
			for (unsigned int i = 0; i < materialFiles.size(); i++)
			{
				bow::MaterialCollectionPtr materialCollection = bow::MaterialManager::GetInstance().Load(materialFiles[i]);

				if (materialCollection != nullptr)
				{
					std::size_t foundPos = materialCollection->VGetName().find_last_of("/");
					std::string filePath = "";
					if (foundPos >= 0)
					{
						filePath = materialCollection->VGetName().substr(0, foundPos + 1);
					}

					for (unsigned int i = 0; i < materialCollection->GetMaterials().size(); i++)
					{
						materials.push_back(*materialCollection->GetMaterials()[i]);
					}

					for (unsigned int i = 0; i < materials.size(); i++)
					{
						if (!materials[i].alpha_texname.empty())
							materials[i].alpha_texname = filePath + materials[i].alpha_texname;

						if (!materials[i].ambient_texname.empty())
							materials[i].ambient_texname = filePath + materials[i].ambient_texname;

						if (!materials[i].bump_texname.empty())
							materials[i].bump_texname = filePath + materials[i].bump_texname;

						if (!materials[i].diffuse_texname.empty())
							materials[i].diffuse_texname = filePath + materials[i].diffuse_texname;

						if (!materials[i].displacement_texname.empty())
							materials[i].displacement_texname = filePath + materials[i].displacement_texname;

						if (!materials[i].emissive_texname.empty())
							materials[i].emissive_texname = filePath + materials[i].emissive_texname;

						if (!materials[i].metallic_texname.empty())
							materials[i].metallic_texname = filePath + materials[i].metallic_texname;

						if (!materials[i].normal_texname.empty())
							materials[i].normal_texname = filePath + materials[i].normal_texname;

						if (!materials[i].reflection_texname.empty())
							materials[i].reflection_texname = filePath + materials[i].reflection_texname;

						if (!materials[i].roughness_texname.empty())
							materials[i].roughness_texname = filePath + materials[i].roughness_texname;

						if (!materials[i].sheen_texname.empty())
							materials[i].sheen_texname = filePath + materials[i].sheen_texname;

						if (!materials[i].specular_highlight_texname.empty())
							materials[i].specular_highlight_texname = filePath + materials[i].specular_highlight_texname;

						if (!materials[i].specular_texname.empty())
							materials[i].specular_texname = filePath + materials[i].specular_texname;
					}
				}
			}
		}

		std::vector<optix::Material> optix_materials;
		if (optix_mesh.material)
		{
			// Rewrite all mat_indices to point to single override material
			memset(mat_indices, 0, mesh->GetNumTriangles() * sizeof(int32_t));

			optix_materials.push_back(optix_mesh.material);
		}
		else
		{
			bool have_textures = false;
			for (unsigned int i = 0; i < materials.size(); ++i)
			{				
				if (!materials[i].diffuse_texname.empty())
				{
					have_textures = true;
				}
			}

			optix::Program closest_hit = optix_mesh.closest_hit;
			optix::Program any_hit = optix_mesh.any_hit;
			createMaterialPrograms(ctx, have_textures, closest_hit, any_hit);

			for (unsigned int i = 0; i < mesh->GetNumSubMeshes(); ++i)
			{
				bow::Material material;
				for (unsigned int j = 0; j < materials.size(); j++)
				{
					if (materials[j].name == mesh->GetSubMeshes()[i]->GetMaterialName())
					{
						material = materials[j];
						break;
					}
				}

				optix_materials.push_back(createOptiXMaterial(
					ctx,
					closest_hit,
					any_hit,
					&material,
					have_textures));
			}
		}

		optix::Geometry geometry = ctx->createGeometry();
		geometry["vertex_buffer"]->setBuffer(buffers.positions);
		geometry["normal_buffer"]->setBuffer(buffers.normals);
		geometry["tangent_buffer"]->setBuffer(buffers.tangents);
		geometry["bitangent_buffer"]->setBuffer(buffers.bitangents);
		geometry["texcoord_buffer"]->setBuffer(buffers.texcoords);
		geometry["material_buffer"]->setBuffer(buffers.mat_indices);
		geometry["index_buffer"]->setBuffer(buffers.tri_indices);
		geometry->setPrimitiveCount(mesh->GetNumTriangles());
		geometry->setBoundingBoxProgram(optix_mesh.bounds ? optix_mesh.bounds : createBoundingBoxProgram(ctx));
		geometry->setIntersectionProgram(optix_mesh.intersection ? optix_mesh.intersection : createIntersectionProgram(ctx));

		optix_mesh.geom_instance = ctx->createGeometryInstance(
			geometry,
			optix_materials.begin(),
			optix_materials.end()
		);

		buffers.tri_indices->unmap();
		buffers.mat_indices->unmap();
		buffers.positions->unmap();

		if (mesh->HasNormals())
		{
			buffers.normals->unmap();

			if (mesh->HasTextureCoordinates())
			{
				buffers.tangents->unmap();
				buffers.bitangents->unmap();
			}
		}

		if (mesh->HasTextureCoordinates())
			buffers.texcoords->unmap();
	}


} // namespace end

void loadMesh(
	const std::string&          filename,
	OptiXMesh&                  optix_mesh,
	float						unitsPerMeter
	)
{
	return loadMesh(filename, optix_mesh, optix::Matrix4x4::identity(), unitsPerMeter);
}

void loadMesh(
	const std::string&          filename,
	OptiXMesh&                  optix_mesh,
	const optix::Matrix4x4&     load_xform,
	float						unitsPerMeter
	)
{
	if (!optix_mesh.context)
	{
		throw std::runtime_error("OptiXMesh: loadMesh() requires valid OptiX context");
	}

	optix::Context context = optix_mesh.context;

	bow::MeshPtr mesh = bow::MeshManager::GetInstance().Load(filename);

	if (unitsPerMeter != 1.0f)
	{
		for (unsigned int i = 0; i < mesh->GetNumVertices(); i++)
		{
			mesh->GetVertices()[i] = mesh->GetVertices()[i] / unitsPerMeter;
		}
		mesh->RecalculateBoundingBox();
	}

	MeshBuffers buffers;
	buffers.tri_indices = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT3, mesh->GetNumTriangles());
	buffers.mat_indices = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT, mesh->GetNumTriangles());
	buffers.positions = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, mesh->GetNumVertices());
	buffers.normals = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, mesh->HasNormals() ? mesh->GetNumVertices() : 0);
	buffers.tangents = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, mesh->HasNormals() && mesh->HasTextureCoordinates() ? mesh->GetNumVertices() : 0);
	buffers.bitangents = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, mesh->HasNormals() && mesh->HasTextureCoordinates() ? mesh->GetNumVertices() : 0);
	buffers.texcoords = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT2, mesh->HasTextureCoordinates() ? mesh->GetNumVertices() : 0);

	translateMeshToOptiX(mesh, buffers, optix_mesh);
}


void createOptixMesh(
	bow::MeshPtr			mesh_ptr,
	OptiXMesh&              optix_mesh,
	float					unitsPerMeter
	)
{
	if (!optix_mesh.context)
	{
		throw std::runtime_error("OptiXMesh: loadMesh() requires valid OptiX context");
	}

	optix::Context context = optix_mesh.context;
	
	if (unitsPerMeter != 1.0f)
	{
		for (unsigned int i = 0; i < mesh_ptr->GetNumVertices(); i++)
		{
			mesh_ptr->GetVertices()[i] = mesh_ptr->GetVertices()[i] / unitsPerMeter;
		}
		mesh_ptr->RecalculateBoundingBox();
	}

	MeshBuffers buffers;
	buffers.tri_indices = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT3, mesh_ptr->GetNumTriangles());
	buffers.mat_indices = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT, mesh_ptr->GetNumTriangles());
	buffers.positions = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, mesh_ptr->GetNumVertices());
	buffers.normals = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, mesh_ptr->HasNormals() ? mesh_ptr->GetNumVertices() : 0);
	buffers.tangents = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, mesh_ptr->HasNormals() && mesh_ptr->HasTextureCoordinates() ? mesh_ptr->GetNumVertices() : 0);
	buffers.bitangents = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, mesh_ptr->HasNormals() && mesh_ptr->HasTextureCoordinates() ? mesh_ptr->GetNumVertices() : 0);
	buffers.texcoords = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT2, mesh_ptr->HasTextureCoordinates() ? mesh_ptr->GetNumVertices() : 0);

	translateMeshToOptiX(mesh_ptr, buffers, optix_mesh);
}

OPTIXUTILS_API optix::TextureSampler createSampler(
	optix::Context context,
	optix::float4 color
) 
{
	// Create tex sampler and populate with default values
	optix::TextureSampler sampler = context->createTextureSampler();
	sampler->setWrapMode(0, RT_WRAP_REPEAT);
	sampler->setWrapMode(1, RT_WRAP_REPEAT);
	sampler->setWrapMode(2, RT_WRAP_REPEAT);
	sampler->setIndexingMode(RT_TEXTURE_INDEX_NORMALIZED_COORDINATES);
	sampler->setReadMode(RT_TEXTURE_READ_NORMALIZED_FLOAT);
	sampler->setMaxAnisotropy(1.0f);
	sampler->setMipLevelCount(1u);
	sampler->setArraySize(1u);

	// Create buffer with single texel set to default_color
	optix::Buffer buffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_UNSIGNED_BYTE4, 1u, 1u);
	unsigned char* buffer_data = static_cast<unsigned char*>(buffer->map());
	buffer_data[0] = (unsigned char)optix::clamp((int)(color.x * 255.0f), 0, 255);
	buffer_data[1] = (unsigned char)optix::clamp((int)(color.y * 255.0f), 0, 255);
	buffer_data[2] = (unsigned char)optix::clamp((int)(color.z * 255.0f), 0, 255);
	buffer_data[3] = 255;
	buffer->unmap();

	sampler->setBuffer(0u, 0u, buffer);
	// Although it would be possible to use nearest filtering here, we chose linear
	// to be consistent with the textures that have been loaded from a file. This
	// allows OptiX to perform some optimizations.
	sampler->setFilteringModes(RT_FILTER_LINEAR, RT_FILTER_LINEAR, RT_FILTER_NONE);

	return sampler;
}

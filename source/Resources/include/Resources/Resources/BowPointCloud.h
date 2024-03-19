#pragma once
#include "Resources/Resources_api.h"
#include "Resources/BowResourcesPredeclares.h"

#include "Resources/BowResource.h"
#include "CoreSystems/BowMath.h"

namespace bow {

	// ---------------------------------------------------------------------------
	/** @brief A mesh represents geometry without any material.
	*/
	class RESOURCES_API PointCloud : public Resource
	{
		friend class PointCloudLoader_bin;
		friend class PointCloudLoader_xcn;
		friend class PointCloudLoader_xtc;
		friend class PointCloudLoader_xyz;
	public:
		PointCloud(ResourceManager* creator, const std::string& name, ResourceHandle handle);
		~PointCloud();

		std::vector<Vector3<float>> GetVertices() { return m_vertices; }
		std::vector<Vector3<float>> GetColors() { return m_colors; }
		std::vector<Vector3<float>>& GetNormals() { return m_normals; }

		MeshAttribute CreateAttribute(const std::string& positionAttribute, const std::string& colorAttribute);
		MeshAttribute CreateAttribute(const std::string& positionAttribute, const std::string& normalAttribute, const std::string& colorAttribute);

		AABB<float> GetBoundingBox();

	private:
		/** Loads the mesh from disk.  This call only performs IO, it
		does not parse the bytestream or check for any errors therein.
		It also does not set up submeshes, etc.  You have to call load()
		to do that.
		*/
		void VPrepareImpl(void);

		/** Destroys data cached by prepareImpl.
		*/
		void VUnprepareImpl(void);

		/// @copydoc Resource::VLoadImpl
		void VLoadImpl(void);

		/// @copydoc Resource::VPostLoadImpl
		void VPostLoadImpl(void);

		/// @copydoc Resource::VUnloadImpl
		void VUnloadImpl(void);

		/// @copydoc Resource::VCalculateSize
		size_t VCalculateSize(void) const;

		char* m_dataFromDisk;

		std::vector<Vector3<float>> m_vertices;
		std::vector<Vector3<float>> m_colors;
		std::vector<Vector3<float>> m_normals;
	};
}
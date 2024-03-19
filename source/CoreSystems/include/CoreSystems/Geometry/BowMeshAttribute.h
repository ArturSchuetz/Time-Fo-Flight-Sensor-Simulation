#pragma once
#include "CoreSystems/CoreSystems_api.h"
#include "CoreSystems/BowCorePredeclares.h"

#include "CoreSystems/Geometry/Indices/IBowIndicesBase.h"
#include "CoreSystems/Geometry/VertexAttributes/IBowVertexAttribute.h"

namespace bow {

	// ---------------------------------------------------------------------------
	/** @brief A mesh represents geometry without any material.
	*/
	class CORESYSTEMS_API MeshAttribute
	{
	public:
		VertexAttributePtr GetAttribute(std::string name) { return m_attributes.find(name)->second; };
		void AddAttribute(VertexAttributePtr vertexAttribute) {
			m_attributes.insert(std::pair<std::string, VertexAttributePtr>(vertexAttribute->Name, vertexAttribute));
		}

		IndicesBasePtr Indices;

		MeshAttribute() : m_attributes(), Indices(nullptr) { }
		~MeshAttribute() { }

	private:
		VertexAttributeMap m_attributes;
	};
}

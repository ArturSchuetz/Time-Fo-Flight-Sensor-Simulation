#pragma once
#include "CoreSystems/Geometry/VertexAttributes/BowVertexAttributeFloatVec4.h"

namespace bow {
	
	VertexAttributeFloatVec4::VertexAttributeFloatVec4(const std::string& name) : VertexAttribute<Vector4<float>>(name, VertexAttributeType::FloatVector4)
	{
	}

	VertexAttributeFloatVec4::VertexAttributeFloatVec4(const std::string& name, int capacity) : VertexAttribute<Vector4<float>>(name, VertexAttributeType::FloatVector4, capacity)
	{
	}

}

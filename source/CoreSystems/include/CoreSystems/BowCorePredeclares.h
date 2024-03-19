#pragma once
#include "CoreSystems/CoreSystems_api.h"

#include <memory>

namespace bow
{

	template<class T> class CORESYSTEMS_API Vector2;
	template<class T> class CORESYSTEMS_API Vector3;
	template<class T> class CORESYSTEMS_API Vector4;
	typedef Vector3<float> CORESYSTEMS_API ColorRGB;
	typedef Vector4<float> CORESYSTEMS_API ColorRGBA;

	template <typename T> class CORESYSTEMS_API Matrix;
	template <typename T> class CORESYSTEMS_API Matrix_trans;
	template <typename T> class CORESYSTEMS_API Matrix2D;
	template <typename T> class CORESYSTEMS_API Matrix3D;
	template <typename T> class CORESYSTEMS_API Matrix2x2;
	template <typename T> class CORESYSTEMS_API Matrix3x3;
	template <typename T> class CORESYSTEMS_API Matrix4x4;

	template <typename T> class CORESYSTEMS_API Quaternion;
	template <typename T> class CORESYSTEMS_API Transform;

	template <typename T> class CORESYSTEMS_API AABB;
	template <typename T> class CORESYSTEMS_API Plane;
	template <typename T> class CORESYSTEMS_API Ray;
	template <typename T> class CORESYSTEMS_API Sphere;
	template <typename T> class CORESYSTEMS_API Triangle;
	template <typename T> class CORESYSTEMS_API Frustum;

	enum class CORESYSTEMS_API IndicesType : char;
	struct CORESYSTEMS_API IndicesUnsignedInt;
	struct CORESYSTEMS_API IndicesUnsignedShort;
	struct CORESYSTEMS_API TriangleIndicesUnsignedInt;
	struct CORESYSTEMS_API VertexAttributeFloat;
	struct CORESYSTEMS_API VertexAttributeFloatVec2;
	struct CORESYSTEMS_API VertexAttributeFloatVec3;
	struct CORESYSTEMS_API VertexAttributeFloatVec4;

	class CORESYSTEMS_API MeshAttribute;
		typedef std::shared_ptr<MeshAttribute> MeshAttributePtr;

	class CORESYSTEMS_API SubdivisionSphereTessellator;

	class CORESYSTEMS_API BasicTimer;
	class CORESYSTEMS_API EventLogger;

} // namespace baselib

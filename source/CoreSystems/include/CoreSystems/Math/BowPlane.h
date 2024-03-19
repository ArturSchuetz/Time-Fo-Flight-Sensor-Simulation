#pragma once
#include "CoreSystems/CoreSystems_api.h"
#include "CoreSystems/BowCorePredeclares.h"

#include "CoreSystems/Math/BowVector3.h"

namespace bow {

	template <typename T> class CORESYSTEMS_API Plane
	{
	public:
		struct
		{
			Vector3<T>	point, normal;
			T			distance;
		};

		Plane()
		{
		}

		template <typename C>
		inline void Set(Vector3<C> _normal, Vector3<C> _point)
		{
			distance = -(_normal * _point);
			normal = _normal;
			point = _point;
		}

		template <typename C>
		inline void Set(Vector3<C> _normal, Vector3<C> _point, float _distance)
		{
			distance = _distance;
			normal = _normal;
			point = _point;
		}

		template <typename C>
		inline void Set(Vector3<C> vec1, Vector3<C> vec2, Vector3<C> vec3)
		{
			Vector3 edge1 = vec2 - vec1;
			Vector3 edge2 = vec3 - vec1;

			normal = edge1.Cross(edge2);
			distance = -(normal * vec1);
			point = vec1;
		}

		template <typename C>
		inline float Distance(Vector3<C> vcPoint)
		{
			return abs((normal * vcPoint) - distance);
		}
	};
	/*----------------------------------------------------------------*/
}

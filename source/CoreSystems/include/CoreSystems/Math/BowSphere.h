#pragma once
#include "CoreSystems/CoreSystems_api.h"
#include "CoreSystems/BowCorePredeclares.h"

#include "CoreSystems/Math/BowVector3.h"

namespace bow {

	template <typename T> class CORESYSTEMS_API Sphere
	{
	public:
		Vector3<T>	Center;
		T			Radius;

		Sphere()
		{
			Origin = Direction = Vector3<T>();
		}

		Sphere(const Vector3<T> center, T radius)
		{
			Center = center, Radius = radius;
		}

		inline void Set(Vector3<T> _center, T radius)
		{
			Center = center, Radius = radius;
		}
	};
	/*----------------------------------------------------------------*/
}

#pragma once
#include "CoreSystems/CoreSystems_api.h"
#include "CoreSystems/BowCorePredeclares.h"

#include "CoreSystems/Math/BowVector2.h"
#include "CoreSystems/Math/BowVector3.h"
#include "CoreSystems/Math/BowVector4.h"

#include "CoreSystems/Math/BowQuaternion.h"

#include "CoreSystems/Math/BowMatrix.h"
#include "CoreSystems/Math/BowMatrix2D.h"
#include "CoreSystems/Math/BowMatrix3D.h"
#include "CoreSystems/Math/BowMatrix3x3.h"
#include "CoreSystems/Math/BowMatrix4x4.h"

#include "CoreSystems/Math/BowSVD.h"
#include "CoreSystems/Math/BowAABB.h"
#include "CoreSystems/Math/BowPlane.h"
#include "CoreSystems/Math/BowRay.h"
#include "CoreSystems/Math/BowSphere.h"
#include "CoreSystems/Math/BowTriangle.h"
#include "CoreSystems/Math/BowFrustum.h"
#include "CoreSystems/Math/BowTransform.h"

namespace bow
{
	namespace math
	{
		//Taken from 'From Quaterninon to Matrix and Back', J.M.P. van Waveren, February 27th 2005
		static float CORESYSTEMS_API Sqrt(float x) {
			long i;
			float y, r;
			y = x * 0.5f;
			i = *(long *)(&x);
			i = 0x5f3759df - (i >> 1);
			r = *(float *)(&i);
			r = r * (1.5f - r * r * y);
			return r;
		}

		static float CORESYSTEMS_API Sqrt(double x) {
			long i;
			double y, r;
			y = x * 0.5f;
			i = *(long *)(&x);
			i = 0x5f3759df - (i >> 1);
			r = *(double *)(&i);
			r = r * (1.5f - r * r * y);
			return r;
		}
	}
}
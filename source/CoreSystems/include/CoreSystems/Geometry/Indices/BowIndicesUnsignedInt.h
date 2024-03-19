#pragma once
#include "CoreSystems/CoreSystems_api.h"
#include "CoreSystems/BowCorePredeclares.h"

#include "CoreSystems/Geometry/Indices/IBowIndicesBase.h"

#include <vector>

namespace bow {

	struct CORESYSTEMS_API IndicesUnsignedInt : IIndicesBase
	{
	public:
		IndicesUnsignedInt();
		IndicesUnsignedInt(int capacity);
		~IndicesUnsignedInt();

		unsigned int Size();
		std::vector<unsigned int> Values;
	};

}

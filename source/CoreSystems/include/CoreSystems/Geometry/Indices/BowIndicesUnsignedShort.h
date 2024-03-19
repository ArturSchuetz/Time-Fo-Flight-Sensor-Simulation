#pragma once
#include "CoreSystems/CoreSystems_api.h"
#include "CoreSystems/BowCorePredeclares.h"

#include "CoreSystems/Geometry/Indices/IBowIndicesBase.h"

#include <vector>

namespace bow {

	struct CORESYSTEMS_API IndicesUnsignedShort : IIndicesBase
	{
	public:
		IndicesUnsignedShort();
		IndicesUnsignedShort(int capacity);
		~IndicesUnsignedShort();

		unsigned int Size();
		std::vector<unsigned short> Values;
	};

}

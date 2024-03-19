#pragma once
#include "CoreSystems/Geometry/Indices/BowIndicesUnsignedInt.h"

namespace bow {
	
	IndicesUnsignedInt::IndicesUnsignedInt() : IIndicesBase(IndicesType::UnsignedInt), Values(std::vector<unsigned int>())
	{
	}

	IndicesUnsignedInt::IndicesUnsignedInt(int capacity) : IIndicesBase(IndicesType::UnsignedInt), Values(std::vector<unsigned int>(capacity))
	{
	}

	unsigned int IndicesUnsignedInt::Size()
	{
		return (unsigned int)Values.size();
	}

	IndicesUnsignedInt::~IndicesUnsignedInt()
	{
		Values.clear();
	}

}
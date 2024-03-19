#pragma once
#include "CoreSystems/CoreSystems_api.h"
#include "CoreSystems/BowCorePredeclares.h"

namespace bow {

	enum class CORESYSTEMS_API IndicesType : char
	{
		UnsignedShort,
		UnsignedInt
	};

	class CORESYSTEMS_API IIndicesBase
	{
	protected:
		IIndicesBase(IndicesType type) : Type(type)
		{
		}

	public:
		virtual ~IIndicesBase(){}

		virtual unsigned int Size() = 0;

	public:
		const IndicesType Type;
	};

	typedef std::shared_ptr<IIndicesBase> IndicesBasePtr;

}

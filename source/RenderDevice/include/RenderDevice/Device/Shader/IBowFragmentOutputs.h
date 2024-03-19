#pragma once
#include <RenderDevice/RenderDevice_api.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>

namespace bow {

	class IFragmentOutputs
	{
	public:
		virtual ~IFragmentOutputs() {}
		virtual int operator[](std::string index) const = 0;
	};

}

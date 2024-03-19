#pragma once
#include <RenderDevice/RenderDevice_api.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>

namespace bow {

	enum class DepthTestFunction : char
	{
		Never,
		Less,
		Equal,
		LessThanOrEqual,
		Greater,
		NotEqual,
		GreaterThanOrEqual,
		Always
	};

	struct DepthTest
	{
	public:
		DepthTest()
		{
			Enabled = true;
			Function = DepthTestFunction::Less;
		}

		bool Enabled;
		DepthTestFunction Function;
	};

}

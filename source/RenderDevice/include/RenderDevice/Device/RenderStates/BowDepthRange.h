#pragma once
#include <RenderDevice/RenderDevice_api.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>

namespace bow {

	struct DepthRange
	{
	public:

		DepthRange()
		{
			Near = 0.0;
			Far = 1.0;
		}

		float Near;
		float Far;
	};

}

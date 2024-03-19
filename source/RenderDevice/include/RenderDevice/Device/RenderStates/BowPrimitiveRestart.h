#pragma once
#include <RenderDevice/RenderDevice_api.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>

namespace bow {

	struct PrimitiveRestart
	{
	public:
		PrimitiveRestart()
		{
			Enabled = false;
			Index = 0;
		}

		bool Enabled;
		int Index;
	};

}

#pragma once
#include <RenderDevice/RenderDevice_api.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>

#include <RenderDevice/Device/RenderStates/BowStencilTestFace.h>

namespace bow {

	struct StencilTest
	{
	public:
		StencilTest()
		{
			Enabled = false;
		}

		bool Enabled;
		StencilTestFace FrontFace;
		StencilTestFace BackFace;
	};

}

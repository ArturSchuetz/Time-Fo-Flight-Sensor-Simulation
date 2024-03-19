#pragma once
#include <RenderDevice/RenderDevice_api.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>

namespace bow {

	struct ScissorTest
	{
	public:
		ScissorTest()
		{
			Enabled = false;
			rectangle_top = 0;
			rectangle_left = 0;
			rectangle_bottom = 0;
			rectangle_right = 0;
		}

		bool Enabled;
		long rectangle_top;
		long rectangle_left;
		long rectangle_bottom;
		long rectangle_right;
	};

}

#pragma once
#include <RenderDevice/RenderDevice_api.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>

#include <RenderDevice/Device/RenderStates/BowBlending.h>
#include <RenderDevice/Device/RenderStates/BowColorMask.h>
#include <RenderDevice/Device/RenderStates/BowDepthRange.h>
#include <RenderDevice/Device/RenderStates/BowDepthTest.h>
#include <RenderDevice/Device/RenderStates/BowFaceCulling.h>
#include <RenderDevice/Device/RenderStates/BowPrimitiveRestart.h>
#include <RenderDevice/Device/RenderStates/BowScissorTest.h>
#include <RenderDevice/Device/RenderStates/BowStencilTest.h>
#include <RenderDevice/Device/RenderStates/BowStencilTestFace.h>

namespace bow {

	enum class ProgramPointSize : char
	{
		Enabled,
		Disabled
	};

	enum class RasterizationMode : char
	{
		Point,
		Line,
		Fill
	};

	struct RenderState
	{
	public:
		RenderState() : colorMask(true, true, true, true)
		{
			programPointSize = ProgramPointSize::Disabled;
			rasterizationMode = RasterizationMode::Fill;
			depthMask = true;

			lineWidth = 1.0;
			pointSize = 1.0;
		}

		PrimitiveRestart primitiveRestart;
		FaceCulling faceCulling;
		ProgramPointSize programPointSize;
		RasterizationMode rasterizationMode;
		ScissorTest scissorTest;
		StencilTest stencilTest;
		DepthTest depthTest;
		DepthRange depthRange;
		Blending blending;
		ColorMask colorMask;
		bool depthMask;

		float lineWidth;
		float pointSize;
	};

}

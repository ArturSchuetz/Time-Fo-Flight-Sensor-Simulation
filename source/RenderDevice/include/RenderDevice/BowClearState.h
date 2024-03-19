#pragma once
#include <RenderDevice/RenderDevice_api.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>

#include <RenderDevice/Device/RenderStates/BowScissorTest.h>
#include <RenderDevice/Device/RenderStates/BowColorMask.h>

#include <CoreSystems/BowMath.h>

namespace bow {

	typedef enum TYPE_ClearBuffers : char
	{
		ColorBuffer = 1,
		DepthBuffer = 2,
		StencilBuffer = 4,
		ColorAndDepthBuffer = ColorBuffer | DepthBuffer,
		All = ColorBuffer | DepthBuffer | StencilBuffer
	} ClearBuffers;

	struct ClearState
	{
	public:
		ClearState() : colorMask(true, true, true, true)
		{
			depthMask = true;

			buffers = ClearBuffers::All;
			color = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
			depth = 1;
			stencil = 0;
		}

		ScissorTest scissorTest;
		ColorMask	colorMask;
		bool		depthMask;

		ClearBuffers	buffers;
		ColorRGBA		color;
		float			depth;
		int				stencil;
	};

}
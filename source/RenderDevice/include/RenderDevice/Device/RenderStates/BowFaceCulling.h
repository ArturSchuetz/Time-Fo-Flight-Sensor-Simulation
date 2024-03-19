#pragma once
#include <RenderDevice/RenderDevice_api.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>

#include <RenderDevice/Device/IBowRenderContext.h>

namespace bow {

	enum class CullFace : char
	{
		Front,
		Back,
		FrontAndBack
	};

	struct FaceCulling
	{
	public:
		FaceCulling()
		{
			Enabled = true;
			Face = CullFace::Back;
			FrontFaceWindingOrder = WindingOrder::Counterclockwise;
		}

		bool Enabled;
		CullFace Face;
		WindingOrder FrontFaceWindingOrder;
	};

}

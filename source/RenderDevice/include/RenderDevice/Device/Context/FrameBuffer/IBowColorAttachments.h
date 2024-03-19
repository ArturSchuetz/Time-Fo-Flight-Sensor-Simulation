#pragma once
#include <RenderDevice/RenderDevice_api.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>

namespace bow {

	class IColorAttachments
	{
	public:
		virtual ~IColorAttachments() {}

		virtual Texture2DPtr	VGetAttachment(unsigned int index) const = 0;
		virtual void			VSetAttachment(unsigned int index, Texture2DPtr texture) = 0;
	};

}

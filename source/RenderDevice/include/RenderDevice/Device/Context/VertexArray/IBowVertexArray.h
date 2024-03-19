#pragma once
#include <RenderDevice/RenderDevice_api.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>

namespace bow {

	class IVertexArray
	{
	public:
		virtual ~IVertexArray() {}

		virtual VertexBufferAttributeMap VGetAttributes() = 0;
		virtual void VSetAttribute(unsigned int location, VertexBufferAttributePtr pointer) = 0;
		virtual void VSetAttribute(ShaderVertexAttributePtr Location, VertexBufferAttributePtr pointer) = 0;

		virtual IndexBufferPtr	VGetIndexBuffer() = 0;
		virtual void			VSetIndexBuffer(IndexBufferPtr pointer) = 0;
	};

}

#pragma once
#include <OpenGL3xRenderDevice/OpenGL3xRenderDevice_api.h>
#include <OpenGL3xRenderDevice/Device/Buffer/BowOGL3xBuffer.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>
#include <RenderDevice/Device/Buffer/IBowVertexBuffer.h>

namespace bow {

	class OGLVertexBuffer : public IVertexBuffer
	{
	public:
		OGLVertexBuffer(BufferHint usageHint, int sizeInBytes);
		~OGLVertexBuffer();

		void Bind();
		static void UnBind();

		void VCopyFromSystemMemory(void* bufferInSystemMemory, int destinationOffsetInBytes, int lengthInBytes);
		std::shared_ptr<void> VCopyToSystemMemory(int offsetInBytes, int sizeInBytes);

		int			VGetSizeInBytes();
		BufferHint	VGetUsageHint();

	private:
		OGLBuffer	m_BufferObject;
	};

	typedef std::shared_ptr<OGLVertexBuffer> OGLVertexBufferPtr;

}

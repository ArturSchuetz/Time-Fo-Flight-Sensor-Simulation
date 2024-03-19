#pragma once
#include <OpenGL3xRenderDevice/OpenGL3xRenderDevice_api.h>
#include <OpenGL3xRenderDevice/Device/Buffer/BowOGL3xPixelBuffer.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>
#include <RenderDevice/Device/Buffer/IBowWritePixelBuffer.h>

namespace bow {

	class OGLWritePixelBuffer : public IWritePixelBuffer
	{
	public:
		OGLWritePixelBuffer(PixelBufferHint usageHint, int sizeInBytes);

		void Bind();
		static void UnBind();

		void VCopyFromSystemMemory(void* bufferInSystemMemory, int destinationOffsetInBytes, int lengthInBytes);
		std::shared_ptr<void> VCopyToSystemMemory(int offsetInBytes, int sizeInBytes);

		int				VGetSizeInBytes() const;
		PixelBufferHint	VGetUsageHint() const;

	private:
		const PixelBufferHint m_UsageHint;
		OGLPixelBuffer m_BufferObject;
	};

	typedef std::shared_ptr<OGLWritePixelBuffer> OGLWritePixelBufferPtr;

}

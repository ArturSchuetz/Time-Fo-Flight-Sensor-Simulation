#include <OpenGL3xRenderDevice/Device/Buffer/BowOGL3xReadPixelBuffer.h>
#include <RenderDevice/Device/Buffer/BowBufferHint.h>

#include <GL/glew.h>
#if defined(_WIN32)
#include <GL/wglew.h>
#endif

namespace bow {

	BufferHint rbp_bufferHints[] = {
		BufferHint::StreamRead,
		BufferHint::StaticRead,
		BufferHint::DynamicRead
	};


	OGLReadPixelBuffer::OGLReadPixelBuffer(PixelBufferHint usageHint, int sizeInBytes) : m_UsageHint(usageHint), m_BufferObject(GL_PIXEL_PACK_BUFFER, rbp_bufferHints[(int)usageHint], sizeInBytes)
	{
	}

	void OGLReadPixelBuffer::Bind()
	{
		m_BufferObject.Bind();
	}

	void OGLReadPixelBuffer::VCopyFromSystemMemory(void* bufferInSystemMemory, int destinationOffsetInBytes, int lengthInBytes)
	{
		m_BufferObject.CopyFromSystemMemory(bufferInSystemMemory, destinationOffsetInBytes, lengthInBytes);
	}

	std::shared_ptr<void> OGLReadPixelBuffer::VCopyToSystemMemory(int offsetInBytes, int sizeInBytes)
	{
		return m_BufferObject.CopyToSystemMemory(offsetInBytes, sizeInBytes);
	}

	int	OGLReadPixelBuffer::VGetSizeInBytes() const
	{
		return m_BufferObject.GetSizeInBytes();
	}

	PixelBufferHint	OGLReadPixelBuffer::VGetUsageHint() const
	{
		return m_UsageHint;
	}

}

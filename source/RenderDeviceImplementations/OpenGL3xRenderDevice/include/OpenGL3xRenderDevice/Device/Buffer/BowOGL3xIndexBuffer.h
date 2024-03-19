#pragma once
#include <OpenGL3xRenderDevice/OpenGL3xRenderDevice_api.h>
#include <OpenGL3xRenderDevice/Device/Buffer/BowOGL3xBuffer.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>
#include <RenderDevice/Device/Buffer/IBowIndexBuffer.h>
#include <RenderDevice/Device/Buffer/BowIndexBufferDatatype.h>

namespace bow {

	class OGLIndexBuffer : public IIndexBuffer
	{
	public:
		OGLIndexBuffer(BufferHint usageHint, IndexBufferDatatype dataType, int sizeInBytes);
		~OGLIndexBuffer();

		void Bind();
		static void UnBind();
		int GetCount();

		void VCopyFromSystemMemory(void* bufferInSystemMemory, int destinationOffsetInBytes, int lengthInBytes);
		std::shared_ptr<void> VCopyToSystemMemory(int offsetInBytes, int sizeInBytes);

		int VGetSizeInBytes();
		BufferHint VGetUsageHint();
		IndexBufferDatatype GetDatatype() { return m_Datatype; }

	private:
		OGLBuffer	m_BufferObject;
		IndexBufferDatatype m_Datatype;
	};

	typedef std::shared_ptr<OGLIndexBuffer> OGLIndexBufferPtr;

}

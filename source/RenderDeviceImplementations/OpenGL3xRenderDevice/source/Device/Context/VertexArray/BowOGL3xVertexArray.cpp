#include <OpenGL3xRenderDevice/Device/Context/VertexArray/BowOGL3xVertexArray.h>
#include <OpenGL3xRenderDevice/Device/Buffer/BowOGL3xVertexBuffer.h>
#include <OpenGL3xRenderDevice/Device/Buffer/BowOGL3xIndexBuffer.h>
#include <RenderDevice/Device/Shader/BowShaderVertexAttribute.h>
#include <CoreSystems/BowLogger.h>

#include <GL/glew.h>
#if defined(_WIN32)
#include <GL/wglew.h>
#endif

namespace bow {

	OGLVertexArray::OGLVertexArray() : m_dirtyIndexBuffer(false), m_VertexArrayHandle(0)
	{
		glGenVertexArrays(1, &m_VertexArrayHandle);
	}

	OGLVertexArray::~OGLVertexArray()
	{
		if (m_VertexArrayHandle != 0)
		{
			glDeleteVertexArrays(1, &m_VertexArrayHandle);
			m_VertexArrayHandle = 0;
		}
	}

	void OGLVertexArray::Bind()
	{
		glBindVertexArray(m_VertexArrayHandle);
	}

	void OGLVertexArray::Clean()
	{
		m_Attributes.Clean();

		if (m_dirtyIndexBuffer)
		{
			if (m_indexBuffer.get() != nullptr)
			{
				m_indexBuffer->Bind();
			}
			else
			{
				OGLIndexBuffer::UnBind();
			}

			m_dirtyIndexBuffer = false;
		}
	}

	int OGLVertexArray::MaximumArrayIndex()
	{
		return m_Attributes.GetMaximumArrayIndex();
	}

	VertexBufferAttributeMap OGLVertexArray::VGetAttributes()
	{
		return m_Attributes.GetAttributes();
	}

	void OGLVertexArray::VSetAttribute(unsigned int location, VertexBufferAttributePtr pointer)
	{
		m_Attributes.SetAttribute(location, pointer);
	}

	void OGLVertexArray::VSetAttribute(ShaderVertexAttributePtr vertexAttribute, VertexBufferAttributePtr pointer)
	{
		m_Attributes.SetAttribute(vertexAttribute->Location, pointer);
	}

	IndexBufferPtr OGLVertexArray::VGetIndexBuffer()
	{
		return std::dynamic_pointer_cast<IIndexBuffer>(m_indexBuffer);
	}

	void OGLVertexArray::VSetIndexBuffer(IndexBufferPtr indexBufferPtr)
	{
		m_indexBuffer = std::dynamic_pointer_cast<OGLIndexBuffer>(indexBufferPtr);
		m_dirtyIndexBuffer = true;
	}

}

#include <OpenGL3xRenderDevice/Device/Buffer/BowOGL3xBufferName.h>

#include <GL/glew.h>
#if defined(_WIN32)
#include <GL/wglew.h>
#endif

namespace bow {

	OGLBufferName::OGLBufferName()
	{
		m_value = 0;
		glGenBuffers(1, &m_value);
	}

	OGLBufferName::~OGLBufferName()
	{
		if (m_value != 0)
		{
			glDeleteBuffers(1, &m_value);
			m_value = 0;
		}
	}

	unsigned int OGLBufferName::GetValue()
	{
		return m_value;
	}

}

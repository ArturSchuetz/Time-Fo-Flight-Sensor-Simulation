#include <OpenGL3xRenderDevice/Device/Textures/BowOGL3xTextureSampler.h>
#include <OpenGL3xRenderDevice/BowOGL3xTypeConverter.h>
#include <CoreSystems/BowLogger.h>

#include <GL/glew.h>
#if defined(_WIN32)
#include <GL/wglew.h>
#endif

namespace bow {

	OGLTextureSampler::OGLTextureSampler(TextureMinificationFilter minificationFilter, TextureMagnificationFilter magnificationFilter, TextureWrap wrapS, TextureWrap wrapT, float maximumAnistropy)
		: ITextureSampler(minificationFilter, magnificationFilter, wrapS, wrapT, maximumAnistropy), m_SamplerHandle(0)
	{
		m_SamplerHandle = 0;
		glGenSamplers(1, &m_SamplerHandle);

		int glMinificationFilter = (int)OGLTypeConverter::To(minificationFilter);
		int glMagnificationFilter = (int)OGLTypeConverter::To(magnificationFilter);
		int glWrapS = (int)OGLTypeConverter::To(wrapS);
		int glWrapT = (int)OGLTypeConverter::To(wrapT);

		glSamplerParameteri(m_SamplerHandle, GL_TEXTURE_MIN_FILTER, glMinificationFilter);
		glSamplerParameteri(m_SamplerHandle, GL_TEXTURE_MAG_FILTER, glMagnificationFilter);
		glSamplerParameteri(m_SamplerHandle, GL_TEXTURE_WRAP_S, glWrapS);
		glSamplerParameteri(m_SamplerHandle, GL_TEXTURE_WRAP_T, glWrapT);

		/*
		if (Device.Extensions.AnisotropicFiltering)
		{
		glSamplerParameteri(m_name->GetValue(), GL_TEXTURE_MAX_ANISOTROPY_EXT, maximumAnistropy);
		}
		else
		{
		LOG_ASSERT(!(maximumAnistropy != 1), "Anisotropic filtering is not supported.  The extension GL_EXT_texture_filter_anisotropic was not found.");
		}
		*/
	}

	OGLTextureSampler::~OGLTextureSampler()
	{
		if (m_SamplerHandle != 0)
		{
			glDeleteSamplers(1, &m_SamplerHandle);
			m_SamplerHandle = 0;
		}
	}

	void OGLTextureSampler::Bind(int textureUnitIndex)
	{
		glBindSampler(textureUnitIndex, m_SamplerHandle);
	}

	void OGLTextureSampler::UnBind(int textureUnitIndex)
	{
		glBindSampler(textureUnitIndex, 0);
	}

}

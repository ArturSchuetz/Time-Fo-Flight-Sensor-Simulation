#pragma once
#include <OpenGL3xRenderDevice/OpenGL3xRenderDevice_api.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>

namespace bow {

	typedef unsigned int GLenum;

	class OGLShaderObject
	{
	public:
		OGLShaderObject(GLenum shaderType, std::string source);
		~OGLShaderObject();

		std::string		GetCompileLog();
		unsigned int	GetShader();
		bool			IsReady();

	private:
		unsigned int m_shaderObject;
		int			 m_result;
	};

	typedef std::shared_ptr<OGLShaderObject> OGLShaderObjectPtr;

}

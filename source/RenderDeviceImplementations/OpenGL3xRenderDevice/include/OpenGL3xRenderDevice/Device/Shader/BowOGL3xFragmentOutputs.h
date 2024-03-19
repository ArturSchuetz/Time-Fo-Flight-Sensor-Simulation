#pragma once
#include <OpenGL3xRenderDevice/OpenGL3xRenderDevice_api.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>

#include <RenderDevice/Device/Shader/IBowFragmentOutputs.h>

namespace bow {

	class OGLFragmentOutputs : public IFragmentOutputs
	{
	public:
		OGLFragmentOutputs(unsigned int program);
		~OGLFragmentOutputs();

		int operator[](std::string index) const;

	private:
		unsigned int m_ShaderProgramHandle;
	};

	typedef std::shared_ptr<OGLFragmentOutputs> OGLFragmentOutputsPtr;

}

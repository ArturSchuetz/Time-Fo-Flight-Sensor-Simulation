#include <OpenGL3xRenderDevice/Device/Buffer/BowOGL3xIndexBuffer.h>
#include <OpenGL3xRenderDevice/Device/Context/FrameBuffer/BowOGL3xFramebuffer.h>
#include <OpenGL3xRenderDevice/Device/Context/VertexArray/BowOGL3xVertexArray.h>
#include <OpenGL3xRenderDevice/Device/Shader/BowOGL3xShaderProgram.h>
#include <OpenGL3xRenderDevice/Device/Textures/BowOGL3xTexture2D.h>
#include <OpenGL3xRenderDevice/Device/Textures/BowOGL3xTextureUnits.h>
#include <OpenGL3xRenderDevice/Device/Textures/BowOGL3xTextureSampler.h>
#include <OpenGL3xRenderDevice/Device/BowOGL3xRenderContext.h>
#include <OpenGL3xRenderDevice/BowOGL3xTypeConverter.h>
#include <RenderDevice/Device/Context/Mesh/BowMeshBuffers.h>
#include <RenderDevice/Device/Context/VertexArray/BowVertexBufferAttribute.h>
#include <RenderDevice/BowClearState.h>
#include <CoreSystems/Geometry/BowMeshAttribute.h>
#include <CoreSystems/BowLogger.h>

#include <GL/glew.h>
#if defined(_WIN32)
#include <GL/wglew.h>
#endif
#include <GLFW/glfw3.h>

#include <OpenGL3xRenderDevice/Device/GUI/imgui.h>
#include <OpenGL3xRenderDevice/Device/GUI/imgui_impl_glfw.h>
#include <OpenGL3xRenderDevice/Device/GUI/imgui_impl_opengl3.h>

namespace bow {

	OGLRenderContext* OGLRenderContext::m_currentContext;

	OGLRenderContext::OGLRenderContext(GLFWwindow* window) :
		m_window(window),
		m_initialized(false),
		m_vsync(false)
	{

	}


	bool OGLRenderContext::Initialize(OGLRenderDevice* device)
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		if (GLEW_OK != glewInit())
		{
			LOG_ERROR("Could not initialize GLEW!");
		}
		else
		{
			LOG_TRACE("GLEW sucessfully initialized!");
		}

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		const char* glsl_version = "#version 130";
		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForOpenGL(m_window, true);
		if (!ImGui_ImplOpenGL3_Init(glsl_version))
		{
			LOG_ERROR("Could not initialize ImGui!");
			return false;
		}
		else
		{
			LOG_TRACE("ImGui sucessfully initialized!");
		}

		Vector4<float> clearColor;
		glGetFloatv(GL_DEPTH_CLEAR_VALUE, &m_clearDepth);
		glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &m_clearStencil);
		glGetFloatv(GL_COLOR_CLEAR_VALUE, (GLfloat*)&clearColor);

		m_clearColor = clearColor * 255.0f;

		m_textureUnits = OGLTextureUnitsPtr(new OGLTextureUnits());

		//
		// Sync GL state with default render state.
		//
		ForceApplyRenderState(m_renderState);

		int width, height;
		glfwGetFramebufferSize(m_window, &width, &height);
		VSetViewport(Viewport(0, 0, width, height));

		LOG_TRACE("OpenGL-Context sucessfully initialized!");

		//Checking GL version
		const GLubyte *GLVersionString = glGetString(GL_VERSION);
		LOG_TRACE("Using GL_VERSION: %s", GLVersionString);

		glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
		glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);
		glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);

		m_device = device;
		m_initialized = true;
		return m_initialized;
	}


	OGLRenderContext::~OGLRenderContext(void)
	{
		VRelease();
	}


	void OGLRenderContext::VRelease(void)
	{
		if (m_initialized)
		{
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}

		m_initialized = false;
		m_window = nullptr;
		LOG_TRACE("OGLRenderContext released");
	}


	VertexArrayPtr OGLRenderContext::VCreateVertexArray(MeshAttribute mesh, ShaderVertexAttributeMap shaderAttributes, BufferHint usageHint)
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		return VCreateVertexArray(m_device->VCreateMeshBuffers(mesh, shaderAttributes, usageHint));
	}


	VertexArrayPtr OGLRenderContext::VCreateVertexArray(MeshBufferPtr meshBuffers)
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		VertexArrayPtr vertexArray = VCreateVertexArray();
		if (meshBuffers->IndexBuffer != nullptr)
		{
			vertexArray->VSetIndexBuffer(meshBuffers->IndexBuffer);
		}

		VertexBufferAttributeMap attributeMap = meshBuffers->GetAttributes();
		for (auto attribute = attributeMap.begin(); attribute != attributeMap.end(); ++attribute)
		{
			vertexArray->VSetAttribute(attribute->first, attribute->second);
		}

		return vertexArray;
	}


	VertexArrayPtr OGLRenderContext::VCreateVertexArray()
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		return VertexArrayPtr(new OGLVertexArray());
	}


	FramebufferPtr OGLRenderContext::VCreateFramebuffer()
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		return OGLFramebufferPtr(new OGLFramebuffer());
	}


	void OGLRenderContext::VClear(ClearState clearState)
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		ApplyFramebuffer();

		ApplyScissorTest(clearState.scissorTest);
		ApplyColorMask(clearState.colorMask);
		ApplyDepthMask(clearState.depthMask);
		// TODO: StencilMaskSeparate

		if (m_clearColor != clearState.color)
		{
			glClearColor(clearState.color.a[0], clearState.color.a[1], clearState.color.a[2], clearState.color.a[3]);
			m_clearColor = clearState.color;
		}

		if (m_clearDepth != clearState.depth)
		{
			glClearDepth((float)clearState.depth);
			m_clearDepth = clearState.depth;
		}

		if (m_clearStencil != clearState.stencil)
		{
			glClearStencil(clearState.stencil);
			m_clearStencil = clearState.stencil;
		}

		glClear(OGLTypeConverter::To(clearState.buffers));
	}


	void OGLRenderContext::VDraw(PrimitiveType primitiveType, VertexArrayPtr vertexArray, ShaderProgramPtr shaderProgram, RenderState renderState)
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		ApplyRenderState(renderState);
		ApplyVertexArray(vertexArray);
		ApplyShaderProgram(shaderProgram);

		Draw(primitiveType, 0, std::dynamic_pointer_cast<OGLVertexArray>(vertexArray)->MaximumArrayIndex() + 1, vertexArray, shaderProgram, renderState);
	}


	void OGLRenderContext::VDraw(PrimitiveType primitiveType, int offset, int count, VertexArrayPtr vertexArray, ShaderProgramPtr shaderProgram, RenderState renderState)
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		ApplyRenderState(renderState);
		ApplyVertexArray(vertexArray);
		ApplyShaderProgram(shaderProgram);

		Draw(primitiveType, offset, count, vertexArray, shaderProgram, renderState);
	}


	void OGLRenderContext::VDrawLine(const bow::Vector3<float> &start, const bow::Vector3<float> &end)
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		m_textureUnits->Clean();

		glFlush();
		ApplyFramebuffer();

		glBegin(GL_LINES);
		glVertex3f(start.x, start.y, start.z);
		glVertex3f(end.x, end.y, end.z);
		glEnd();
	}


	void OGLRenderContext::Draw(PrimitiveType primitiveType, int offset, int count, VertexArrayPtr vertexArray, ShaderProgramPtr shaderProgram, RenderState renderState)
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		m_textureUnits->Clean();

		glFlush();
		ApplyFramebuffer();

		OGLVertexArrayPtr oglVertexArray = std::dynamic_pointer_cast<OGLVertexArray>(vertexArray);
		OGLIndexBufferPtr oglIndexBuffer = std::dynamic_pointer_cast<OGLIndexBuffer>(oglVertexArray->VGetIndexBuffer());

		if (oglIndexBuffer != nullptr)
		{
			if (offset == 0 && count == oglVertexArray->MaximumArrayIndex() + 1)
			{
				glDrawRangeElements(OGLTypeConverter::To(primitiveType), 0, oglVertexArray->MaximumArrayIndex(), oglIndexBuffer->GetCount(), OGLTypeConverter::To(oglIndexBuffer->GetDatatype()), 0);
			}
			else
			{
				glDrawRangeElements(OGLTypeConverter::To(primitiveType), 0, oglVertexArray->MaximumArrayIndex(), count, OGLTypeConverter::To(oglIndexBuffer->GetDatatype()), (void*)(offset * sizeof(unsigned int)));
			}
		}
		else
		{
			glDrawArrays(OGLTypeConverter::To(primitiveType), offset, count);
		}
	}


	void OGLRenderContext::VSetTexture(int location, Texture2DPtr texture)
	{
		LOG_ASSERT(location < m_textureUnits->GetMaxTextureUnits(), "TextureUnit does not Exist");

		m_textureUnits->SetTexture(location, std::dynamic_pointer_cast<OGLTexture2D>(texture));
	}

	void OGLRenderContext::VSetTextureSampler(int location, TextureSamplerPtr sampler)
	{
		m_textureUnits->SetSampler(location, std::dynamic_pointer_cast<OGLTextureSampler>(sampler));
	}


	void OGLRenderContext::VSetFramebuffer(FramebufferPtr framebufer)
	{
		m_setFramebuffer = std::dynamic_pointer_cast<OGLFramebuffer>(framebufer);
	}


	void OGLRenderContext::VSetViewport(Viewport viewport)
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		LOG_ASSERT(!(viewport.width < 0 || viewport.height < 0), "The viewport width and height must be greater than or equal to zero.");

		if (m_viewport != viewport)
		{
			m_viewport = viewport;
			glViewport(m_viewport.x, m_viewport.y, m_viewport.width, m_viewport.height);
		}
	}


	Viewport OGLRenderContext::VGetViewport(void)
	{
		return m_viewport;
	}


	void OGLRenderContext::VSwapBuffers(bool vsync)
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		if (m_vsync != vsync)
		{
			if (vsync)
			{
				glfwSwapInterval(1);
			}
			else
			{
				glfwSwapInterval(0);
			}
			m_vsync = vsync;
		}

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(m_window);
	}


	void OGLRenderContext::ApplyVertexArray(VertexArrayPtr vertexArray)
	{
		OGLVertexArrayPtr oglVertexArray = std::dynamic_pointer_cast<OGLVertexArray>(vertexArray);
		oglVertexArray->Bind();
		oglVertexArray->Clean();
	}


	void OGLRenderContext::ApplyShaderProgram(ShaderProgramPtr shaderProgram)
	{
		OGLShaderProgramPtr oglShaderProgram = std::dynamic_pointer_cast<OGLShaderProgram>(shaderProgram);

		if (m_boundShaderProgram != oglShaderProgram)
		{
			oglShaderProgram->Bind();
			m_boundShaderProgram = oglShaderProgram;
		}
		m_boundShaderProgram->Clean();

#if _DEBUG
		glValidateProgram(m_boundShaderProgram->GetProgram());

		int validateStatus;
		glGetProgramiv(m_boundShaderProgram->GetProgram(), GL_VALIDATE_STATUS, &validateStatus);
		if (validateStatus == 0)
		{
			LOG_ERROR("Shader program validation failed: %s", m_boundShaderProgram->GetLog().c_str());
		}
#endif
	}


	void OGLRenderContext::ApplyFramebuffer()
	{
		if (m_boundFramebuffer != m_setFramebuffer)
		{
			if (m_setFramebuffer != nullptr)
			{
				m_setFramebuffer->Bind();
			}
			else
			{
				OGLFramebuffer::UnBind();
			}

			m_boundFramebuffer = m_setFramebuffer;
		}

		if (m_setFramebuffer != nullptr)
		{
			m_setFramebuffer->Clean();
#if _DEBUG
			GLenum errorCode = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			LOG_ASSERT(!(errorCode != GL_FRAMEBUFFER_COMPLETE), "Frame buffer is incomplete.");
#endif
		}
	}

}

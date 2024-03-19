#include <OpenGL3xRenderDevice/Device/Buffer/BowOGL3xIndexBuffer.h>
#include <OpenGL3xRenderDevice/Device/Context/FrameBuffer/BowOGL3xFramebuffer.h>
#include <OpenGL3xRenderDevice/Device/Context/VertexArray/BowOGL3xVertexArray.h>
#include <OpenGL3xRenderDevice/Device/Shader/BowOGL3xShaderProgram.h>
#include <OpenGL3xRenderDevice/Device/Textures/BowOGL3xTexture2D.h>
#include <OpenGL3xRenderDevice/Device/Textures/BowOGL3xTextureUnits.h>
#include <OpenGL3xRenderDevice/Device/Textures/BowOGL3xTextureSampler.h>
#include <OpenGL3xRenderDevice/Device/BowOGL3xRenderContext.h>
#include <OpenGL3xRenderDevice/BowOGL3xTypeConverter.h>
#include <RenderDevice/BowClearState.h>
#include <CoreSystems/BowLogger.h>

#include <GL/glew.h>
#if defined(_WIN32)
#include <GL/wglew.h>
#endif
#include <GLFW/glfw3.h>

namespace bow {

	enum class StencilFace : GLenum
	{
		Front = GL_FRONT,
		Back = GL_BACK,
		FrontAndBack = GL_FRONT_AND_BACK
	};

	enum class MaterialFace : GLenum
	{
		Front = GL_FRONT,
		Back = GL_BACK,
		FrontAndBack = GL_FRONT_AND_BACK
	};


	void Enable(GLenum enableCap, bool enable)
	{
		if (enable)
		{
			glEnable(enableCap);
		}
		else
		{
			glDisable(enableCap);
		}
	}


	void OGLRenderContext::ForceApplyRenderState(RenderState renderState)
	{
		glPointSize(renderState.pointSize);
		glPointSize(renderState.lineWidth);

		Enable(GL_PRIMITIVE_RESTART, renderState.primitiveRestart.Enabled);
		glPrimitiveRestartIndex(renderState.primitiveRestart.Index);

		Enable(GL_CULL_FACE, renderState.faceCulling.Enabled);
		glCullFace(OGLTypeConverter::To(renderState.faceCulling.Face));
		glFrontFace(OGLTypeConverter::To(renderState.faceCulling.FrontFaceWindingOrder));

		Enable(GL_PROGRAM_POINT_SIZE, renderState.programPointSize == ProgramPointSize::Enabled);
		glPolygonMode((GLenum)MaterialFace::FrontAndBack, OGLTypeConverter::To(renderState.rasterizationMode));

		Enable(GL_SCISSOR_TEST, renderState.scissorTest.Enabled);
		long rectangle_left = renderState.scissorTest.rectangle_left;
		long rectangle_top = renderState.scissorTest.rectangle_top;
		long rectangle_right = renderState.scissorTest.rectangle_right;
		long rectangle_bottom = renderState.scissorTest.rectangle_bottom;
		glScissor(rectangle_left, rectangle_top, rectangle_right - rectangle_left, rectangle_bottom - rectangle_top);

		Enable(GL_STENCIL_TEST, renderState.stencilTest.Enabled);

		ForceApplyRenderStateStencil(StencilFace::Front, renderState.stencilTest.FrontFace);
		ForceApplyRenderStateStencil(StencilFace::Back, renderState.stencilTest.BackFace);

		Enable(GL_DEPTH_TEST, renderState.depthTest.Enabled);
		glDepthFunc(OGLTypeConverter::To(renderState.depthTest.Function));

		glDepthRange(renderState.depthRange.Near, renderState.depthRange.Far);

		Enable(GL_BLEND, renderState.blending.Enabled);
		glBlendFuncSeparate(
			OGLTypeConverter::To(renderState.blending.SourceRGBFactor),
			OGLTypeConverter::To(renderState.blending.DestinationRGBFactor),
			OGLTypeConverter::To(renderState.blending.SourceAlphaFactor),
			OGLTypeConverter::To(renderState.blending.DestinationAlphaFactor));
		glBlendEquationSeparate(
			OGLTypeConverter::To(renderState.blending.RGBEquation),
			OGLTypeConverter::To(renderState.blending.AlphaEquation));


		glBlendColor(renderState.blending.color[0], renderState.blending.color[1], renderState.blending.color[2], renderState.blending.color[3]);

		glDepthMask(renderState.depthMask);

		glColorMask(renderState.colorMask.GetRed(), renderState.colorMask.GetGreen(), renderState.colorMask.GetBlue(), renderState.colorMask.GetAlpha());
	}


	void OGLRenderContext::ForceApplyRenderStateStencil(StencilFace face, StencilTestFace test)
	{
		glStencilOpSeparate((GLenum)face,
			OGLTypeConverter::To(test.StencilFailOperation),
			OGLTypeConverter::To(test.DepthFailStencilPassOperation),
			OGLTypeConverter::To(test.DepthPassStencilPassOperation));

		glStencilFuncSeparate((GLenum)face,
			OGLTypeConverter::To(test.Function),
			test.ReferenceValue,
			test.Mask);
	}


	void OGLRenderContext::ApplyRenderState(RenderState renderState)
	{
		if (m_renderState.pointSize != renderState.pointSize)
		{
			m_renderState.pointSize = renderState.pointSize;
			glPointSize(renderState.pointSize);
		}

		if (m_renderState.lineWidth != renderState.lineWidth)
		{
			m_renderState.lineWidth = renderState.lineWidth;
			glLineWidth(renderState.lineWidth);
		}

		ApplyPrimitiveRestart(renderState.primitiveRestart);
		ApplyFaceCulling(renderState.faceCulling);
		ApplyProgramPointSize(renderState.programPointSize);
		ApplyRasterizationMode(renderState.rasterizationMode);
		ApplyScissorTest(renderState.scissorTest);
		ApplyStencilTest(renderState.stencilTest);
		ApplyDepthTest(renderState.depthTest);
		ApplyDepthRange(renderState.depthRange);
		ApplyBlending(renderState.blending);
		ApplyColorMask(renderState.colorMask);
		ApplyDepthMask(renderState.depthMask);
	}


	void OGLRenderContext::ApplyPrimitiveRestart(PrimitiveRestart primitiveRestart)
	{
		if (m_renderState.primitiveRestart.Enabled != primitiveRestart.Enabled)
		{
			Enable(GL_PRIMITIVE_RESTART, primitiveRestart.Enabled);
			m_renderState.primitiveRestart.Enabled = primitiveRestart.Enabled;
		}

		if (primitiveRestart.Enabled)
		{
			if (m_renderState.primitiveRestart.Index != primitiveRestart.Index)
			{
				glPrimitiveRestartIndex(primitiveRestart.Index);
				m_renderState.primitiveRestart.Index = primitiveRestart.Index;
			}
		}
	}


	void OGLRenderContext::ApplyFaceCulling(FaceCulling FaceCulling)
	{
		if (m_renderState.faceCulling.Enabled != FaceCulling.Enabled)
		{
			Enable(GL_CULL_FACE, FaceCulling.Enabled);
			m_renderState.faceCulling.Enabled = FaceCulling.Enabled;
		}

		if (FaceCulling.Enabled)
		{
			if (m_renderState.faceCulling.Face != FaceCulling.Face)
			{
				glCullFace(OGLTypeConverter::To(FaceCulling.Face));
				m_renderState.faceCulling.Face = FaceCulling.Face;
			}

			if (m_renderState.faceCulling.FrontFaceWindingOrder != FaceCulling.FrontFaceWindingOrder)
			{
				glFrontFace(OGLTypeConverter::To(FaceCulling.FrontFaceWindingOrder));
				m_renderState.faceCulling.FrontFaceWindingOrder = FaceCulling.FrontFaceWindingOrder;
			}
		}
	}


	void OGLRenderContext::ApplyProgramPointSize(ProgramPointSize programPointSize)
	{
		if (m_renderState.programPointSize != programPointSize)
		{
			Enable(GL_PROGRAM_POINT_SIZE, programPointSize == ProgramPointSize::Enabled);
			m_renderState.programPointSize = programPointSize;
		}
	}


	void OGLRenderContext::ApplyRasterizationMode(RasterizationMode rasterizationMode)
	{
		if (m_renderState.rasterizationMode != rasterizationMode)
		{
			glPolygonMode((GLenum)MaterialFace::FrontAndBack, OGLTypeConverter::To(rasterizationMode));
			m_renderState.rasterizationMode = rasterizationMode;
		}
	}


	void OGLRenderContext::ApplyScissorTest(ScissorTest scissorTest)
	{
		if (scissorTest.rectangle_right - scissorTest.rectangle_left < 0)
		{
			LOG_ASSERT(false, "renderState.ScissorTest.Rectangle.Width must be greater than or equal to zero");
			return;
		}

		if (scissorTest.rectangle_bottom - scissorTest.rectangle_top < 0)
		{
			LOG_ASSERT(false, "renderState.ScissorTest.Rectangle.Height must be greater than or equal to zero");
			return;
		}

		if (m_renderState.scissorTest.Enabled != scissorTest.Enabled)
		{
			Enable(GL_SCISSOR_TEST, scissorTest.Enabled);
			m_renderState.scissorTest.Enabled = scissorTest.Enabled;
		}

		if (scissorTest.Enabled)
		{
			if (!(m_renderState.scissorTest.rectangle_bottom == scissorTest.rectangle_bottom && m_renderState.scissorTest.rectangle_left == scissorTest.rectangle_left &&m_renderState.scissorTest.rectangle_right == scissorTest.rectangle_right &&m_renderState.scissorTest.rectangle_top == scissorTest.rectangle_top))
			{
				glScissor(scissorTest.rectangle_left, scissorTest.rectangle_bottom, scissorTest.rectangle_right - scissorTest.rectangle_left, scissorTest.rectangle_bottom - scissorTest.rectangle_top);
				m_renderState.scissorTest.rectangle_left = scissorTest.rectangle_left;
				m_renderState.scissorTest.rectangle_top = scissorTest.rectangle_top;
				m_renderState.scissorTest.rectangle_right = scissorTest.rectangle_right;
				m_renderState.scissorTest.rectangle_bottom = scissorTest.rectangle_bottom;
			}
		}
	}


	void OGLRenderContext::ApplyStencilTest(StencilTest stencilTest)
	{
		if (m_renderState.stencilTest.Enabled != stencilTest.Enabled)
		{
			Enable(GL_STENCIL_TEST, stencilTest.Enabled);
			m_renderState.stencilTest.Enabled = stencilTest.Enabled;
		}

		if (stencilTest.Enabled)
		{
			ApplyStencil(StencilFace::Front, m_renderState.stencilTest.FrontFace, stencilTest.FrontFace);
			ApplyStencil(StencilFace::Back, m_renderState.stencilTest.BackFace, stencilTest.BackFace);
		}
	}


	void OGLRenderContext::ApplyStencil(StencilFace face, StencilTestFace currentTest, StencilTestFace test)
	{
		if ((currentTest.StencilFailOperation != test.StencilFailOperation) ||
			(currentTest.DepthFailStencilPassOperation != test.DepthFailStencilPassOperation) ||
			(currentTest.DepthPassStencilPassOperation != test.DepthPassStencilPassOperation))
		{
			glStencilOpSeparate((GLenum)face,
				OGLTypeConverter::To(test.StencilFailOperation),
				OGLTypeConverter::To(test.DepthFailStencilPassOperation),
				OGLTypeConverter::To(test.DepthPassStencilPassOperation));

			currentTest.StencilFailOperation = test.StencilFailOperation;
			currentTest.DepthFailStencilPassOperation = test.DepthFailStencilPassOperation;
			currentTest.DepthPassStencilPassOperation = test.DepthPassStencilPassOperation;
		}

		if ((currentTest.Function != test.Function) ||
			(currentTest.ReferenceValue != test.ReferenceValue) ||
			(currentTest.Mask != test.Mask))
		{
			glStencilFuncSeparate((GLenum)face,
				OGLTypeConverter::To(test.Function),
				test.ReferenceValue,
				test.Mask);

			currentTest.Function = test.Function;
			currentTest.ReferenceValue = test.ReferenceValue;
			currentTest.Mask = test.Mask;
		}
	}

	void OGLRenderContext::ApplyDepthTest(DepthTest depthTest)
	{
		if (m_renderState.depthTest.Enabled != depthTest.Enabled)
		{
			Enable(GL_DEPTH_TEST, depthTest.Enabled);
			m_renderState.depthTest.Enabled = depthTest.Enabled;
		}

		if (depthTest.Enabled)
		{
			if (m_renderState.depthTest.Function != depthTest.Function)
			{
				glDepthFunc(OGLTypeConverter::To(depthTest.Function));
				m_renderState.depthTest.Function = depthTest.Function;
			}
		}
	}


	void OGLRenderContext::ApplyDepthRange(DepthRange depthRange)
	{
		if (depthRange.Near < 0.0 || depthRange.Near > 1.0)
		{
			LOG_ASSERT(false, "renderState.DepthRange.Near must be between zero and one");
			return;
		}

		if (depthRange.Far < 0.0 || depthRange.Far > 1.0)
		{
			LOG_ASSERT(false, "renderState.DepthRange.Far must be between zero and one");
			return;
		}

		if ((m_renderState.depthRange.Near != depthRange.Near) ||
			(m_renderState.depthRange.Far != depthRange.Far))
		{
			glDepthRange(depthRange.Near, depthRange.Far);

			m_renderState.depthRange.Near = depthRange.Near;
			m_renderState.depthRange.Far = depthRange.Far;
		}
	}


	void OGLRenderContext::ApplyBlending(Blending blending)
	{
		if (m_renderState.blending.Enabled != blending.Enabled)
		{
			Enable(GL_BLEND, blending.Enabled);
			m_renderState.blending.Enabled = blending.Enabled;
		}

		if (blending.Enabled)
		{
			if ((m_renderState.blending.SourceRGBFactor != blending.SourceRGBFactor) ||
				(m_renderState.blending.DestinationRGBFactor != blending.DestinationRGBFactor) ||
				(m_renderState.blending.SourceAlphaFactor != blending.SourceAlphaFactor) ||
				(m_renderState.blending.DestinationAlphaFactor != blending.DestinationAlphaFactor))
			{
				glBlendFuncSeparate(
					OGLTypeConverter::To(blending.SourceRGBFactor),
					OGLTypeConverter::To(blending.DestinationRGBFactor),
					OGLTypeConverter::To(blending.SourceAlphaFactor),
					OGLTypeConverter::To(blending.DestinationAlphaFactor));

				m_renderState.blending.SourceRGBFactor = blending.SourceRGBFactor;
				m_renderState.blending.DestinationRGBFactor = blending.DestinationRGBFactor;
				m_renderState.blending.SourceAlphaFactor = blending.SourceAlphaFactor;
				m_renderState.blending.DestinationAlphaFactor = blending.DestinationAlphaFactor;
			}

			if ((m_renderState.blending.RGBEquation != blending.RGBEquation) ||
				(m_renderState.blending.AlphaEquation != blending.AlphaEquation))
			{
				glBlendEquationSeparate(
					OGLTypeConverter::To(blending.RGBEquation),
					OGLTypeConverter::To(blending.AlphaEquation));

				m_renderState.blending.RGBEquation = blending.RGBEquation;
				m_renderState.blending.AlphaEquation = blending.AlphaEquation;
			}

			if (m_renderState.blending.color != blending.color)
			{
				glBlendColor(blending.color[0], blending.color[1], blending.color[2], blending.color[3]);
				memcpy(&m_renderState.blending.color, &blending.color, sizeof(float) * 4);
			}
		}
	}

	void OGLRenderContext::ApplyColorMask(ColorMask colorMask)
	{
		if (m_renderState.colorMask != colorMask)
		{
			glColorMask(colorMask.GetRed(), colorMask.GetGreen(), colorMask.GetBlue(), colorMask.GetAlpha());
			m_renderState.colorMask = colorMask;
		}
	}


	void OGLRenderContext::ApplyDepthMask(bool depthMask)
	{
		if (m_renderState.depthMask != depthMask)
		{
			glDepthMask(depthMask);
			m_renderState.depthMask = depthMask;
		}
	}
}

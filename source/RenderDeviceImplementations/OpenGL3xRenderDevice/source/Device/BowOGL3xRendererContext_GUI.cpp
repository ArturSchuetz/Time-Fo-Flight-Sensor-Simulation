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

#define IMGUI_IMPL_OPENGL_LOADER_GLEW 1
#include <OpenGL3xRenderDevice/Device/GUI/imgui.h>
#include <OpenGL3xRenderDevice/Device/GUI/imgui_impl_glfw.h>
#include <OpenGL3xRenderDevice/Device/GUI/imgui_impl_opengl3.h>

namespace bow {

	void OGLRenderContext::GUI_NewFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void OGLRenderContext::GUI_EndFrame()
	{
		ImGui::EndFrame();
	}

	void OGLRenderContext::GUI_Render()
	{
		ImGui::Render();
	}

	void OGLRenderContext::GUI_Begin(const char* name, bool* p_open)
	{
		ImGui::Begin(name, p_open);
	}

	void OGLRenderContext::GUI_End()
	{
		ImGui::End();
	}

	void OGLRenderContext::GUI_TextUnformatted(const char* text, const char* text_end)
	{
		return ImGui::TextUnformatted(text, text_end);
	}

	void OGLRenderContext::GUI_Text(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ImGui::TextV(fmt, args);
		va_end(args);
	}

	void OGLRenderContext::GUI_TextV(const char* fmt, va_list args)
	{
		return ImGui::TextV(fmt, args);
	}

	void OGLRenderContext::GUI_TextColored(const bow::Vector4<float>& col, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ImGui::TextColoredV(ImVec4(col.x, col.y, col.z, col.w), fmt, args);
		va_end(args);
	}

	void OGLRenderContext::GUI_TextColoredV(const bow::Vector4<float>& col, const char* fmt, va_list args)
	{
		return ImGui::TextColoredV(ImVec4(col.x, col.y, col.z, col.w), fmt, args);
	}

	void OGLRenderContext::GUI_TextDisabled(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ImGui::TextDisabledV(fmt, args);
		va_end(args);
	}

	void OGLRenderContext::GUI_TextDisabledV(const char* fmt, va_list args)
	{
		return ImGui::TextDisabledV(fmt, args);
	}

	void OGLRenderContext::GUI_TextWrapped(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ImGui::TextWrappedV(fmt, args);
		va_end(args);
	}

	void OGLRenderContext::GUI_TextWrappedV(const char* fmt, va_list args)
	{
		return ImGui::TextWrappedV(fmt, args);
	}

	void OGLRenderContext::GUI_LabelText(const char* label, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ImGui::LabelTextV(label, fmt, args);
		va_end(args);
	}

	void OGLRenderContext::GUI_LabelTextV(const char* label, const char* fmt, va_list args)
	{
		return ImGui::LabelTextV(label, fmt, args);
	}

	void OGLRenderContext::GUI_BulletText(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ImGui::BulletTextV(fmt, args);
		va_end(args);
	}

	void OGLRenderContext::GUI_BulletTextV(const char* fmt, va_list args)
	{
		return ImGui::BulletTextV(fmt, args);
	}

	bool OGLRenderContext::GUI_Button(const char* label, const bow::Vector2<float>& size)
	{
		return ImGui::Button(label, ImVec2(size.x, size.y));
	}

	bool OGLRenderContext::GUI_SmallButton(const char* label)
	{
		return ImGui::Button(label);
	}

	bool OGLRenderContext::GUI_InvisibleButton(const char* str_id, const bow::Vector2<float>& size)
	{
		return ImGui::InvisibleButton(str_id, ImVec2(size.x, size.y));
	}

	bool OGLRenderContext::GUI_Checkbox(const char* label, bool* v)
	{
		return ImGui::Checkbox(label, v);
	}

	bool OGLRenderContext::GUI_CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value)
	{
		return ImGui::CheckboxFlags(label, flags, flags_value);
	}

	bool OGLRenderContext::GUI_RadioButton(const char* label, bool active)
	{
		return ImGui::RadioButton(label, active);
	}

	bool OGLRenderContext::GUI_RadioButton(const char* label, int* v, int v_button)
	{
		return ImGui::RadioButton(label, v, v_button);
	}

	void OGLRenderContext::GUI_ProgressBar(float fraction, const bow::Vector2<float>& size_arg, const char* overlay)
	{
		return ImGui::ProgressBar(fraction, ImVec2(size_arg.x, size_arg.y), overlay);
	}

	void OGLRenderContext::GUI_Bullet()
	{
		return ImGui::Bullet();
	}

	bool OGLRenderContext::GUI_SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, float power)
	{
		return ImGui::SliderFloat(label, v, v_min, v_max, format, power);
	}

	bool OGLRenderContext::GUI_SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format, float power)
	{
		return ImGui::SliderFloat2(label, v, v_min, v_max, format, power);
	}

	bool OGLRenderContext::GUI_SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format, float power)
	{
		return ImGui::SliderFloat3(label, v, v_min, v_max, format, power);
	}

	bool OGLRenderContext::GUI_SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format, float power)
	{
		return ImGui::SliderFloat4(label, v, v_min, v_max, format, power);
	}

	bool OGLRenderContext::GUI_SliderAngle(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max, const char* format)
	{
		return ImGui::SliderAngle(label, v_rad, v_degrees_min, v_degrees_max, format);
	}

	bool OGLRenderContext::GUI_SliderInt(const char* label, int* v, int v_min, int v_max, const char* format)
	{
		return ImGui::SliderInt(label, v, v_min, v_max, format);
	}

	bool OGLRenderContext::GUI_SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format)
	{
		return ImGui::SliderInt2(label, v, v_min, v_max, format);
	}

	bool OGLRenderContext::GUI_SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format)
	{
		return ImGui::SliderInt3(label, v, v_min, v_max, format);
	}

	bool OGLRenderContext::GUI_SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format)
	{
		return ImGui::SliderInt4(label, v, v_min, v_max, format);
	}

	void OGLRenderContext::GUI_Separator()
	{
		return ImGui::Separator();
	}

	void OGLRenderContext::GUI_SameLine(float offset_from_start_x, float spacing)
	{
		return ImGui::SameLine(offset_from_start_x, spacing);
	}

	void OGLRenderContext::GUI_NewLine()
	{
		return ImGui::NewLine();
	}

	void OGLRenderContext::GUI_Spacing()
	{
		return ImGui::Spacing();
	}

	void OGLRenderContext::GUI_Indent(float indent_w)
	{
		return ImGui::Indent(indent_w);
	}

	void OGLRenderContext::GUI_Unindent(float indent_w)
	{
		return ImGui::Unindent(indent_w);
	}

	void OGLRenderContext::GUI_BeginGroup()
	{
		return ImGui::BeginGroup();
	}

	void OGLRenderContext::GUI_EndGroup()
	{
		return ImGui::EndGroup();
	}

	float OGLRenderContext::GUI_GetCursorPosX()
	{
		return ImGui::GetCursorPosX();
	}

	float OGLRenderContext::GUI_GetCursorPosY()
	{
		return ImGui::GetCursorPosY();
	}

	void OGLRenderContext::GUI_SetCursorPos(const bow::Vector2<float>& local_pos)
	{
		return ImGui::SetCursorPos(ImVec2(local_pos.x, local_pos.y));
	}

	void OGLRenderContext::GUI_SetCursorPosX(float local_x)
	{
		return ImGui::SetCursorPosX(local_x);
	}

	void OGLRenderContext::GUI_SetCursorPosY(float local_y)
	{
		return ImGui::SetCursorPosY(local_y);
	}

	bow::Vector2<float> OGLRenderContext::GUI_GetCursorStartPos()
	{
		auto vec2 = ImGui::GetCursorStartPos();
		return bow::Vector2<float>(vec2.x, vec2.y);
	}

	bow::Vector2<float> OGLRenderContext::GUI_GetCursorScreenPos()
	{
		auto vec2 = ImGui::GetCursorScreenPos();
		return bow::Vector2<float>(vec2.x, vec2.y);
	}

	void OGLRenderContext::GUI_SetCursorScreenPos(const bow::Vector2<float>& pos)
	{
		return ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y));
	}

	void OGLRenderContext::GUI_AlignTextToFramePadding()
	{
		return ImGui::AlignTextToFramePadding();
	}

	float OGLRenderContext::GUI_GetTextLineHeight()
	{
		return ImGui::GetTextLineHeight();
	}

	float OGLRenderContext::GUI_GetTextLineHeightWithSpacing()
	{
		return ImGui::GetTextLineHeightWithSpacing();
	}

	float OGLRenderContext::GUI_GetFrameHeight()
	{
		return ImGui::GetFrameHeight();
	}

	float OGLRenderContext::GUI_GetFrameHeightWithSpacing()
	{
		return ImGui::GetFrameHeightWithSpacing();
	}
}

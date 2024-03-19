#include <DirectX11RenderDevice/Device/Buffer/BowD3D11IndexBuffer.h>
#include <DirectX11RenderDevice/Device/Context/FrameBuffer/BowD3D11Framebuffer.h>
#include <DirectX11RenderDevice/Device/Context/VertexArray/BowD3D11VertexArray.h>
#include <DirectX11RenderDevice/Device/Shader/BowD3D11ShaderProgram.h>
#include <DirectX11RenderDevice/Device/Textures/BowD3D11Texture2D.h>
#include <DirectX11RenderDevice/Device/Textures/BowD3D11TextureUnits.h>
#include <DirectX11RenderDevice/Device/Textures/BowD3D11TextureSampler.h>
#include <DirectX11RenderDevice/Device/BowD3D11RenderContext.h>
#include <DirectX11RenderDevice/BowD3D11TypeConverter.h>
#include <RenderDevice/Device/Context/Mesh/BowMeshBuffers.h>
#include <RenderDevice/Device/Context/VertexArray/BowVertexBufferAttribute.h>
#include <RenderDevice/BowClearState.h>
#include <CoreSystems/Geometry/BowMeshAttribute.h>
#include <CoreSystems/BowLogger.h>

#include <DirectX11RenderDevice/Device/GUI/imgui.h>
#include <DirectX11RenderDevice/Device/GUI/imgui_impl_win32.h>
#include <DirectX11RenderDevice/Device/GUI/imgui_impl_dx11.h>

namespace bow {

	void D3DRenderContext::GUI_NewFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void D3DRenderContext::GUI_EndFrame()
	{
		ImGui::EndFrame();
	}

	void D3DRenderContext::GUI_Render()
	{
		ImGui::Render();
	}

	void D3DRenderContext::GUI_Begin(const char* name, bool* p_open)
	{
		ImGui::Begin(name, p_open);
	}

	void D3DRenderContext::GUI_End()
	{
		ImGui::End();
	}

	void D3DRenderContext::GUI_TextUnformatted(const char* text, const char* text_end)
	{
		return ImGui::TextUnformatted(text, text_end);
	}

	void D3DRenderContext::GUI_Text(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ImGui::TextV(fmt, args);
		va_end(args);
	}

	void D3DRenderContext::GUI_TextV(const char* fmt, va_list args)
	{
		return ImGui::TextV(fmt, args);
	}

	void D3DRenderContext::GUI_TextColored(const bow::Vector4<float>& col, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ImGui::TextColoredV(ImVec4(col.x, col.y, col.z, col.w), fmt, args);
		va_end(args);
	}

	void D3DRenderContext::GUI_TextColoredV(const bow::Vector4<float>& col, const char* fmt, va_list args)
	{
		return ImGui::TextColoredV(ImVec4(col.x, col.y, col.z, col.w), fmt, args);
	}

	void D3DRenderContext::GUI_TextDisabled(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ImGui::TextDisabledV(fmt, args);
		va_end(args);
	}

	void D3DRenderContext::GUI_TextDisabledV(const char* fmt, va_list args)
	{
		return ImGui::TextDisabledV(fmt, args);
	}

	void D3DRenderContext::GUI_TextWrapped(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ImGui::TextWrappedV(fmt, args);
		va_end(args);
	}

	void D3DRenderContext::GUI_TextWrappedV(const char* fmt, va_list args)
	{
		return ImGui::TextWrappedV(fmt, args);
	}

	void D3DRenderContext::GUI_LabelText(const char* label, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ImGui::LabelTextV(label, fmt, args);
		va_end(args);
	}

	void D3DRenderContext::GUI_LabelTextV(const char* label, const char* fmt, va_list args)
	{
		return ImGui::LabelTextV(label, fmt, args);
	}

	void D3DRenderContext::GUI_BulletText(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ImGui::BulletTextV(fmt, args);
		va_end(args);
	}

	void D3DRenderContext::GUI_BulletTextV(const char* fmt, va_list args)
	{
		return ImGui::BulletTextV(fmt, args);
	}

	bool D3DRenderContext::GUI_Button(const char* label, const bow::Vector2<float>& size)
	{
		return ImGui::Button(label, ImVec2(size.x, size.y));
	}

	bool D3DRenderContext::GUI_SmallButton(const char* label)
	{
		return ImGui::Button(label);
	}

	bool D3DRenderContext::GUI_InvisibleButton(const char* str_id, const bow::Vector2<float>& size)
	{
		return ImGui::InvisibleButton(str_id, ImVec2(size.x, size.y));
	}

	bool D3DRenderContext::GUI_Checkbox(const char* label, bool* v)
	{
		return ImGui::Checkbox(label, v);
	}

	bool D3DRenderContext::GUI_CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value)
	{
		return ImGui::CheckboxFlags(label, flags, flags_value);
	}

	bool D3DRenderContext::GUI_RadioButton(const char* label, bool active)
	{
		return ImGui::RadioButton(label, active);
	}

	bool D3DRenderContext::GUI_RadioButton(const char* label, int* v, int v_button)
	{
		return ImGui::RadioButton(label, v, v_button);
	}

	void D3DRenderContext::GUI_ProgressBar(float fraction, const bow::Vector2<float>& size_arg, const char* overlay)
	{
		return ImGui::ProgressBar(fraction, ImVec2(size_arg.x, size_arg.y), overlay);
	}

	void D3DRenderContext::GUI_Bullet()
	{
		return ImGui::Bullet();
	}

	bool D3DRenderContext::GUI_SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, float power)
	{
		return ImGui::SliderFloat(label, v, v_min, v_max, format, power);
	}

	bool D3DRenderContext::GUI_SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format, float power)
	{
		return ImGui::SliderFloat2(label, v, v_min, v_max, format, power);
	}

	bool D3DRenderContext::GUI_SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format, float power)
	{
		return ImGui::SliderFloat3(label, v, v_min, v_max, format, power);
	}

	bool D3DRenderContext::GUI_SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format, float power)
	{
		return ImGui::SliderFloat4(label, v, v_min, v_max, format, power);
	}

	bool D3DRenderContext::GUI_SliderAngle(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max, const char* format)
	{
		return ImGui::SliderAngle(label, v_rad, v_degrees_min, v_degrees_max, format);
	}

	bool D3DRenderContext::GUI_SliderInt(const char* label, int* v, int v_min, int v_max, const char* format)
	{
		return ImGui::SliderInt(label, v, v_min, v_max, format);
	}

	bool D3DRenderContext::GUI_SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format)
	{
		return ImGui::SliderInt2(label, v, v_min, v_max, format);
	}

	bool D3DRenderContext::GUI_SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format)
	{
		return ImGui::SliderInt3(label, v, v_min, v_max, format);
	}

	bool D3DRenderContext::GUI_SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format)
	{
		return ImGui::SliderInt4(label, v, v_min, v_max, format);
	}

	void D3DRenderContext::GUI_Separator()
	{
		return ImGui::Separator();
	}

	void D3DRenderContext::GUI_SameLine(float offset_from_start_x, float spacing)
	{
		return ImGui::SameLine(offset_from_start_x, spacing);
	}

	void D3DRenderContext::GUI_NewLine()
	{
		return ImGui::NewLine();
	}

	void D3DRenderContext::GUI_Spacing()
	{
		return ImGui::Spacing();
	}

	void D3DRenderContext::GUI_Indent(float indent_w)
	{
		return ImGui::Indent(indent_w);
	}

	void D3DRenderContext::GUI_Unindent(float indent_w)
	{
		return ImGui::Unindent(indent_w);
	}

	void D3DRenderContext::GUI_BeginGroup()
	{
		return ImGui::BeginGroup();
	}

	void D3DRenderContext::GUI_EndGroup()
	{
		return ImGui::EndGroup();
	}

	float D3DRenderContext::GUI_GetCursorPosX()
	{
		return ImGui::GetCursorPosX();
	}

	float D3DRenderContext::GUI_GetCursorPosY()
	{
		return ImGui::GetCursorPosY();
	}

	void D3DRenderContext::GUI_SetCursorPos(const bow::Vector2<float>& local_pos)
	{
		return ImGui::SetCursorPos(ImVec2(local_pos.x, local_pos.y));
	}

	void D3DRenderContext::GUI_SetCursorPosX(float local_x)
	{
		return ImGui::SetCursorPosX(local_x);
	}

	void D3DRenderContext::GUI_SetCursorPosY(float local_y)
	{
		return ImGui::SetCursorPosY(local_y);
	}

	bow::Vector2<float> D3DRenderContext::GUI_GetCursorStartPos()
	{
		auto vec2 = ImGui::GetCursorStartPos();
		return bow::Vector2<float>(vec2.x, vec2.y);
	}

	bow::Vector2<float> D3DRenderContext::GUI_GetCursorScreenPos()
	{
		auto vec2 = ImGui::GetCursorScreenPos();
		return bow::Vector2<float>(vec2.x, vec2.y);
	}

	void D3DRenderContext::GUI_SetCursorScreenPos(const bow::Vector2<float>& pos)
	{
		return ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y));
	}

	void D3DRenderContext::GUI_AlignTextToFramePadding()
	{
		return ImGui::AlignTextToFramePadding();
	}

	float D3DRenderContext::GUI_GetTextLineHeight()
	{
		return ImGui::GetTextLineHeight();
	}

	float D3DRenderContext::GUI_GetTextLineHeightWithSpacing()
	{
		return ImGui::GetTextLineHeightWithSpacing();
	}

	float D3DRenderContext::GUI_GetFrameHeight()
	{
		return ImGui::GetFrameHeight();
	}

	float D3DRenderContext::GUI_GetFrameHeightWithSpacing()
	{
		return ImGui::GetFrameHeightWithSpacing();
	}
}

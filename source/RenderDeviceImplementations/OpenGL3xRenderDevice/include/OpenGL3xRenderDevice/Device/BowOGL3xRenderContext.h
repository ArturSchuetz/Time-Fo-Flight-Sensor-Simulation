#pragma once
#include <OpenGL3xRenderDevice/Device/Shader/BowOGL3xShaderProgram.h>
#include <OpenGL3xRenderDevice/Device/Textures/BowOGL3xTextureUnits.h>
#include <OpenGL3xRenderDevice/Device/Context/FrameBuffer/BowOGL3xFramebuffer.h>
#include <OpenGL3xRenderDevice/BowOGL3xRenderDevice.h>
#include <RenderDevice/Device/BowGraphicsWindow.h>
#include <RenderDevice/Device/IBowRenderContext.h>
#include <RenderDevice/BowRenderState.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>
#include <CoreSystems/BowCorePredeclares.h>
#include <CoreSystems/BowMath.h>

struct GLFWwindow;

namespace bow {

	typedef std::shared_ptr<class OGLShaderProgram> OGLShaderProgramPtr;
	typedef std::shared_ptr<class OGLTextureUnits> OGLTextureUnitsPtr;
	typedef std::shared_ptr<class OGLFramebuffer> OGLFramebufferPtr;

	enum class StencilFace : unsigned int;
	enum class MaterialFace : unsigned int;

	class OGLRenderContext : public IRenderContext
	{
	public:
		OGLRenderContext(GLFWwindow* window);
		~OGLRenderContext(void);

		// =========================================================================
		// INIT/RELEASE STUFF:
		// =========================================================================
		bool	Initialize(OGLRenderDevice* device);
		void	VRelease(void);

		// =========================================================================
		// RENDERING STUFF:
		// =========================================================================
		VertexArrayPtr	VCreateVertexArray(MeshAttribute mesh, ShaderVertexAttributeMap shaderAttributes, BufferHint usageHint);
		VertexArrayPtr	VCreateVertexArray(MeshBufferPtr meshBuffers);
		VertexArrayPtr	VCreateVertexArray();
		FramebufferPtr	VCreateFramebuffer();

		void	VClear(struct ClearState clearState);
		void	VDraw(PrimitiveType primitiveType, VertexArrayPtr vertexArray, ShaderProgramPtr shaderProgram, RenderState renderState);
		void	VDraw(PrimitiveType primitiveType, int offset, int count, VertexArrayPtr vertexArray, ShaderProgramPtr shaderProgram, RenderState renderState);
		void	Draw(PrimitiveType primitiveType, int offset, int count, VertexArrayPtr vertexArray, ShaderProgramPtr shaderProgram, RenderState renderState);
		void	VDrawLine(const bow::Vector3<float> &start, const bow::Vector3<float> &end);

		void	VSetTexture(int location, Texture2DPtr texture);
		void	VSetTextureSampler(int location, TextureSamplerPtr sampler);
		void	VSetFramebuffer(FramebufferPtr framebufer);
		void	VSetViewport(Viewport viewport);
		Viewport VGetViewport(void);

		void	VSwapBuffers(bool vsync);

		// =========================================================================
		// GUI STUFF:
		// =========================================================================

		// Main
		void GUI_NewFrame();                                 // start a new Dear ImGui frame, you can submit any command from this point until Render()/EndFrame().
		void GUI_EndFrame();                                 // ends the Dear ImGui frame. automatically called by Render(), you likely don't need to call that yourself directly. If you don't need to render data (skipping rendering) you may call EndFrame() but you'll have wasted CPU already! If you don't need to render, better to not create any imgui windows and not call NewFrame() at all!
		void GUI_Render();                                   // ends the Dear ImGui frame, finalize the draw data. You can get call GetDrawData() to obtain it and run your rendering function. (Obsolete: this used to call io.RenderDrawListsFn(). Nowadays, we allow and prefer calling your render function yourself.)

		// Windows
		// - Begin() = push window to the stack and start appending to it. End() = pop window from the stack.
		// - You may append multiple times to the same window during the same frame.
		// - Passing 'bool* p_open != NULL' shows a window-closing widget in the upper-right corner of the window,
		//   which clicking will set the boolean to false when clicked.
		// - Begin() return false to indicate the window is collapsed or fully clipped, so you may early out and omit submitting
		//   anything to the window. Always call a matching End() for each Begin() call, regardless of its return value!
		//   [this is due to legacy reason and is inconsistent with most other functions such as BeginMenu/EndMenu, BeginPopup/EndPopup, etc.
		//    where the EndXXX call should only be called if the corresponding BeginXXX function returned true.]
		// - Note that the bottom of window stack always contains a window called "Debug".
		void GUI_Begin(const char* name, bool* p_open = nullptr);
		void GUI_End();

		// Widgets: Text
		void GUI_TextUnformatted(const char* text, const char* text_end = NULL);                // raw text without formatting. Roughly equivalent to Text("%s", text) but: A) doesn't require null terminated string if 'text_end' is specified, B) it's faster, no memory copy is done, no buffer size limits, recommended for long chunks of text.
		void GUI_Text(const char* fmt, ...);													// simple formatted text
		void GUI_TextV(const char* fmt, va_list args);
		void GUI_TextColored(const bow::Vector4<float>& col, const char* fmt, ...);							// shortcut for PushStyleColor(ImGuiCol_Text, col); Text(fmt, ...); PopStyleColor();
		void GUI_TextColoredV(const bow::Vector4<float>& col, const char* fmt, va_list args);
		void GUI_TextDisabled(const char* fmt, ...);											// shortcut for PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]); Text(fmt, ...); PopStyleColor();
		void GUI_TextDisabledV(const char* fmt, va_list args);
		void GUI_TextWrapped(const char* fmt, ...);												// shortcut for PushTextWrapPos(0.0f); Text(fmt, ...); PopTextWrapPos();. Note that this won't work on an auto-resizing window if there's no other widgets to extend the window width, yoy may need to set a size using SetNextWindowSize().
		void GUI_TextWrappedV(const char* fmt, va_list args);
		void GUI_LabelText(const char* label, const char* fmt, ...);							// display text+label aligned the same way as value+label widgets
		void GUI_LabelTextV(const char* label, const char* fmt, va_list args);
		void GUI_BulletText(const char* fmt, ...);												// shortcut for Bullet()+Text()
		void GUI_BulletTextV(const char* fmt, va_list args);

		// Widgets: Main
		// - Most widgets return true when the value has been changed or when pressed/selected
		bool GUI_Button(const char* label, const bow::Vector2<float>& size = bow::Vector2<float>(0, 0));    // button
		bool GUI_SmallButton(const char* label);                                 // button with FramePadding=(0,0) to easily embed within text
		bool GUI_InvisibleButton(const char* str_id, const bow::Vector2<float>& size);        // button behavior without the visuals, frequently useful to build custom behaviors using the public api (along with IsItemActive, IsItemHovered, etc.)
		bool GUI_Checkbox(const char* label, bool* v);
		bool GUI_CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value);
		bool GUI_RadioButton(const char* label, bool active);                    // use with e.g. if (RadioButton("one", my_value==1)) { my_value = 1; }
		bool GUI_RadioButton(const char* label, int* v, int v_button);           // shortcut to handle the above pattern when value is an integer
		void GUI_ProgressBar(float fraction, const bow::Vector2<float>& size_arg = bow::Vector2<float>(-1, 0), const char* overlay = NULL);
		void GUI_Bullet();                                                       // draw a small circle and keep the cursor on the same line. advance cursor x position by GetTreeNodeToLabelSpacing(), same distance that TreeNode() uses

		// Widgets: Sliders
		// - CTRL+Click on any slider to turn them into an input box. Manually input values aren't clamped and can go off-bounds.
		// - Adjust format string to decorate the value with a prefix, a suffix, or adapt the editing and display precision e.g. "%.3f" -> 1.234; "%5.2f secs" -> 01.23 secs; "Biscuit: %.0f" -> Biscuit: 1; etc.
		bool GUI_SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);     // adjust format to decorate the value with a prefix or a suffix for in-slider labels or unit display. Use power!=1.0 for power curve sliders
		bool GUI_SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
		bool GUI_SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
		bool GUI_SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
		bool GUI_SliderAngle(const char* label, float* v_rad, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f, const char* format = "%.0f deg");
		bool GUI_SliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d");
		bool GUI_SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format = "%d");
		bool GUI_SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format = "%d");
		bool GUI_SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format = "%d");

		// Cursor / Layout
		// - By "cursor" we mean the current output position.
		// - The typical widget behavior is to output themselves at the current cursor position, then move the cursor one line down.
		void GUI_Separator();                                                    // separator, generally horizontal. inside a menu bar or in horizontal layout mode, this becomes a vertical separator.
		void GUI_SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f);  // call between widgets or groups to layout them horizontally. X position given in window coordinates.
		void GUI_NewLine();                                                      // undo a SameLine() or force a new line when in an horizontal-layout context.
		void GUI_Spacing();                                                      // add vertical spacing.
		void GUI_Indent(float indent_w = 0.0f);                                  // move content position toward the right, by style.IndentSpacing or indent_w if != 0
		void GUI_Unindent(float indent_w = 0.0f);                                // move content position back to the left, by style.IndentSpacing or indent_w if != 0
		void GUI_BeginGroup();                                                   // lock horizontal starting position
		void GUI_EndGroup();                                                     // unlock horizontal starting position + capture the whole group bounding box into one "item" (so you can use IsItemHovered() or layout primitives such as SameLine() on whole group, etc.)
		float GUI_GetCursorPosX();												 //   (some functions are using window-relative coordinates, such as: GetCursorPos, GetCursorStartPos, GetContentRegionMax, GetWindowContentRegion* etc.
		float GUI_GetCursorPosY();												 //    other functions such as GetCursorScreenPos or everything in ImDrawList::
		void GUI_SetCursorPos(const bow::Vector2<float>& local_pos);             //    are using the main, absolute coordinate system.
		void GUI_SetCursorPosX(float local_x);                                   //    GetWindowPos() + GetCursorPos() == GetCursorScreenPos() etc.)
		void GUI_SetCursorPosY(float local_y);                                   //
		bow::Vector2<float> GUI_GetCursorStartPos();                             // initial cursor position in window coordinates
		bow::Vector2<float> GUI_GetCursorScreenPos();                            // cursor position in absolute screen coordinates [0..io.DisplaySize] (useful to work with ImDrawList API)
		void GUI_SetCursorScreenPos(const bow::Vector2<float>& pos);             // cursor position in absolute screen coordinates [0..io.DisplaySize]
		void GUI_AlignTextToFramePadding();                                      // vertically align upcoming text baseline to FramePadding.y so that it will align properly to regularly framed items (call if you have text on a line before a framed item)
		float GUI_GetTextLineHeight();                                           // ~ FontSize
		float GUI_GetTextLineHeightWithSpacing();                                // ~ FontSize + style.ItemSpacing.y (distance in pixels between 2 consecutive lines of text)
		float GUI_GetFrameHeight();                                              // ~ FontSize + style.FramePadding.y * 2
		float GUI_GetFrameHeightWithSpacing();                                   // ~ FontSize + style.FramePadding.y * 2 + style.ItemSpacing.y (distance in pixels between 2 consecutive lines of framed widgets)

	private:
		//you shall not direct
		OGLRenderContext(OGLRenderContext&) {}
		OGLRenderContext& operator=(const OGLRenderContext&) { return *this; }

		void ApplyVertexArray(VertexArrayPtr vertexArray);
		void ApplyShaderProgram(ShaderProgramPtr shaderProgram);
		void ApplyFramebuffer();

		void ForceApplyRenderState(RenderState renderState);
		void ForceApplyRenderStateStencil(StencilFace face, StencilTestFace test);

		void ApplyRenderState(RenderState renderState);
		void ApplyPrimitiveRestart(PrimitiveRestart primitiveRestart);
		void ApplyFaceCulling(FaceCulling FaceCulling);
		void ApplyProgramPointSize(ProgramPointSize programPointSize);
		void ApplyRasterizationMode(RasterizationMode rasterizationMode);
		void ApplyScissorTest(ScissorTest scissorTest);
		void ApplyStencilTest(StencilTest stencilTest);
		void ApplyStencil(StencilFace face, StencilTestFace currentTest, StencilTestFace test);
		void ApplyDepthTest(DepthTest depthTest);
		void ApplyDepthRange(DepthRange depthRange);
		void ApplyBlending(Blending blending);
		void ApplyColorMask(ColorMask colorMask);
		void ApplyDepthMask(bool depthMask);

		Viewport			m_viewport;

		ColorRGBA			m_clearColor;
		float				m_clearDepth;
		int					m_clearStencil;

		RenderState			m_renderState;
		OGLShaderProgramPtr	m_boundShaderProgram;
		OGLTextureUnitsPtr	m_textureUnits;

		OGLFramebufferPtr	m_boundFramebuffer;
		OGLFramebufferPtr	m_setFramebuffer;

		GLFWwindow*					m_window;
		OGLRenderDevice*			m_device;
		static OGLRenderContext*	m_currentContext;
		bool						m_initialized;
		bool						m_vsync;
	};

	typedef std::shared_ptr<OGLRenderContext> OGLRenderContextPtr;

}

#pragma once
#include <RenderDevice/RenderDevice_api.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>

#include <CoreSystems/BowCorePredeclares.h>
#include <CoreSystems/BowMath.h>

namespace bow {

	enum class WindingOrder : char
	{
		Clockwise,
		Counterclockwise
	};

	enum class PrimitiveType : char
	{
		Points,
		Lines,
		LineLoop,
		LineStrip,
		Triangles,
		TriangleStrip,
		TriangleFan,
		LinesAdjacency,
		LineStripAdjacency,
		TrianglesAdjacency,
		TriangleStripAdjacency
	};

	struct Viewport
	{
		Viewport() : x(0), y(0), width(0), height(0) {}
		Viewport(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) {}
		int x;
		int y;
		int width;
		int height;

		bool operator == (const Viewport& vp)
		{
			return (x == vp.x && x == vp.y && x == vp.width && x == vp.height);
		}

		bool operator != (const Viewport& vp)
		{
			return !(x == vp.x && x == vp.y && x == vp.width && x == vp.height);
		}
	};


	class IRenderContext
	{
	public:
		virtual	~IRenderContext(void) {}

		// =========================================================================
		// INIT/RELEASE STUFF:
		// =========================================================================
		virtual void VRelease(void) = 0;

		// =========================================================================
		// RENDERING STUFF:
		// =========================================================================
		virtual VertexArrayPtr VCreateVertexArray(MeshAttribute mesh, ShaderVertexAttributeMap shaderAttributes, BufferHint usageHint) = 0;
		virtual VertexArrayPtr VCreateVertexArray(MeshBufferPtr meshBuffers) = 0;
		virtual VertexArrayPtr VCreateVertexArray() = 0;
		virtual FramebufferPtr VCreateFramebuffer() = 0;

		virtual void VClear(ClearState clearState) = 0;
		virtual void VDraw(PrimitiveType primitiveType, VertexArrayPtr vertexArray, ShaderProgramPtr shaderProgram, RenderState renderState) = 0;
		virtual void VDraw(PrimitiveType primitiveType, int offset, int count, VertexArrayPtr vertexArray, ShaderProgramPtr shaderProgram, RenderState renderState) = 0;
		virtual void VDrawLine(const bow::Vector3<float> &start, const bow::Vector3<float> &end) = 0;

		virtual void VSetTexture(int location, Texture2DPtr texture) = 0;
		virtual void VSetTextureSampler(int location, TextureSamplerPtr sampler) = 0;

		virtual void VSetFramebuffer(FramebufferPtr framebufer) = 0;
		virtual void VSetViewport(Viewport viewport) = 0;
		virtual Viewport VGetViewport(void) = 0;

		virtual void VSwapBuffers(bool vsync = false) = 0;

		// =========================================================================
		// GUI STUFF:
		// =========================================================================

		// Main
		virtual void GUI_NewFrame() = 0;                                 // start a new Dear ImGui frame, you can submit any command from this point until Render()/EndFrame().
		virtual void GUI_EndFrame() = 0;                                 // ends the Dear ImGui frame. automatically called by Render(), you likely don't need to call that yourself directly. If you don't need to render data (skipping rendering) you may call EndFrame() but you'll have wasted CPU already! If you don't need to render, better to not create any imgui windows and not call NewFrame() at all!
		virtual void GUI_Render() = 0;                                   // ends the Dear ImGui frame, finalize the draw data. You can get call GetDrawData() to obtain it and run your rendering function. (Obsolete: this used to call io.RenderDrawListsFn(). Nowadays, we allow and prefer calling your render function yourself.)

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
		virtual void GUI_Begin(const char* name, bool* p_open = nullptr) = 0;
		virtual void GUI_End() = 0;

		// Widgets: Text
		virtual void GUI_TextUnformatted(const char* text, const char* text_end = NULL) = 0;                // raw text without formatting. Roughly equivalent to Text("%s", text) but: A) doesn't require null terminated string if 'text_end' is specified, B) it's faster, no memory copy is done, no buffer size limits, recommended for long chunks of text.
		virtual void GUI_Text(const char* fmt, ...) = 0;													// simple formatted text
		virtual void GUI_TextV(const char* fmt, va_list args) = 0;
		virtual void GUI_TextColored(const bow::Vector4<float>& col, const char* fmt, ...) = 0;				// shortcut for PushStyleColor(ImGuiCol_Text, col); Text(fmt, ...); PopStyleColor();
		virtual void GUI_TextColoredV(const bow::Vector4<float>& col, const char* fmt, va_list args) = 0;
		virtual void GUI_TextDisabled(const char* fmt, ...) = 0;											// shortcut for PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]); Text(fmt, ...); PopStyleColor();
		virtual void GUI_TextDisabledV(const char* fmt, va_list args) = 0;
		virtual void GUI_TextWrapped(const char* fmt, ...) = 0;												// shortcut for PushTextWrapPos(0.0f); Text(fmt, ...); PopTextWrapPos();. Note that this won't work on an auto-resizing window if there's no other widgets to extend the window width, yoy may need to set a size using SetNextWindowSize().
		virtual void GUI_TextWrappedV(const char* fmt, va_list args) = 0;
		virtual void GUI_LabelText(const char* label, const char* fmt, ...) = 0;							// display text+label aligned the same way as value+label widgets
		virtual void GUI_LabelTextV(const char* label, const char* fmt, va_list args) = 0;
		virtual void GUI_BulletText(const char* fmt, ...) = 0;												// shortcut for Bullet()+Text()
		virtual void GUI_BulletTextV(const char* fmt, va_list args) = 0;

		// Widgets: Main
		// - Most widgets return true when the value has been changed or when pressed/selected
		virtual bool GUI_Button(const char* label, const bow::Vector2<float>& size = bow::Vector2<float>(0, 0)) = 0;    // button
		virtual bool GUI_SmallButton(const char* label) = 0;                                 // button with FramePadding=(0,0) to easily embed within text
		virtual bool GUI_InvisibleButton(const char* str_id, const bow::Vector2<float>& size) = 0;        // button behavior without the visuals, frequently useful to build custom behaviors using the public api (along with IsItemActive, IsItemHovered, etc.)
		virtual bool GUI_Checkbox(const char* label, bool* v) = 0;
		virtual bool GUI_CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value) = 0;
		virtual bool GUI_RadioButton(const char* label, bool active) = 0;                    // use with e.g. if (RadioButton("one", my_value==1)) { my_value = 1; }
		virtual bool GUI_RadioButton(const char* label, int* v, int v_button) = 0;           // shortcut to handle the above pattern when value is an integer
		virtual void GUI_ProgressBar(float fraction, const bow::Vector2<float>& size_arg = bow::Vector2<float>(-1, 0), const char* overlay = NULL) = 0;
		virtual void GUI_Bullet() = 0;                                                       // draw a small circle and keep the cursor on the same line. advance cursor x position by GetTreeNodeToLabelSpacing(), same distance that TreeNode() uses

		// Widgets: Sliders
		// - CTRL+Click on any slider to turn them into an input box. Manually input values aren't clamped and can go off-bounds.
		// - Adjust format string to decorate the value with a prefix, a suffix, or adapt the editing and display precision e.g. "%.3f" -> 1.234; "%5.2f secs" -> 01.23 secs; "Biscuit: %.0f" -> Biscuit: 1; etc.
		virtual bool GUI_SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", float power = 1.0f) = 0;     // adjust format to decorate the value with a prefix or a suffix for in-slider labels or unit display. Use power!=1.0 for power curve sliders
		virtual bool GUI_SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f) = 0;
		virtual bool GUI_SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f) = 0;
		virtual bool GUI_SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f) = 0;
		virtual bool GUI_SliderAngle(const char* label, float* v_rad, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f, const char* format = "%.0f deg") = 0;
		virtual bool GUI_SliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d") = 0;
		virtual bool GUI_SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format = "%d") = 0;
		virtual bool GUI_SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format = "%d") = 0;
		virtual bool GUI_SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format = "%d") = 0;

		// Cursor / Layout
		// - By "cursor" we mean the current output position.
		// - The typical widget behavior is to output themselves at the current cursor position, then move the cursor one line down.
		virtual void GUI_Separator() = 0;                                                    // separator, generally horizontal. inside a menu bar or in horizontal layout mode, this becomes a vertical separator.
		virtual void GUI_SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f) = 0;  // call between widgets or groups to layout them horizontally. X position given in window coordinates.
		virtual void GUI_NewLine() = 0;                                                      // undo a SameLine() or force a new line when in an horizontal-layout context.
		virtual void GUI_Spacing() = 0;                                                      // add vertical spacing.
		virtual void GUI_Indent(float indent_w = 0.0f) = 0;                                  // move content position toward the right, by style.IndentSpacing or indent_w if != 0
		virtual void GUI_Unindent(float indent_w = 0.0f) = 0;                                // move content position back to the left, by style.IndentSpacing or indent_w if != 0
		virtual void GUI_BeginGroup() = 0;                                                   // lock horizontal starting position
		virtual void GUI_EndGroup() = 0;                                                     // unlock horizontal starting position + capture the whole group bounding box into one "item" (so you can use IsItemHovered() or layout primitives such as SameLine() on whole group, etc.)
		virtual float GUI_GetCursorPosX() = 0;													 //   (some functions are using window-relative coordinates, such as: GetCursorPos, GetCursorStartPos, GetContentRegionMax, GetWindowContentRegion* etc.
		virtual float GUI_GetCursorPosY() = 0;													 //    other functions such as GetCursorScreenPos or everything in ImDrawList::
		virtual void GUI_SetCursorPos(const bow::Vector2<float>& local_pos) = 0;                 //    are using the main, absolute coordinate system.
		virtual void GUI_SetCursorPosX(float local_x) = 0;                                       //    GetWindowPos() + GetCursorPos() == GetCursorScreenPos() etc.)
		virtual void GUI_SetCursorPosY(float local_y) = 0;                                       //
		virtual bow::Vector2<float> GUI_GetCursorStartPos() = 0;                                 // initial cursor position in window coordinates
		virtual bow::Vector2<float> GUI_GetCursorScreenPos() = 0;                                // cursor position in absolute screen coordinates [0..io.DisplaySize] (useful to work with ImDrawList API)
		virtual void GUI_SetCursorScreenPos(const bow::Vector2<float>& pos) = 0;                 // cursor position in absolute screen coordinates [0..io.DisplaySize]
		virtual void GUI_AlignTextToFramePadding() = 0;                                      // vertically align upcoming text baseline to FramePadding.y so that it will align properly to regularly framed items (call if you have text on a line before a framed item)
		virtual float GUI_GetTextLineHeight() = 0;                                           // ~ FontSize
		virtual float GUI_GetTextLineHeightWithSpacing() = 0;                                // ~ FontSize + style.ItemSpacing.y (distance in pixels between 2 consecutive lines of text)
		virtual float GUI_GetFrameHeight() = 0;                                              // ~ FontSize + style.FramePadding.y * 2
		virtual float GUI_GetFrameHeightWithSpacing() = 0;
	};

}

#pragma once
#include <DirectX11RenderDevice/DirectX11RenderDevice_api.h>
#include <DirectX11RenderDevice/BowD3D11RenderDevicePredeclares.h>

#include <RenderDevice/IBowRenderDevice.h>

#include <d3d11_4.h>
#include <dxgi1_5.h>

namespace bow {

	typedef std::shared_ptr<class D3DGraphicsWindow> D3DGraphicsWindowPtr;
	typedef std::unordered_map<unsigned int, D3DGraphicsWindowPtr> D3DGraphicsWindowMap;

	class D3DRenderDevice : public IRenderDevice
	{
	public:
		D3DRenderDevice(void);
		~D3DRenderDevice(void);

		//// =========================================================================
		//// INIT/RELEASE STUFF:
		//// =========================================================================
		bool Initialize(void);
		void VRelease(void);

		GraphicsWindowPtr VCreateWindow(int width, int height, const std::string& title, WindowType type);

		//// =========================================================================
		//// SHADER STUFF:
		//// =========================================================================
		ShaderProgramPtr		VCreateShaderProgramFromFile(const std::string& VertexShaderFilename, const std::string& FragementShaderFilename);
		ShaderProgramPtr		VCreateShaderProgramFromFile(const std::string& VertexShaderFilename, const std::string& GeometryShaderFilename, const std::string& FragementShaderFilename);

		ShaderProgramPtr		VCreateShaderProgram(const std::string& VertexShaderSource, const std::string& FragementShaderSource);
		ShaderProgramPtr		VCreateShaderProgram(const std::string& VertexShaderSource, const std::string& GeometryShaderSource, const std::string& FragementShaderSource);

		MeshBufferPtr			VCreateMeshBuffers(MeshAttribute mesh, ShaderVertexAttributeMap shaderAttributes, BufferHint usageHint);
		VertexBufferPtr			VCreateVertexBuffer(BufferHint usageHint, int sizeInBytes);
		IndexBufferPtr			VCreateIndexBuffer(BufferHint usageHint, IndexBufferDatatype dataType, int sizeInBytes);

		WritePixelBufferPtr		VCreateWritePixelBuffer(PixelBufferHint usageHint, int sizeInBytes);

		Texture2DPtr			VCreateTexture2D(Texture2DDescription description);
		Texture2DPtr			VCreateTexture2D(ImagePtr image);

		TextureSamplerPtr		VCreateTexture2DSampler(TextureMinificationFilter minificationFilter, TextureMagnificationFilter magnificationFilter, TextureWrap wrapS, TextureWrap wrapT, float maximumAnistropy);

	private:
		//you shall not copy
		D3DRenderDevice(D3DRenderDevice&) {}
		D3DRenderDevice& operator=(const D3DRenderDevice&) { return *this; }

		ComPtr<ID3D11Device2> m_d3d11device;

		HINSTANCE m_hInstance;
		bool m_useWarpDevice;
		bool m_initialized;
	};

	typedef std::shared_ptr<D3DRenderDevice> D3DRenderDevicePtr;
	typedef std::unordered_map<unsigned int, D3DRenderDevice> D3DRenderDeviceMap;

}

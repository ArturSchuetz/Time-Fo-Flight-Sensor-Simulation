#include <DirectX11RenderDevice/BowD3D11RenderDevice.h>
#include <DirectX11RenderDevice/Device/BowD3D11GraphicsWindow.h>
#include <DirectX11RenderDevice/Device/Shader/BowD3D11ShaderProgram.h>
#include <DirectX11RenderDevice/Device/Buffer/BowD3D11IndexBuffer.h>
#include <DirectX11RenderDevice/Device/Buffer/BowD3D11VertexBuffer.h>
#include <DirectX11RenderDevice/Device/Buffer/BowD3D11WritePixelBuffer.h>
#include <DirectX11RenderDevice/Device/Textures/BowD3D11Texture2D.h>
#include <DirectX11RenderDevice/Device/Textures/BowD3D11TextureSampler.h>
#include <DirectX11RenderDevice/BowD3D11TypeConverter.h>
#include <RenderDevice/BowRenderDevicePredeclares.h>
#include <RenderDevice/Device/Shader/BowShaderVertexAttribute.h>
#include <RenderDevice/Device/Context/VertexArray/BowVertexBufferAttribute.h>
#include <RenderDevice/Device/Context/Mesh/BowMeshBuffers.h>
#include <CoreSystems/BowCorePredeclares.h>
#include <CoreSystems/Geometry/VertexAttributes/IBowVertexAttribute.h>
#include <CoreSystems/Geometry/Indices/IBowIndicesBase.h>
#include <CoreSystems/Geometry/Indices/BowIndicesUnsignedInt.h>
#include <CoreSystems/Geometry/Indices/BowIndicesUnsignedShort.h>
#include <CoreSystems/Geometry/BowMeshAttribute.h>
#include <CoreSystems/BowLogger.h>
#include <Resources/BowResourcesPredeclares.h>
#include <Resources/Resources/BowMesh.h>
#include <Resources/Resources/BowImage.h>

// DirectX 12 specific headers.
#include <d3d11.h>
#include <dxgi1_6.h>

// D3D11 extension library.
#include "d3dx11.h"

namespace bow {

	extern std::string widestring2string(const std::wstring& wstr);
	extern std::wstring string2widestring(const std::string& s);
	extern std::string toErrorString(HRESULT hresult);

	D3DRenderDevice::D3DRenderDevice(void) :
		m_D3D11device(nullptr),
		m_useWarpDevice(false),
		m_initialized(false)
	{
		m_hInstance = GetModuleHandle(NULL);
	}

	D3DRenderDevice::~D3DRenderDevice(void)
	{
		VRelease();
	}

	bool D3DRenderDevice::Initialize(void)
	{
		HRESULT hresult;

		UINT dxgiFactoryFlags = 0;
		ComPtr<IDXGIFactory4> factory;
#if defined(_DEBUG)
		ComPtr<ID3D11Debug> debugController = nullptr;

		// Enable the debug layer (requires the Graphics Tools "optional feature").
		// NOTE: Enabling the debug layer after device creation will invalidate the active device.
		{
			hresult = D3D11GetDebugInterface(IID_PPV_ARGS(&debugController));
			if (SUCCEEDED(hresult))
			{
				debugController->EnableDebugLayer();

				// Enable additional debug layers.
				dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			}
			else
			{
				std::string errorMsg = toErrorString(hresult);
				LOG_ERROR(std::string(std::string("D3D11GetDebugInterface: ") + errorMsg).c_str());
			}
		}
#endif

		ComPtr<IDXGIAdapter1> dxgiAdapter1 = nullptr;
		ComPtr<IDXGIAdapter4> dxgiAdapter4 = nullptr;

		hresult = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
		if (FAILED(hresult))
		{
			std::string errorMsg = toErrorString(hresult);
			LOG_ERROR(std::string(std::string("CreateDXGIFactory2: ") + errorMsg).c_str());
			return false;
		}

		if (m_useWarpDevice)
		{
			hresult = factory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1));
			if (FAILED(hresult))
			{
				std::string errorMsg = toErrorString(hresult);
				LOG_ERROR(std::string(std::string("factory->EnumWarpAdapter: ") + errorMsg).c_str());
				return false;
			}

			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			hresult = dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);
			if (FAILED(hresult))
			{
				std::string errorMsg = toErrorString(hresult);
				LOG_ERROR(std::string(std::string("dxgiAdapter1->GetDesc1: ") + errorMsg).c_str());
			}

			LOG_INFO(std::string("Using WARP Device: " + widestring2string(dxgiAdapterDesc1.Description)).c_str());
			LOG_INFO(std::string("\t\tDedicated Video Memory:  " + std::to_string(dxgiAdapterDesc1.DedicatedVideoMemory / 1024 / 1024) + " MB").c_str());
			LOG_INFO(std::string("\t\tDedicated System Memory: " + std::to_string(dxgiAdapterDesc1.DedicatedSystemMemory / 1024 / 1024) + " MB").c_str());
			LOG_INFO(std::string("\t\tShared System Memory:    " + std::to_string(dxgiAdapterDesc1.SharedSystemMemory / 1024 / 1024) + " MB").c_str());

			hresult = dxgiAdapter1.As(&dxgiAdapter4);
			if (FAILED(hresult))
			{
				std::string errorMsg = toErrorString(hresult);
				LOG_ERROR(std::string(std::string("dxgiAdapter1.As: ") + errorMsg).c_str());
				return false;
			}
		}
		else
		{
			SIZE_T maxDedicatedVideoMemory = 0;
			ComPtr<IDXGIAdapter1> choosedDevice = nullptr;
			LOG_INFO("Graphics Devices found:");
			for (UINT i = 0; factory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
			{
				DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
				dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);
				if (FAILED(hresult))
				{
					std::string errorMsg = toErrorString(hresult);
					LOG_ERROR(std::string(std::string("dxgiAdapter1->GetDesc1: ") + errorMsg).c_str());
				}

				LOG_INFO(std::string("\t" + widestring2string(dxgiAdapterDesc1.Description) + ":").c_str());
				LOG_INFO(std::string("\t\tDedicated Video Memory:  " + std::to_string(dxgiAdapterDesc1.DedicatedVideoMemory / 1024 / 1024) + " MB").c_str());
				LOG_INFO(std::string("\t\tDedicated System Memory: " + std::to_string(dxgiAdapterDesc1.DedicatedSystemMemory / 1024 / 1024) + " MB").c_str());
				LOG_INFO(std::string("\t\tShared System Memory:    " + std::to_string(dxgiAdapterDesc1.SharedSystemMemory / 1024 / 1024) + " MB").c_str());

				// Check to see if the adapter can create a D3D11 device without actually 
				// creating it. The adapter with the largest dedicated video memory
				// is favored.

				hresult = D3D11CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_1, __uuidof(ID3D11Device), nullptr);
				if (SUCCEEDED(hresult))
				{
					if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 && dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
					{
						maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
						choosedDevice = dxgiAdapter1;

						hresult = dxgiAdapter1.As(&dxgiAdapter4);
						if (FAILED(hresult))
						{
							std::string errorMsg = toErrorString(hresult);
							LOG_ERROR(std::string(std::string("dxgiAdapter1.As: ") + errorMsg).c_str());
							return false;
						}
					}
				}
			}

			if (dxgiAdapter4 == nullptr)
			{
				// If no hardware device was found, warp should be used
				m_useWarpDevice = true;
				// retry with warp devices
				return Initialize();
			}
			else
			{
				DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
				choosedDevice->GetDesc1(&dxgiAdapterDesc1);
				LOG_INFO(std::string("Using HARDWARE Device: " + widestring2string(dxgiAdapterDesc1.Description)).c_str());
			}
		}

		hresult = D3D11CreateDevice(dxgiAdapter4.Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&m_D3D11device));
		if (FAILED(hresult))
		{
			std::string errorMsg = toErrorString(hresult);
			LOG_ERROR(std::string(std::string("D3D11CreateDevice: ") + errorMsg).c_str());
			return false;
		}

		// Enable debug messages in debug mode.
#if defined(_DEBUG)
		ComPtr<ID3D11InfoQueue> infoQueue = nullptr;
		if (SUCCEEDED(m_D3D11device.As(&infoQueue)))
		{
			infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
			infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);

			// Suppress whole categories of messages
			//D3D11_MESSAGE_CATEGORY Categories[] = {};

			// Suppress messages based on their severity level
			D3D11_MESSAGE_SEVERITY Severities[] =
			{
				D3D11_MESSAGE_SEVERITY_INFO
			};

			// Suppress individual messages by their ID
			D3D11_MESSAGE_ID DenyIds[] = {
				D3D11_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
				D3D11_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
				D3D11_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
			};

			D3D11_INFO_QUEUE_FILTER NewFilter = {};
			//NewFilter.DenyList.NumCategories = _countof(Categories);
			//NewFilter.DenyList.pCategoryList = Categories;
			NewFilter.DenyList.NumSeverities = _countof(Severities);
			NewFilter.DenyList.pSeverityList = Severities;
			NewFilter.DenyList.NumIDs = _countof(DenyIds);
			NewFilter.DenyList.pIDList = DenyIds;

			hresult = infoQueue->PushStorageFilter(&NewFilter);
			if (FAILED(hresult))
			{
				std::string errorMsg = toErrorString(hresult);
				LOG_ERROR(std::string(std::string("infoQueue->PushStorageFilter: ") + errorMsg).c_str());
				return false;
			}
		}
#endif
		return true;
	}

	void D3DRenderDevice::VRelease(void)
	{

	}

	GraphicsWindowPtr D3DRenderDevice::VCreateWindow(int width, int height, const std::string& title, WindowType type)
	{
		D3DGraphicsWindowPtr pGraphicsWindow = D3DGraphicsWindowPtr(new D3DGraphicsWindow());
		if (pGraphicsWindow->Initialize(width, height, title, type, this))
		{
			return pGraphicsWindow;
		}
		else
		{
			LOG_ERROR("Error while creating OpenGL-Window!");
			return GraphicsWindowPtr(nullptr);
		}
	}

	// =========================================================================
	// SHADER STUFF:
	// =========================================================================
	ShaderProgramPtr D3DRenderDevice::VCreateShaderProgramFromFile(const std::string& VertexShaderFilename, const std::string& FragementShaderFilename)
	{
		//Open file
		std::string vshaderString;
		std::ifstream vsourceFile(VertexShaderFilename.c_str());

		//Source file loaded
		if (vsourceFile)
		{
			//Get shader source
			vshaderString.assign((std::istreambuf_iterator< char >(vsourceFile)), std::istreambuf_iterator< char >());

			//Open file
			std::string fshaderString;
			std::ifstream fsourceFile(FragementShaderFilename.c_str());

			if (fsourceFile)
			{
				//Get shader source
				fshaderString.assign((std::istreambuf_iterator< char >(fsourceFile)), std::istreambuf_iterator< char >());
				return VCreateShaderProgram(vshaderString, fshaderString);
			}
			else
			{
				LOG_ERROR("Could not open Shader from File");
				return D3DShaderProgramPtr(nullptr);
			}
		}
		else
		{
			LOG_ERROR("Could not open Shader from File");
			return D3DShaderProgramPtr(nullptr);
		}
	}

	ShaderProgramPtr D3DRenderDevice::VCreateShaderProgramFromFile(const std::string& VertexShaderFilename, const std::string& GeometryShaderFilename, const std::string& FragementShaderFilename)
	{
		//Open file
		std::string vshaderString;
		std::ifstream vsourceFile(VertexShaderFilename.c_str());

		//Source file loaded
		if (vsourceFile)
		{
			//Get shader source
			vshaderString.assign((std::istreambuf_iterator< char >(vsourceFile)), std::istreambuf_iterator< char >());

			//Open file
			std::string gshaderString;
			std::ifstream gsourceFile(GeometryShaderFilename.c_str());

			if (gsourceFile)
			{
				//Get shader source
				gshaderString.assign((std::istreambuf_iterator< char >(gsourceFile)), std::istreambuf_iterator< char >());

				//Open file
				std::string fshaderString;
				std::ifstream fsourceFile(FragementShaderFilename.c_str());

				if (fsourceFile)
				{
					//Get shader source
					fshaderString.assign((std::istreambuf_iterator< char >(fsourceFile)), std::istreambuf_iterator< char >());
					return VCreateShaderProgram(vshaderString, gshaderString, fshaderString);
				}
				else
				{
					LOG_ERROR("Could not open Shader from File");
					return D3DShaderProgramPtr(nullptr);
				}
			}
			else
			{
				LOG_ERROR("Could not open Shader from File");
				return D3DShaderProgramPtr(nullptr);
			}
		}
		else
		{
			LOG_ERROR("Could not open Shader from File");
			return D3DShaderProgramPtr(nullptr);
		}
	}

	ShaderProgramPtr D3DRenderDevice::VCreateShaderProgram(const std::string& VertexShaderSource, const std::string& FragementShaderSource)
	{
		return VCreateShaderProgram(VertexShaderSource, std::string(), FragementShaderSource);
	}

	ShaderProgramPtr D3DRenderDevice::VCreateShaderProgram(const std::string& VertexShaderSource, const std::string& GeometryShaderSource, const std::string& FragementShaderSource)
	{
		return D3DShaderProgramPtr(new D3DShaderProgram(VertexShaderSource, GeometryShaderSource, FragementShaderSource));
	}

	MeshBufferPtr D3DRenderDevice::VCreateMeshBuffers(MeshAttribute mesh, ShaderVertexAttributeMap shaderAttributes, BufferHint usageHint)
	{
		MeshBuffers *meshBuffers = new MeshBuffers();

		if (mesh.Indices != nullptr)
		{
			if (mesh.Indices->Type == IndicesType::UnsignedShort)
			{
				std::vector<unsigned short> meshIndices = (std::dynamic_pointer_cast<IndicesUnsignedShort>(mesh.Indices))->Values;

				std::vector<unsigned short> indices = std::vector<unsigned short>(meshIndices.size());
				for (unsigned int j = 0; j < meshIndices.size(); ++j)
				{
					indices[j] = meshIndices[j];
				}

				IndexBufferPtr indexBuffer = VCreateIndexBuffer(usageHint, IndexBufferDatatype::UnsignedShort, indices.size() * sizeof(unsigned short));
				indexBuffer->VCopyFromSystemMemory(&(indices[0]), indices.size() * sizeof(unsigned short));
				meshBuffers->IndexBuffer = indexBuffer;
			}
			else if (mesh.Indices->Type == IndicesType::UnsignedInt)
			{
				std::vector<unsigned int> meshIndices = (std::dynamic_pointer_cast<IndicesUnsignedInt>(mesh.Indices))->Values;

				std::vector<unsigned int> indices = std::vector<unsigned int>(meshIndices.size());
				for (unsigned int j = 0; j < meshIndices.size(); ++j)
				{
					indices[j] = meshIndices[j];
				}

				IndexBufferPtr indexBuffer = VCreateIndexBuffer(usageHint, IndexBufferDatatype::UnsignedInt, indices.size() * sizeof(unsigned int));
				indexBuffer->VCopyFromSystemMemory(&(indices[0]), indices.size() * sizeof(unsigned int));
				meshBuffers->IndexBuffer = indexBuffer;
			}
			else
			{
				LOG_ASSERT(false, "mesh.Indices.Datatype is not supported.");
			}
		}

		for (auto shaderAttribute = shaderAttributes.begin(); shaderAttribute != shaderAttributes.end(); ++shaderAttribute)
		{
			VertexAttributePtr attribute = mesh.GetAttribute(shaderAttribute->first);
			if (attribute.get() == nullptr)
			{
				LOG_ERROR("Shader requires vertex attribute \"%s\", which is not present in mesh.", shaderAttribute->first);
			}

			if (attribute->Type == VertexAttributeType::UnsignedByte)
			{
				unsigned int count = (std::dynamic_pointer_cast<VertexAttribute<unsigned char>>(attribute))->Values.size();

				VertexBufferPtr vertexBuffer = VCreateVertexBuffer(usageHint, sizeof(unsigned char) * count);
				vertexBuffer->VCopyFromSystemMemory(&((std::dynamic_pointer_cast<VertexAttribute<unsigned char>>(attribute))->Values[0]), 0, sizeof(unsigned char) * count);

				meshBuffers->SetAttribute(shaderAttribute->second->Location, VertexBufferAttributePtr(new VertexBufferAttribute(vertexBuffer, ComponentDatatype::UnsignedByte, 1)));
			}
			else if (attribute->Type == VertexAttributeType::Float)
			{
				unsigned int count = (std::dynamic_pointer_cast<VertexAttribute<float>>(attribute))->Values.size();

				VertexBufferPtr vertexBuffer = VCreateVertexBuffer(usageHint, sizeof(float) * count);
				vertexBuffer->VCopyFromSystemMemory(&((std::dynamic_pointer_cast<VertexAttribute<float>>(attribute))->Values[0]), 0, sizeof(float) * count);

				meshBuffers->SetAttribute(shaderAttribute->second->Location, VertexBufferAttributePtr(new VertexBufferAttribute(vertexBuffer, ComponentDatatype::Float, 1)));
			}
			else if (attribute->Type == VertexAttributeType::FloatVector2)
			{
				unsigned int count = (std::dynamic_pointer_cast<VertexAttribute<Vector2<float>>>(attribute))->Values.size();

				VertexBufferPtr vertexBuffer = VCreateVertexBuffer(usageHint, sizeof(Vector2<float>) * count);
				vertexBuffer->VCopyFromSystemMemory(&((std::dynamic_pointer_cast<VertexAttribute<Vector2<float>>>(attribute))->Values[0]), 0, sizeof(Vector2<float>) * count);

				meshBuffers->SetAttribute(shaderAttribute->second->Location, VertexBufferAttributePtr(new VertexBufferAttribute(vertexBuffer, ComponentDatatype::Float, 2)));
			}
			else if (attribute->Type == VertexAttributeType::FloatVector3)
			{
				unsigned int count = (std::dynamic_pointer_cast<VertexAttribute<Vector3<float>>>(attribute))->Values.size();

				VertexBufferPtr vertexBuffer = VCreateVertexBuffer(usageHint, sizeof(Vector3<float>) * count);
				vertexBuffer->VCopyFromSystemMemory(&((std::dynamic_pointer_cast<VertexAttribute<Vector3<float>>>(attribute))->Values[0]), 0, sizeof(Vector3<float>) * count);

				meshBuffers->SetAttribute(shaderAttribute->second->Location, VertexBufferAttributePtr(new VertexBufferAttribute(vertexBuffer, ComponentDatatype::Float, 3)));
			}
			else if (attribute->Type == VertexAttributeType::FloatVector4)
			{
				unsigned int count = (std::dynamic_pointer_cast<VertexAttribute<Vector4<float>>>(attribute))->Values.size();

				VertexBufferPtr vertexBuffer = VCreateVertexBuffer(usageHint, sizeof(Vector4<float>) * count);
				vertexBuffer->VCopyFromSystemMemory(&((std::dynamic_pointer_cast<VertexAttribute<Vector4<float>>>(attribute))->Values[0]), 0, sizeof(Vector4<float>) * count);

				meshBuffers->SetAttribute(shaderAttribute->second->Location, VertexBufferAttributePtr(new VertexBufferAttribute(vertexBuffer, ComponentDatatype::Float, 4)));
			}
			else
			{
				LOG_ERROR("attribute.Datatype not implemented!");
			}
		}

		return MeshBufferPtr(meshBuffers);
	}

	VertexBufferPtr	D3DRenderDevice::VCreateVertexBuffer(BufferHint usageHint, int sizeInBytes)
	{
		return D3DVertexBufferPtr(new D3DVertexBuffer(usageHint, sizeInBytes));
	}

	IndexBufferPtr D3DRenderDevice::VCreateIndexBuffer(BufferHint usageHint, IndexBufferDatatype dataType, int sizeInBytes)
	{
		return D3DIndexBufferPtr(new D3DIndexBuffer(usageHint, dataType, sizeInBytes));
	}

	WritePixelBufferPtr D3DRenderDevice::VCreateWritePixelBuffer(PixelBufferHint usageHint, int sizeInBytes)
	{
		return D3DWritePixelBufferPtr(new D3DWritePixelBuffer(usageHint, sizeInBytes));
	}

	Texture2DPtr D3DRenderDevice::VCreateTexture2D(Texture2DDescription description)
	{
		return D3DTexture2DPtr(new D3DTexture2D(description, GL_TEXTURE_2D));
	}

	Texture2DPtr D3DRenderDevice::VCreateTexture2D(ImagePtr image)
	{
		if (image->VGetSizeInBytes() == 0)
			return Texture2DPtr(nullptr);

		TextureFormat format;
		if (image->VGetSizeInBytes() / (image->GetHeight() * image->GetWidth()) == 3)
			format = TextureFormat::RedGreenBlue8;
		else
			format = TextureFormat::RedGreenBlueAlpha8;

		Texture2DPtr texture = D3DTexture2DPtr(new D3DTexture2D(Texture2DDescription(image->GetWidth(), image->GetHeight(), format, true), GL_TEXTURE_2D));
		texture->VCopyFromSystemMemory(image->GetData(), D3DTypeConverter::TextureToImageFormat(format), ImageDatatype::UnsignedByte);
		return texture;
	}

	TextureSamplerPtr D3DRenderDevice::VCreateTexture2DSampler(TextureMinificationFilter minificationFilter, TextureMagnificationFilter magnificationFilter, TextureWrap wrapS, TextureWrap wrapT, float maximumAnistropy)
	{
		return D3DTextureSamplerPtr(new D3DTextureSampler(minificationFilter, magnificationFilter, wrapS, wrapT, maximumAnistropy));
	}

}

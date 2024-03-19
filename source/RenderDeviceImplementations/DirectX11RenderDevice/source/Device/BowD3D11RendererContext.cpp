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

	D3DRenderContext* D3DRenderContext::m_currentContext;

	D3DRenderContext::D3DRenderContext(HWND hWnd, D3DRenderDevice* parent) :
		m_swapChain(nullptr),
		m_backBuffers(),
		m_currentBackBufferIndex(0),
		m_hWnd(hWnd),
		m_parentDevice(parent),
		m_initialized(false),
		m_VSync(true),
		m_TearingSupported(false)
	{

	}


	bool D3DRenderContext::Initialize(unsigned int width, unsigned int height)
	{
		m_width = width;
		m_height = height;

		HRESULT hresult;

		m_TearingSupported = D3DRenderDevice::CheckTearingSupport();

		m_directCommandQueue = D3DRenderDevice::CreateCommandQueue(m_parentDevice->m_d3d12device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		if (m_directCommandQueue == nullptr)
			return false;

		m_swapChain = D3DRenderDevice::CreateSwapChain(m_hWnd, m_directCommandQueue, width, height, m_numBuffers);
		if (m_swapChain == nullptr)
			return false;

		m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

		m_renderTargetViewDescriptorHeap = D3DRenderDevice::CreateDescriptorHeap(m_parentDevice->m_d3d12device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_numBuffers);
		if (m_renderTargetViewDescriptorHeap == nullptr)
			return false;

		m_depthStencilViewDescriptorHeap = D3DRenderDevice::CreateDescriptorHeap(m_parentDevice->m_d3d12device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, m_numBuffers);
		if (m_depthStencilViewDescriptorHeap == nullptr)
			return false;

		m_renderTargetViewDescriptorSize = m_parentDevice->m_d3d12device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_depthStencilViewDescriptorSize = m_parentDevice->m_d3d12device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		UpdateBackBuffers();

		for (UINT n = 0; n < m_numBuffers; n++)
		{
			m_commandAllocators[n] = D3DRenderDevice::CreateCommandAllocator(m_parentDevice->m_d3d12device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		}

		m_commandList = D3DRenderDevice::CreateCommandList(m_parentDevice->m_d3d12device, m_commandAllocators[m_currentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);
		if (m_commandList == nullptr)
			return false;

		hresult = m_commandList->Close();
		if (FAILED(hresult))
		{
			std::string errorMsg = toErrorString(hresult);
			LOG_ERROR(std::string(std::string("m_commandList->Close: ") + errorMsg).c_str());
		}

		// Create synchronization objects.
		m_fence = D3DRenderDevice::CreateFence(m_parentDevice->m_d3d12device);
		if (m_fence == nullptr)
			return false;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = D3DRenderDevice::CreateEventHandle();
		if (m_fenceEvent == nullptr)
			return false;

		// TODO: Find an new method for this guys
		m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
		m_scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

		m_textureUnits = D3DTextureUnitsPtr(new D3DTextureUnits(m_parentDevice));

		m_initialized = true;
		return true;
	}


	D3DRenderContext::~D3DRenderContext(void)
	{
		VRelease();
	}


	void D3DRenderContext::VRelease(void)
	{
		if (m_initialized)
		{
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}

		m_initialized = false;
		m_window = nullptr;
		LOG_TRACE("D3DRenderContext released");
	}


	VertexArrayPtr D3DRenderContext::VCreateVertexArray(MeshAttribute mesh, ShaderVertexAttributeMap shaderAttributes, BufferHint usageHint)
	{
		return VCreateVertexArray(m_device->VCreateMeshBuffers(mesh, shaderAttributes, usageHint));
	}


	VertexArrayPtr D3DRenderContext::VCreateVertexArray(MeshBufferPtr meshBuffers)
	{
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


	VertexArrayPtr D3DRenderContext::VCreateVertexArray()
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		return VertexArrayPtr(new D3DVertexArray());
	}


	FramebufferPtr D3DRenderContext::VCreateFramebuffer()
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		return D3DFramebufferPtr(new D3DFramebuffer());
	}


	void D3DRenderContext::VClear(ClearState clearState)
	{

	}


	void D3DRenderContext::VDraw(PrimitiveType primitiveType, VertexArrayPtr vertexArray, ShaderProgramPtr shaderProgram, RenderState renderState)
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		ApplyRenderState(renderState);
		ApplyVertexArray(vertexArray);
		ApplyShaderProgram(shaderProgram);

		Draw(primitiveType, 0, std::dynamic_pointer_cast<D3DVertexArray>(vertexArray)->MaximumArrayIndex() + 1, vertexArray, shaderProgram, renderState);
	}


	void D3DRenderContext::VDraw(PrimitiveType primitiveType, int offset, int count, VertexArrayPtr vertexArray, ShaderProgramPtr shaderProgram, RenderState renderState)
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


	void D3DRenderContext::VDrawLine(const bow::Vector3<float> &start, const bow::Vector3<float> &end)
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


	void D3DRenderContext::Draw(PrimitiveType primitiveType, int offset, int count, VertexArrayPtr vertexArray, ShaderProgramPtr shaderProgram, RenderState renderState)
	{
		if (m_currentContext != this)
		{
			glfwMakeContextCurrent(m_window);
			m_currentContext = this;
		}

		m_textureUnits->Clean();

		glFlush();
		ApplyFramebuffer();

		D3DVertexArrayPtr D3DVertexArray = std::dynamic_pointer_cast<D3DVertexArray>(vertexArray);
		D3DIndexBufferPtr D3DIndexBuffer = std::dynamic_pointer_cast<D3DIndexBuffer>(D3DVertexArray->VGetIndexBuffer());

		if (D3DIndexBuffer != nullptr)
		{
			if (offset == 0 && count == D3DVertexArray->MaximumArrayIndex() + 1)
			{
				glDrawRangeElements(D3DTypeConverter::To(primitiveType), 0, D3DVertexArray->MaximumArrayIndex(), D3DIndexBuffer->GetCount(), D3DTypeConverter::To(D3DIndexBuffer->GetDatatype()), 0);
			}
			else
			{
				glDrawRangeElements(D3DTypeConverter::To(primitiveType), 0, D3DVertexArray->MaximumArrayIndex(), count, D3DTypeConverter::To(D3DIndexBuffer->GetDatatype()), (void*)(offset * sizeof(unsigned int)));
			}
		}
		else
		{
			glDrawArrays(D3DTypeConverter::To(primitiveType), offset, count);
		}
	}


	void D3DRenderContext::VSetTexture(int location, Texture2DPtr texture)
	{
		LOG_ASSERT(location < m_textureUnits->GetMaxTextureUnits(), "TextureUnit does not Exist");

		m_textureUnits->SetTexture(location, std::dynamic_pointer_cast<D3DTexture2D>(texture));
	}

	void D3DRenderContext::VSetTextureSampler(int location, TextureSamplerPtr sampler)
	{
		m_textureUnits->SetSampler(location, std::dynamic_pointer_cast<D3DTextureSampler>(sampler));
	}


	void D3DRenderContext::VSetFramebuffer(FramebufferPtr framebufer)
	{
		m_setFramebuffer = std::dynamic_pointer_cast<D3DFramebuffer>(framebufer);
	}


	void D3DRenderContext::VSetViewport(Viewport viewport)
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


	Viewport D3DRenderContext::VGetViewport(void)
	{
		return m_viewport;
	}


	void D3DRenderContext::VSwapBuffers(bool vsync)
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


	void D3DRenderContext::ApplyShaderProgram(ShaderProgramPtr shaderProgram)
	{
		D3DShaderProgramPtr D3DShaderProgram = std::dynamic_pointer_cast<D3DShaderProgram>(shaderProgram);

		if (m_boundShaderProgram != D3DShaderProgram)
		{
			D3DShaderProgram->Bind();
			m_boundShaderProgram = D3DShaderProgram;
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


	void D3DRenderContext::ApplyFramebuffer()
	{
		if (m_boundFramebuffer != m_setFramebuffer)
		{
			if (m_setFramebuffer != nullptr)
			{
				m_setFramebuffer->Bind();
			}
			else
			{
				D3DFramebuffer::UnBind();
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

	// -------------------------------------------------------------------------------------- //
	// PRIVATE METHODS
	// -------------------------------------------------------------------------------------- //

	void D3DRenderContext::Resize(unsigned int width, unsigned int height)
	{
		m_width = width;
		m_height = height;

		HRESULT hresult;

		// Flush the GPU queue to make sure the swap chain's back buffers
		// are not being referenced by an in-flight command list.
		Flush();

		for (int i = 0; i < m_numBuffers; ++i)
		{
			// Any references to the back buffers must be released
			// before the swap chain can be resized.
			m_backBuffers[i].Reset();
			m_depthBuffers[i].Reset();
			m_frameFenceValues[i] = m_frameFenceValues[m_currentBackBufferIndex];
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		hresult = m_swapChain->GetDesc(&swapChainDesc);
		if (FAILED(hresult))
		{
			std::string errorMsg = toErrorString(hresult);
			LOG_ERROR(std::string(std::string("m_swapChain->GetDesc: ") + errorMsg).c_str());
		}

		hresult = m_swapChain->ResizeBuffers(m_numBuffers, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags);
		if (FAILED(hresult))
		{
			std::string errorMsg = toErrorString(hresult);
			LOG_ERROR(std::string(std::string("m_swapChain->ResizeBuffers: ") + errorMsg).c_str());
		}

		m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

		UpdateBackBuffers();
	}


	void D3DRenderContext::UpdateBackBuffers()
	{
		// Create frame resources.
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_renderTargetViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < m_numBuffers; n++)
		{
			ComPtr<ID3D12Resource> backBuffer;
			HRESULT hresult = m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_backBuffers[n]));
			if (FAILED(hresult))
			{
				std::string errorMsg = toErrorString(hresult);
				LOG_ERROR(std::string(std::string("m_swapChain->GetBuffer: ") + errorMsg).c_str());
				return;
			}

			m_parentDevice->m_d3d12device->CreateRenderTargetView(m_backBuffers[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_renderTargetViewDescriptorSize);
		}

		int width = std::max((unsigned int)1, m_width);
		int height = std::max((unsigned int)1, m_height);

		// Resize screen dependent resources.
		// Create a depth buffer.
		D3D12_CLEAR_VALUE optimizedClearValue = {};
		optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		optimizedClearValue.DepthStencil = { 1.0f, 0 };

		D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
		dsv.Format = DXGI_FORMAT_D32_FLOAT;
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0;
		dsv.Flags = D3D12_DSV_FLAG_NONE;

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_depthStencilViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT n = 0; n < m_numBuffers; n++)
		{
			HRESULT hresult = m_parentDevice->m_d3d12device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&optimizedClearValue,
				IID_PPV_ARGS(&m_depthBuffers[n])
			);
			if (FAILED(hresult))
			{
				std::string errorMsg = toErrorString(hresult);
				LOG_ERROR(std::string(std::string("m_d3d12device->CreateCommittedResource: ") + errorMsg).c_str());
				return;
			}

			// Update the depth-stencil view.
			m_parentDevice->m_d3d12device->CreateDepthStencilView(m_depthBuffers[n].Get(), &dsv, dsvHandle);
			dsvHandle.Offset(1, m_depthStencilViewDescriptorSize);
		}
	}


	void D3DRenderContext::Flush()
	{
		uint64_t fenceValueForSignal = D3DRenderDevice::Signal(m_directCommandQueue, m_fence, m_fenceValue);
		D3DRenderDevice::WaitForFenceValue(m_fence, fenceValueForSignal, m_fenceEvent);
	}

}

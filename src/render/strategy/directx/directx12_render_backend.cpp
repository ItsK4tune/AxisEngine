#include <render/strategy/directx/directx12_render_backend.h>

#if AXIS_HAS_DIRECTX_BACKEND
#include <render/strategy/directx/directx12_utils.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <algorithm>

#ifdef ENABLE_EDITOR
#include <imgui.h>
#include <imgui_impl_dx12.h>
#endif

namespace rhi
{
DirectX12RenderBackend::DirectX12RenderBackend()
{
}

DirectX12RenderBackend::~DirectX12RenderBackend()
{
    Shutdown();
}

bool DirectX12RenderBackend::Initialize(const RenderBackendCreateInfo& createInfo)
{
    if (m_Initialized)
        return true;

    m_Vsync = createInfo.vsync;

    if (!CreateFactory(createInfo.enableValidation))
        return false;
    if (!PickAdapter())
        return false;
    if (!CreateDeviceAndQueue())
        return false;
    if (!m_RenderDevice.Initialize(m_Device.Get(), m_GraphicsQueue.Get()))
        return false;
    if (createInfo.nativeWindow && !CreateSwapchain(createInfo.nativeWindow, createInfo.width, createInfo.height))
        return false;

    m_NativeWindow = createInfo.nativeWindow;
    m_Swapchain.Resize(createInfo.width, createInfo.height);
    m_Initialized = true;
    return true;
}

void DirectX12RenderBackend::Shutdown()
{
    if (!m_Device)
        return;

#ifdef ENABLE_EDITOR
    ImGuiShutdown();
#endif
    m_RenderDevice.WaitIdle();
    DestroySwapchain();
    m_RenderDevice.Shutdown();
    m_GraphicsQueue.Reset();
    m_Device.Reset();
    m_Adapter.Reset();
    m_Factory.Reset();
    m_Initialized = false;
}

bool DirectX12RenderBackend::BeginFrame()
{
    if (!m_Initialized)
        return false;
    if (m_SwapchainHandle)
    {
        m_CurrentBackBuffer = m_SwapchainHandle->GetCurrentBackBufferIndex();
        if (m_CurrentBackBuffer < m_SwapchainImageHandles.size())
            m_Swapchain.SetCurrentBackBuffer(m_SwapchainImageHandles[m_CurrentBackBuffer]);
    }
    return true;
}

void DirectX12RenderBackend::EndFrame()
{
    if (m_SwapchainHandle)
        m_SwapchainHandle->Present(m_Vsync ? 1 : 0, 0);
}

void DirectX12RenderBackend::OnResize(uint32_t width, uint32_t height)
{
    if (!m_SwapchainHandle)
    {
        m_Swapchain.Resize(width, height);
        return;
    }

    m_RenderDevice.WaitIdle();
    for (auto handle : m_SwapchainImageHandles) m_RenderDevice.UnregisterImage(handle);
    m_SwapchainImageHandles.clear();
    if (!Check(m_SwapchainHandle->ResizeBuffers(kSwapchainBufferCount, width, height, m_SwapchainFormat, 0),
               "IDXGISwapChain::ResizeBuffers"))
        return;
    for (uint32_t i = 0; i < kSwapchainBufferCount; ++i)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
        if (!Check(m_SwapchainHandle->GetBuffer(i, IID_PPV_ARGS(&buffer)), "IDXGISwapChain::GetBuffer"))
            continue;
        ImageDesc desc;
        desc.width = width;
        desc.height = height;
        desc.format = FromDxgiFormat(m_SwapchainFormat);
        desc.usage = ImageUsage::Present | ImageUsage::ColorAttachment;
        m_SwapchainImageHandles.push_back(m_RenderDevice.RegisterSwapchainImage(buffer.Get(), desc));
    }
    m_CurrentBackBuffer = m_SwapchainHandle->GetCurrentBackBufferIndex();
    if (m_CurrentBackBuffer < m_SwapchainImageHandles.size())
        m_Swapchain.SetCurrentBackBuffer(m_SwapchainImageHandles[m_CurrentBackBuffer]);
    m_Swapchain.Resize(width, height);
}

IRenderDevice& DirectX12RenderBackend::GetDevice()
{
    return m_RenderDevice;
}

ISwapchain& DirectX12RenderBackend::GetSwapchain()
{
    return m_Swapchain;
}

BackendType DirectX12RenderBackend::GetBackendType() const
{
    return BackendType::DirectX;
}

const char* DirectX12RenderBackend::GetName() const
{
    return "DirectX 12";
}

void DirectX12RenderBackend::ImGuiInit(void* window)
{
#ifdef ENABLE_EDITOR
    (void)window;
    if (m_ImGuiInitialized || !m_Device || !m_GraphicsQueue || !m_RenderDevice.m_CbvSrvUavHeap)
        return;

    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = m_Device.Get();
    initInfo.CommandQueue = m_GraphicsQueue.Get();
    initInfo.NumFramesInFlight = kSwapchainBufferCount;
    initInfo.RTVFormat = m_SwapchainFormat;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
    initInfo.UserData = this;
    initInfo.SrvDescriptorHeap = m_RenderDevice.m_CbvSrvUavHeap.Get();
    initInfo.SrvDescriptorAllocFn =
        [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* outCpu,
           D3D12_GPU_DESCRIPTOR_HANDLE* outGpu) {
            auto* backend = static_cast<DirectX12RenderBackend*>(info->UserData);
            const uint32_t index = backend->m_RenderDevice.AllocateCbvSrvUav();
            *outCpu = backend->m_RenderDevice.GetCbvSrvUavCpu(index);
            *outGpu = backend->m_RenderDevice.GetCbvSrvUavGpu(index);
        };
    initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE,
                                      D3D12_GPU_DESCRIPTOR_HANDLE) {};

    m_ImGuiInitialized = ImGui_ImplDX12_Init(&initInfo);
#else
    (void)window;
#endif
}

void DirectX12RenderBackend::ImGuiShutdown()
{
#ifdef ENABLE_EDITOR
    if (!m_ImGuiInitialized)
        return;
    ImGui_ImplDX12_Shutdown();
    m_ImGuiInitialized = false;
#endif
}

void DirectX12RenderBackend::ImGuiNewFrame()
{
#ifdef ENABLE_EDITOR
    if (m_ImGuiInitialized)
        ImGui_ImplDX12_NewFrame();
#endif
}

void DirectX12RenderBackend::ImGuiRender()
{
#ifdef ENABLE_EDITOR
    if (!m_ImGuiInitialized)
        return;

    auto* drawData = ImGui::GetDrawData();
    if (!drawData)
        return;

    ImageHandle backBuffer = m_Swapchain.GetCurrentBackBuffer();
    if (!backBuffer)
        return;

    Extent2D extent = m_Swapchain.GetExtent();
    if (extent.width == 0 || extent.height == 0)
        return;

    auto& commandList = m_RenderDevice.BeginCommandList(CommandQueueType::Graphics);

    RenderAttachmentDesc colorAttachment;
    colorAttachment.image = backBuffer;
    colorAttachment.format = m_Swapchain.GetBackBufferFormat();
    colorAttachment.loadOp = LoadOp::Load;
    colorAttachment.storeOp = StoreOp::Store;

    RenderPassBeginInfo beginInfo;
    beginInfo.colorAttachments.push_back(colorAttachment);
    beginInfo.renderArea = {0, 0, extent.width, extent.height};

    commandList.BeginRendering(beginInfo);

    auto* d3dCommandList = dynamic_cast<DirectX12CommandList*>(&commandList);
    if (d3dCommandList)
        ImGui_ImplDX12_RenderDrawData(drawData, d3dCommandList->GetNative());

    commandList.EndRendering();
    m_RenderDevice.Submit(commandList);
#endif
}

bool DirectX12RenderBackend::CreateFactory(bool enableValidation)
{
    UINT flags = 0;
    if (enableValidation)
    {
        Microsoft::WRL::ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
        {
            debug->EnableDebugLayer();
            flags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
    return Check(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_Factory)), "CreateDXGIFactory2");
}

bool DirectX12RenderBackend::PickAdapter()
{
    for (UINT index = 0;; ++index)
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        if (m_Factory->EnumAdapters1(index, &adapter) == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_ADAPTER_DESC1 desc{};
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
        {
            m_Adapter = adapter;
            return true;
        }
    }

    LOGGER_ERROR("DirectX12RHI") << "No hardware adapter supports DirectX 12.";
    return false;
}

bool DirectX12RenderBackend::CreateDeviceAndQueue()
{
    if (!Check(D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device)),
               "D3D12CreateDevice"))
        return false;

    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    return Check(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_GraphicsQueue)), "CreateCommandQueue");
}

bool DirectX12RenderBackend::CreateSwapchain(void* nativeWindow, uint32_t width, uint32_t height)
{
    HWND hwnd = glfwGetWin32Window(static_cast<GLFWwindow*>(nativeWindow));
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = width;
    desc.Height = height;
    desc.Format = m_SwapchainFormat;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = kSwapchainBufferCount;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Scaling = DXGI_SCALING_STRETCH;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapchain1;
    if (!Check(m_Factory->CreateSwapChainForHwnd(m_GraphicsQueue.Get(), hwnd, &desc, nullptr, nullptr, &swapchain1),
               "CreateSwapChainForHwnd"))
        return false;
    Check(m_Factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER), "MakeWindowAssociation");
    if (!Check(swapchain1.As(&m_SwapchainHandle), "Query IDXGISwapChain3"))
        return false;

    m_SwapchainImageHandles.clear();
    for (uint32_t i = 0; i < kSwapchainBufferCount; ++i)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
        if (!Check(m_SwapchainHandle->GetBuffer(i, IID_PPV_ARGS(&buffer)), "IDXGISwapChain::GetBuffer"))
            return false;
        ImageDesc imageDesc;
        imageDesc.width = width;
        imageDesc.height = height;
        imageDesc.format = FromDxgiFormat(m_SwapchainFormat);
        imageDesc.usage = ImageUsage::Present | ImageUsage::ColorAttachment;
        m_SwapchainImageHandles.push_back(m_RenderDevice.RegisterSwapchainImage(buffer.Get(), imageDesc));
    }
    m_CurrentBackBuffer = m_SwapchainHandle->GetCurrentBackBufferIndex();
    if (m_CurrentBackBuffer < m_SwapchainImageHandles.size())
        m_Swapchain.SetCurrentBackBuffer(m_SwapchainImageHandles[m_CurrentBackBuffer]);
    m_Swapchain.Resize(width, height);
    m_Swapchain.SetBackBufferFormat(FromDxgiFormat(m_SwapchainFormat));
    return true;
}

void DirectX12RenderBackend::DestroySwapchain()
{
    for (auto handle : m_SwapchainImageHandles) m_RenderDevice.UnregisterImage(handle);
    m_SwapchainImageHandles.clear();
    m_SwapchainHandle.Reset();
}
}  // namespace rhi
#endif

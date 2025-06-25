// cpp
#include <iostream>
using namespace std;

// Windows
#include <windows.h>
#define WIN32_LEAN_AND_MEAN
#include <tchar.h>

#define GRS_WND_CLASS_NAME _T("GRSWindowClass")
#define GRS_WND_TITLE _T("GRS DX12 Example")

// dx12
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <directx/d3dx12.h>
#include <DirectXMath.h>
using namespace DirectX;
#include <wrl.h>
using namespace Microsoft;
using Microsoft::WRL::ComPtr;

#define Throw(hr)                \
    if (FAILED(hr)) {            \
        cout << "error" << endl; \
    }


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT4 color;
};

int main() {
    // DX12 示例

    const int iWidth = 800;
    const int iHeight = 800;

    HINSTANCE hInstance = GetModuleHandle(nullptr);
    HWND hWnd = nullptr;


    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_GLOBALCLASS;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wcex.lpszClassName = GRS_WND_CLASS_NAME;
    RegisterClassEx(&wcex);


    DWORD dwWndStyle = WS_OVERLAPPED | WS_SYSMENU;
    RECT rtWnd = {0, 0, iWidth, iHeight};
    AdjustWindowRect(&rtWnd, dwWndStyle, FALSE);


    INT posX = (GetSystemMetrics(SM_CXSCREEN) - rtWnd.right - rtWnd.left) / 2;
    INT posY = (GetSystemMetrics(SM_CYSCREEN) - rtWnd.bottom - rtWnd.top) / 2;

    hWnd = CreateWindow(GRS_WND_CLASS_NAME, GRS_WND_TITLE, dwWndStyle, posX, posY, rtWnd.right - rtWnd.left,
                        rtWnd.bottom - rtWnd.top, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        return FALSE;
    }

    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);


    // Dx12 初始化代码

    // 1. DXGI Factory

    ComPtr<IDXGIFactory5> pFactory = nullptr;
    CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory));

    // 1.1 Factory ==> GPU
    /**
     *  Factory ==> (by Adapter) ==> D3D Device (GPU)
     *
     */
    ComPtr<IDXGIAdapter1> pIAdapter = nullptr;
    ComPtr<ID3D12Device> pID3DDevice;
    for (UINT i = 0; pFactory->EnumAdapters1(i, &pIAdapter) != DXGI_ERROR_NOT_FOUND; i++) {
        DXGI_ADAPTER_DESC1 desc = {};
        pIAdapter->GetDesc1(&desc);

        // 不要软件适配器
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }

        if (SUCCEEDED(D3D12CreateDevice(pIAdapter.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr))) {
            // 输出 GPU 信息
            wprintf(L"GPU %d: %s\n", i, desc.Description);

            break;  // 找到合适的 GPU
        }
    }
    Throw(D3D12CreateDevice(pIAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&pID3DDevice)));


    // 2. Device ==> Command Queue
    /**
     * Device ==> CreateCommandQueue ==> Command Queue（3D Engine / Copy / Compute / Video）
     *
     *
     */
    ComPtr<ID3D12CommandQueue> pCommandQueue = nullptr;
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    Throw(pID3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue)));

    // 3. Command Queue ==> Swap Chain
    ComPtr<IDXGISwapChain1> pSwapChain = nullptr;
    ComPtr<IDXGISwapChain4> pSwapChain4 = nullptr;
    int nFrameBackBufCount = 3;
    int nFrameIndex = 0;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = nFrameBackBufCount;
    swapChainDesc.Width = iWidth;
    swapChainDesc.Height = iHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    Throw(pFactory->CreateSwapChainForHwnd(pCommandQueue.Get(),  // Command Queue
                                           hWnd,                 // Window Handle
                                           &swapChainDesc,       // Swap Chain Description
                                           nullptr,              // Fullscreen Desc
                                           nullptr,              // Restrict Output
                                           &pSwapChain           // Swap Chain
                                           ));
    Throw(pSwapChain.As(&pSwapChain4));
    nFrameIndex = pSwapChain4->GetCurrentBackBufferIndex();

    // 4. Swap Chain ==> Render Target View

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = nFrameBackBufCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ComPtr<ID3D12DescriptorHeap> pRTVHeap = nullptr;

    Throw(pID3DDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&pRTVHeap)));
    size_t rtvDescriptorSize = pID3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(pRTVHeap->GetCPUDescriptorHandleForHeapStart());
    ComPtr<ID3D12Resource> pBackBuffers[3];
    for (int i = 0; i < nFrameBackBufCount; i++) {
        Throw(pSwapChain4->GetBuffer(i, IID_PPV_ARGS(&pBackBuffers[i])));
        pID3DDevice->CreateRenderTargetView(pBackBuffers[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, rtvDescriptorSize);
    }

    // 5. Root Signature
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ComPtr<ID3DBlob> pSignature = nullptr;
    ComPtr<ID3DBlob> pError = nullptr;

    Throw(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError));
    ComPtr<ID3D12RootSignature> pRootSignature = nullptr;
    Throw(pID3DDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(),
                                           IID_PPV_ARGS(&pRootSignature)));

    // 6. PSO
    ComPtr<ID3DBlob> vertexShader = nullptr;
    ComPtr<ID3DBlob> pixelShader = nullptr;
    Throw(D3DCompileFromFile(L"D:\\Code\\AnLux\\shaders\\Triangle.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", 0, 0,
                             &vertexShader, &pError));

    Throw(D3DCompileFromFile(L"D:\\Code\\AnLux\\shaders\\Triangle.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", 0, 0,
                             &pixelShader, &pError));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
    psoDesc.pRootSignature = pRootSignature.Get();
    psoDesc.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
    psoDesc.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    ComPtr<ID3D12PipelineState> pPipelineState = nullptr;
    Throw(pID3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pPipelineState)));

    // 7. 准备渲染数据
    Vertex triangleVertices[] = {
        {{0.0f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},   // 顶点1
        {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},  // 顶点3
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}}  // 顶点2

    };

    const UINT vertexBufferSize = sizeof(triangleVertices);
    ComPtr<ID3D12Resource> pVertexBuffer = nullptr;
    Throw(pID3DDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
                                               &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
                                               D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                               IID_PPV_ARGS(&pVertexBuffer)));

    int* pVertexDataBegin = nullptr;
    CD3DX12_RANGE readRange(0, 0);
    Throw(pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
    memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
    pVertexBuffer->Unmap(0, nullptr);


    D3D12_VERTEX_BUFFER_VIEW stVertexBufferView = {};
    stVertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
    stVertexBufferView.StrideInBytes = sizeof(Vertex);
    stVertexBufferView.SizeInBytes = vertexBufferSize;

    // 8. CommandList
    // Need  CommandAllocator & CommandList
    ComPtr<ID3D12CommandAllocator> pCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> pCommandList;
    Throw(pID3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator)));
    Throw(pID3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator.Get(),
                                         pPipelineState.Get(), IID_PPV_ARGS(&pCommandList)));
    Throw(pCommandList->Close());
    // Fence
    HANDLE hFenceEvent = nullptr;
    UINT64 n64FenceValue = 0ui64;
    ComPtr<ID3D12Fence> pFence;

    hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!hFenceEvent) {
        Throw(HRESULT_FROM_WIN32(GetLastError()));
    }
    Throw(pID3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));
    n64FenceValue = 1;
    // 渲染
    // 渲染循环
    MSG msg = {};
    UINT64 fenceValues[3] = {};  // 为每个后台缓冲区存储围栏值

    CD3DX12_VIEWPORT stViewPort(0.0f, 0.0f, static_cast<float>(iWidth), static_cast<float>(iHeight));
    CD3DX12_RECT stScissorRect(0, 0, iWidth, iHeight);

    while (msg.message != WM_QUIT) {
        // 处理所有待处理的窗口消息
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            // --- 开始记录渲染指令 ---

            // 重置命令分配器和命令列表
            Throw(pCommandAllocator->Reset());
            Throw(pCommandList->Reset(pCommandAllocator.Get(), pPipelineState.Get()));

            // 设置图形根签名
            pCommandList->SetGraphicsRootSignature(pRootSignature.Get());

            // 设置视口和裁剪矩形
            D3D12_VIEWPORT viewport = {0.0f, 0.0f, static_cast<float>(iWidth), static_cast<float>(iHeight), 0.0f, 1.0f};
            D3D12_RECT scissorRect = {0, 0, iWidth, iHeight};
            pCommandList->RSSetViewports(1, &viewport);
            pCommandList->RSSetScissorRects(1, &scissorRect);

            // 资源屏障：将当前后台缓冲区从“呈现”状态转换为“渲染目标”状态
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                pBackBuffers[nFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
            pCommandList->ResourceBarrier(1, &barrier);

            // 获取当前后台缓冲区的渲染目标视图 (RTV) 句柄
            CD3DX12_CPU_DESCRIPTOR_HANDLE currentRtvHandle(pRTVHeap->GetCPUDescriptorHandleForHeapStart(), nFrameIndex,
                                                           rtvDescriptorSize);

            // 设置渲染目标
            pCommandList->OMSetRenderTargets(1, &currentRtvHandle, FALSE, nullptr);

            // 清除渲染目标
            const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
            pCommandList->ClearRenderTargetView(currentRtvHandle, clearColor, 0, nullptr);

            // 设置图元拓扑并绑定顶点缓冲
            pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            pCommandList->IASetVertexBuffers(0, 1, &stVertexBufferView);

            // 绘制三角形！
            pCommandList->DrawInstanced(3, 1, 0, 0);

            // 资源屏障：将后台缓冲区从“渲染目标”状态转回“呈现”状态，准备显示
            barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                pBackBuffers[nFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            pCommandList->ResourceBarrier(1, &barrier);

            // --- 结束记录渲染指令 ---
            Throw(pCommandList->Close());

            // 执行命令列表
            ID3D12CommandList* ppCommandLists[] = {pCommandList.Get()};
            pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

            // 呈现帧
            Throw(pSwapChain4->Present(1, 0));

            // --- CPU-GPU 同步 ---

            // 1. 为当前帧设置一个新的围栏点
            const UINT64 currentFenceValue = n64FenceValue;
            Throw(pCommandQueue->Signal(pFence.Get(), currentFenceValue));
            n64FenceValue++;

            // 2. 等待下一帧的资源准备就绪
            nFrameIndex = pSwapChain4->GetCurrentBackBufferIndex();

            // 如果 GPU 还没有处理完我们将要使用的下一帧，则等待
            if (pFence->GetCompletedValue() < fenceValues[nFrameIndex]) {
                Throw(pFence->SetEventOnCompletion(fenceValues[nFrameIndex], hFenceEvent));
                WaitForSingleObject(hFenceEvent, INFINITE);
            }

            // 3. 为下一帧设置新的围栏值
            fenceValues[nFrameIndex] = currentFenceValue;
        }
    }

    return 0;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
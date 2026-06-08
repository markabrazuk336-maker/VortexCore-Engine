// ⚠️ ДЛИННЫЙ КОД — ВСТАВЛЯЙ ПОЛНОСТЬЮ

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")

using namespace DirectX;

bool running = true;

IDXGISwapChain* swapChain = nullptr;
ID3D11Device* device = nullptr;
ID3D11DeviceContext* context = nullptr;
ID3D11RenderTargetView* renderTargetView = nullptr;
ID3D11DepthStencilView* depthView = nullptr;

ID3D11VertexShader* vertexShader = nullptr;
ID3D11PixelShader* pixelShader = nullptr;
ID3D11InputLayout* inputLayout = nullptr;
ID3D11Buffer* vertexBuffer = nullptr;
ID3D11Buffer* indexBuffer = nullptr;
ID3D11Buffer* constantBuffer = nullptr;

float angle = 0.0f;

// =========================

struct Vertex {
    float x, y, z;
};

struct ConstantBuffer {
    XMMATRIX mvp;
};

// =========================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY)
    {
        running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// =========================

bool InitD3D(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;

    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
        NULL, 0, D3D11_SDK_VERSION,
        &scd, &swapChain, &device, NULL, &context);

    ID3D11Texture2D* backBuffer;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    device->CreateRenderTargetView(backBuffer, NULL, &renderTargetView);
    backBuffer->Release();

    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = 1280;
    depthDesc.Height = 720;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depthBuffer;
    device->CreateTexture2D(&depthDesc, NULL, &depthBuffer);
    device->CreateDepthStencilView(depthBuffer, NULL, &depthView);
    depthBuffer->Release();

    context->OMSetRenderTargets(1, &renderTargetView, depthView);

    D3D11_VIEWPORT vp = {};
    vp.Width = 1280;
    vp.Height = 720;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    context->RSSetViewports(1, &vp);

    return true;
}

// =========================

bool InitShaders()
{
    const char* vsCode =
        "cbuffer ConstantBuffer : register(b0) { matrix mvp; };"
        "struct VS_IN { float3 pos : POSITION; };"
        "struct PS_IN { float4 pos : SV_POSITION; };"
        "PS_IN main(VS_IN input){"
        "PS_IN o; o.pos = mul(float4(input.pos,1), mvp); return o;}";

    const char* psCode =
        "float4 main() : SV_TARGET { return float4(0,1,0,1); }";

    ID3DBlob* vsBlob;
    ID3DBlob* psBlob;

    D3DCompile(vsCode, strlen(vsCode), NULL, NULL, NULL, "main", "vs_4_0", 0, 0, &vsBlob, NULL);
    D3DCompile(psCode, strlen(psCode), NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &psBlob, NULL);

    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &vertexShader);
    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &pixelShader);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0}
    };

    device->CreateInputLayout(layout, 1, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);

    vsBlob->Release();
    psBlob->Release();

    return true;
}

// =========================
// КУБ
// =========================

bool InitCube()
{
    Vertex vertices[] =
    {
        {-0.5f,-0.5f,-0.5f}, { -0.5f,0.5f,-0.5f }, { 0.5f,0.5f,-0.5f }, { 0.5f,-0.5f,-0.5f },
        {-0.5f,-0.5f,0.5f},  { -0.5f,0.5f,0.5f },  { 0.5f,0.5f,0.5f },  { 0.5f,-0.5f,0.5f }
    };

    unsigned int indices[] =
    {
        0,1,2, 0,2,3,
        4,6,5, 4,7,6,
        4,5,1, 4,1,0,
        3,2,6, 3,6,7,
        1,5,6, 1,6,2,
        4,0,3, 4,3,7
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA data = { vertices };
    device->CreateBuffer(&bd, &data, &vertexBuffer);

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(indices);
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA idata = { indices };
    device->CreateBuffer(&ibd, &idata, &indexBuffer);

    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.ByteWidth = sizeof(ConstantBuffer);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    device->CreateBuffer(&cbd, NULL, &constantBuffer);

    return true;
}

// =========================

void Render()
{
    angle += 0.01f;

    XMMATRIX world = XMMatrixRotationY(angle) * XMMatrixRotationX(angle * 0.5f);

    XMMATRIX view = XMMatrixLookAtLH(
        XMVectorSet(0, 0, -3, 1),
        XMVectorSet(0, 0, 0, 1),
        XMVectorSet(0, 1, 0, 0)
    );

    XMMATRIX proj = XMMatrixPerspectiveFovLH(
        XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 100.0f);

    XMMATRIX mvp = XMMatrixTranspose(world * view * proj);

    ConstantBuffer cb = { mvp };
    context->UpdateSubresource(constantBuffer, 0, NULL, &cb, 0, 0);

    float color[4] = { 0.05f,0.05f,0.2f,1 };
    context->ClearRenderTargetView(renderTargetView, color);
    context->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH, 1, 0);

    UINT stride = sizeof(Vertex), offset = 0;

    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    context->IASetInputLayout(inputLayout);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->VSSetShader(vertexShader, 0, 0);
    context->PSSetShader(pixelShader, 0, 0);
    context->VSSetConstantBuffers(0, 1, &constantBuffer);

    context->DrawIndexed(36, 0, 0);

    swapChain->Present(1, 0);
}

// =========================

void Cleanup()
{
    if (indexBuffer) indexBuffer->Release();
    if (constantBuffer) constantBuffer->Release();
    if (vertexBuffer) vertexBuffer->Release();
    if (inputLayout) inputLayout->Release();
    if (vertexShader) vertexShader->Release();
    if (pixelShader) pixelShader->Release();
    if (depthView) depthView->Release();
    if (renderTargetView) renderTargetView->Release();
    if (swapChain) swapChain->Release();
    if (context) context->Release();
    if (device) device->Release();
}

// =========================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"VortexWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Vortex Engine",
        WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);

    InitD3D(hwnd);
    InitShaders();
    InitCube();

    MSG msg = {};

    while (running)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) running = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Render();
    }

    Cleanup();
    return 0;
}
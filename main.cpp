#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <wrl/client.h>
#include <math.h>

#pragma comment (lib, "d3d11.lib")

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Popups;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Graphics::Display;
using namespace Microsoft::WRL;

ref class App sealed : public IFrameworkView {
  bool running = true;
  float dpi;
  long renderWidth, renderHeight;
  float clearColor[4] = { 0.4f, 0.2f, 0.2f, 1.0f };
  ComPtr<ID3D11Device3> device;
  ComPtr<ID3D11DeviceContext3> context;
  ComPtr<IDXGISwapChain3> swapChain;
  ComPtr<ID3D11RenderTargetView1> target;

  static void MessageBox(String^ text) {
    auto msg = ref new MessageDialog(text);
    msg->ShowAsync();
  }

  static long DipsToPixels(float dips, float dpi) {
    return (long)floorf(dips * dpi / 96.0f + 0.5f);
  }

  static bool Chk(HRESULT hr, String^ msg) {
    if (FAILED(hr)) {
      MessageBox(msg + " failed");
      return false;
    }
    return true;
  }

public:
  virtual void Initialize(CoreApplicationView^ view) {
    view->Activated +=
      ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>
        (this, &App::OnActivated);
  }

  virtual void SetWindow(CoreWindow^ window) {
    window->Closed +=
      ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>
        (this, &App::OnWindowClosed);
  }

  virtual void Load(String^ entryPoint) {

  }

  void Init(CoreWindow^ window) {
    DisplayInformation^ dpyinfo = DisplayInformation::GetForCurrentView();
    dpi = dpyinfo->LogicalDpi;
    renderWidth = max(1, DipsToPixels(window->Bounds.Width, dpi));
    renderHeight = max(1, DipsToPixels(window->Bounds.Height, dpi));

    ComPtr<ID3D11Device> baseDevice;
    ComPtr<ID3D11DeviceContext> baseContext;
    ComPtr<IDXGIDevice3> dxgiDevice;
    ComPtr<IDXGIAdapter> dxgiAdapter;
    ComPtr<IDXGIFactory4> dxgiFactory;

    HRESULT hr = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0,
      D3D11_CREATE_DEVICE_BGRA_SUPPORT, 0, 0, D3D11_SDK_VERSION,
      &baseDevice, 0, &baseContext);
    bool succ = (
      Chk(hr, "D3D11CreateDevice") &&
      Chk(baseDevice.As(&device), "device3") &&
      Chk(baseContext.As(&context), "context3") &&
      Chk(device.As(&dxgiDevice), "dxgi device") &&
      Chk(dxgiDevice->GetAdapter(&dxgiAdapter), "dxgi adapter") &&
      Chk(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)), "factory") &&
      true
    );
    if (!succ) {
      return;
    }

    ComPtr<IDXGISwapChain1> dxgiSwapChain;
    ComPtr<ID3D11Texture2D1> backBuffer;

    DXGI_SWAP_CHAIN_DESC1 desc = { 0 };
    desc.Width = renderWidth;
    desc.Height = renderHeight;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Stereo = FALSE;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.Flags = 0;
    desc.Scaling = DXGI_SCALING_NONE;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    hr = dxgiFactory->CreateSwapChainForCoreWindow(device.Get(),
      (IUnknown*)window, &desc, 0, &dxgiSwapChain);
    succ = (
      Chk(hr, "CreateSwapChainForCoreWindow") &&
      Chk(dxgiSwapChain.As(&swapChain), "swapchain3") &&
      Chk(dxgiDevice->SetMaximumFrameLatency(1), "setting latency") &&
      Chk(swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)), "backbuf") &&
      true
    );
    if (!succ) {
      return;
    }

    hr = device->CreateRenderTargetView1(backBuffer.Get(), 0, &target);
    if (!Chk(hr, "CreateRenderTargetView1")) {
      return;
    }

    D3D11_VIEWPORT viewport = { 0 };
    viewport.Width = renderWidth;
    viewport.Height = renderHeight;
    context->RSSetViewports(1, &viewport);
  }

  virtual void Run() {
    CoreWindow^ window = CoreWindow::GetForCurrentThread();
    Init(window);
    CoreDispatcher^ disp = window->Dispatcher;
    while (running) {
      disp->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
      context->ClearRenderTargetView(target.Get(), clearColor);
      swapChain->Present(0, 0);
    }
  }

  virtual void Uninitialize() {

  }

protected:
  void OnActivated(CoreApplicationView^ view, IActivatedEventArgs^ args) {
    CoreWindow::GetForCurrentThread()->Activate();
  }

  void OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args) {
    running = false;
  }
};

ref class AppSource sealed : IFrameworkViewSource {
public:
  virtual IFrameworkView^ CreateView() {
    return ref new App();
  }
};

[MTAThread]
int main(Array<String^>^) {
  auto appSource = ref new AppSource();
  CoreApplication::Run(appSource);
  return 0;
}

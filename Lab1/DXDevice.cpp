#include "DXDevice.h"


DXDevice::DXDevice() {
	UINT creationFlags = 0;
	#if defined(_DEBUG)
		creationFlags = D3D11_CREATE_DEVICE_DEBUG;
	#endif
	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE
	};
	uint32_t driverTypesAmount = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};
	uint32_t featureLevelsAmount = ARRAYSIZE(featureLevels);

	HRESULT res = 0;
	bool found = false;
	for (uint32_t i = 0; i < driverTypesAmount; i++)
	{
		res = D3D11CreateDevice(nullptr, driverTypes[i], nullptr, creationFlags, featureLevels,
			featureLevelsAmount, D3D11_SDK_VERSION, &device, &featureLevel, &deviceContext);
		if (SUCCEEDED(res)) {
			found = true;
			break;
		}

	}
	if (!found) {
		throw std::runtime_error("Failed to find suitable device");
	}
	initializeDxgi();
}

DXSwapChain* DXDevice::getSwapChain(Window* window) {
	if (!swapChains.count(window)) {
		swapChains[window] = createSwapChain(window);
	}
	return swapChains[window];
}


DXSwapChain* DXDevice::createSwapChain(Window* window) {
	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BufferCount = DX_SWAPCHAIN_DEFAULT_BUFFER_AMOUNT;
	desc.BufferDesc.Width = window->getWidth();
	desc.BufferDesc.Height = window->getHeight();
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferDesc.RefreshRate.Numerator = 0;
	desc.BufferDesc.RefreshRate.Denominator = 1;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.OutputWindow = window->getWindowHandle();
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	IDXGISwapChain* swapChain;
	if (FAILED(dxgiFactory->CreateSwapChain(device, &desc, &swapChain))) {
		throw std::runtime_error("Failed to create swapChain");
	}
	return new DXSwapChain(dxgiFactory, swapChain, device, window->getWindowHandle(), window->getWidth(), window->getHeight());
}

void DXDevice::initializeDxgi() {
	if (FAILED(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice))))
		throw std::runtime_error("Failed to query dxgi device");
	
	if (FAILED(dxgiDevice->GetParent(IID_PPV_ARGS(&dxgiAdapter))))
		throw std::runtime_error("Failed to query dxgi adapter");
	if (FAILED(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))))
		throw std::runtime_error("Failed to query dxgi factory");
}

ID3D11DeviceContext* DXDevice::getDeviceContext() {
	return deviceContext;
}


DXDevice::~DXDevice() {
	dxgiDevice->Release();
	dxgiAdapter->Release();
	dxgiFactory->Release();
	deviceContext->Release();
	device->Release();
}
#include"DxSwapChain.h"

/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Application/System/Environment.h>

void DxSwapChain::Initialize(
	ComPtr<IDXGIFactory7> dxgiFactory,
	ComPtr<ID3D12CommandQueue> commandQueue,
	HWND hwnd,
	uint32_t width,
	uint32_t height
){
	HRESULT hr;

	// スワップチェイン設定
	swapChainDesc_.Width = width;
	swapChainDesc_.Height = height;
	swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc_.SampleDesc.Count = 1;
	swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc_.BufferCount = 2;
	swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc_.Scaling = DXGI_SCALING_NONE;

	// スワップチェイン作成
	ComPtr<IDXGISwapChain1> tempSwapChain;
	hr = dxgiFactory->CreateSwapChainForHwnd(
		commandQueue.Get(),
		hwnd,
		&swapChainDesc_,
		nullptr,
		nullptr,
		&tempSwapChain
	);
	assert(SUCCEEDED(hr));

	hr = tempSwapChain.As(&swapChain_);
	assert(SUCCEEDED(hr));

	// 0 = VSyncなし。最適化時にFPS上限で隠れないよう、既定はアンロック。
	syncInterval_ = 0;

	// バックバッファリソースを取得
	for (UINT i = 0; i < swapChainDesc_.BufferCount; ++i) {
		hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i]));
		assert(SUCCEEDED(hr));
	}
}

void DxSwapChain::Present(){
	// スワップチェインをPresent
	swapChain_->Present(syncInterval_, 0);
}

void DxSwapChain::ReleaseBackBuffers() {
	// リソースを確実に解放
	for (auto& bb : backBuffers_) {
		bb = nullptr;
		bb.Reset();
	}
}

void DxSwapChain::Resize(uint32_t width, uint32_t height) {
	ReleaseBackBuffers();

	HRESULT hr = swapChain_->ResizeBuffers(
		swapChainDesc_.BufferCount,
		width,
		height,
		swapChainDesc_.Format,
		swapChainDesc_.Flags
	);
	assert(SUCCEEDED(hr));

	swapChainDesc_.Width = width;
	swapChainDesc_.Height = height;

	// リソースを再取得
	for (UINT i = 0; i < swapChainDesc_.BufferCount; ++i) {
		hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i]));
		assert(SUCCEEDED(hr));
	}
}

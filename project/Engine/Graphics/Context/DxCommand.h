#pragma once

// c++
#include <d3d12.h>
#include <wrl.h>
#include <array>
#include <cassert>

/*-----------------------------------------------------------------------------------------
 * DxCommand class
 * - DirectXコマンド関連のリソースを管理するクラス
 * - コマンドリスト/キュー/アロケータの初期化とリセットを行う
 *---------------------------------------------------------------------------------------*/
class DxCommand{
    template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="device"></param>
    void Initialize(const ComPtr<ID3D12Device>& device);
    /// <summary>
    /// リセット
    /// </summary>
    void Reset();
	void Reset(uint32_t frameIndex);

    /////////////////////////////////////////////////////////////////////////////////////////
    //              アクセッサ
    /////////////////////////////////////////////////////////////////////////////////////////    ComPtr<ID3D12GraphicsCommandList> GetCommandList() const{ return commandList_; }
    const ComPtr<ID3D12GraphicsCommandList>& GetCommandList() const{ return commandList_; }
    const ComPtr<ID3D12CommandQueue>& GetCommandQueue() const{ return commandQueue_; }
	static constexpr uint32_t kFrameCount = 2;

private:
    ///////////////////////////////////////////////////
    //              リソース
    ///////////////////////////////////////////////////
    ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
    std::array<ComPtr<ID3D12CommandAllocator>, kFrameCount> commandAllocators_ {};
    ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;
	uint32_t currentAllocatorIndex_ = 0;
};

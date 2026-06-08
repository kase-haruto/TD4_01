#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
/* engine */
#include <Engine/scene/Base/BaseScene.h>

/* ========================================================================
/* GameScene
/* ===================================================================== */
class GameScene final : public BaseScene {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	GameScene();
	~GameScene() override = default;

	void Initialize() override;
	void Update(float dt) override;
	void Draw(ID3D12GraphicsCommandList* cmdLst, class PipelineService* psoService, IRenderTarget*) override;
	void CleanUp() override;
	void LoadAssets() override;
};
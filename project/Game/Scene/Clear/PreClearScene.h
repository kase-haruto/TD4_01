#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
/* engine */
#include <Engine/Extensions/Fog/FogEffect.h>
#include <Engine/Objects/3D/Actor/TestObject/CalyxHuman.h>
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/Scene/Transitioner/IScenePayload.h>
#include <Engine/scene/Base/BaseScene.h>
/* c++ */
#include <memory>
#include <vector>

#include <Game\DemoPlayer\DemoPlayer.h>
#include "Game\StageGimmick\Base\GeneralObject.h"
#include <Game/Scene/Transition/TransitionControl.h>
#include <Game\Scene\Transition\TransitionPayload.h>

/// デバッグ関連///
#ifdef _DEBUG

#include <externals/imgui/imgui.h>
#endif // _DEBUG

/* ========================================================================
/* PreClearScene
/* ===================================================================== */
class PreClearScene final : public BaseScene {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	PreClearScene();
	~PreClearScene() override = default;

	void Initialize() override;
	void Update(float dt) override;
	void Draw(ID3D12GraphicsCommandList* cmdLst, class PipelineService* psoService, IRenderTarget*) override;
	void CleanUp() override;
	void LoadAssets() override;

	void OnPayload(std::unique_ptr<CalyxEngine::IScenePayload> payload) override;

private:
	std::unique_ptr<TransitionPayload> BuildNowTypePayload(SceneType Type);
	void							   AnimUpdate(float dt);

private:
	/* graphics =====================================================*/
	std::unique_ptr<FogEffect> fog_ = nullptr;

	/* objects ====================================================*/
	std::shared_ptr<BaseGameObject> modelField_;
	std::unique_ptr<Sprite>			testSprite_;
	std::shared_ptr<CalyxHuman>		animationHuman_;

	std::shared_ptr<GeneralObject> player_;
	std::shared_ptr<GeneralObject> oni_;

	CalyxEngine::Vector3 firstPos_;
	CalyxEngine::Vector3 firstScale_;
	CalyxEngine::Vector3 flyDir_   = {-2.0f, 0.5f, -2.75f};
	float animTime_ = 2.0f;

	std::unique_ptr<TransitionControl>			transitionControl_ = nullptr;
	std::unique_ptr<CalyxEngine::IScenePayload> payload_;
	SceneType									preType_   = SceneType::SELECT;
	bool										IsPhase_   = false;
	bool										IsOpening_ = false;

	CalyxEngine::EffectAsset effectData_;
};

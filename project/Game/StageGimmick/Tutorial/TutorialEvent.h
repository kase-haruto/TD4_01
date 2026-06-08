#pragma once

#include "Game\StageGimmick\Base\StageGimmickEventBase.h"
#include "Game\StageGimmick\Gimmicks\Projectile\ProjectileObject.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

#include "Engine/Foundation/Reflection/CalyxReflection.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"
#include <Engine/Objects/2D/Object2d/SpriteObject2d.h>

#include <memory>

class SpriteRenderer;

enum TutorialType {
	Jump,
	Dive,
	Purpose
};

/// <summary>
/// チュートリアル表示イベントクラス
/// プレイヤーが触れると時間を止めてチュートリアル画像を表示し、
/// typeに応じた入力で進行/解除する
/// </summary>
CALYX_OBJECT(Category = Event, DisplayName = "TutorialEvent")
class TutorialEvent : public StageGimmickEventBase {
public:
	TutorialEvent() = default;
	TutorialEvent(const std::string& name);
	~TutorialEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "TutorialEvent";
	}

	// 衝突開始時コールバック
	void OnCollisionEnter(Collider* other) override;

	// チュートリアル画像の描画（描画はTestScene等の2Dパスから呼ぶ）
	void DrawSprite(SpriteRenderer* renderer) const;

	// チュートリアルを表示中か（時間停止中か）
	bool IsShowing() const { return state_ == TutorialState::Active; }

protected:
	// 初期化
	void EventInitialize() override;

	// 更新
	void EventUpdate(float dt) override;

	// gui
	void DerivativeGui() override;
	void ApplyDerivedConfigFromJson(const nlohmann::json& root, const nlohmann::json* derived) override;
	void ExtractDerivedConfigToJson(nlohmann::json& root, nlohmann::json& derived) const override;
	void RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) override;

private:

	// チュートリアルの進行状態
	enum class TutorialState {
		Idle,	  //< 未発火
		Active,	  //< 表示中（時間停止）
		Finished, //< 完了（再発火しない）
	};

	// Dive専用のサブ進行状態
	enum class DivePhase {
		WaitFirstInput,	 //< 1回目の入力待ち
		Playing,		 //< 一瞬だけ時間を進めている
		WaitSecondInput, //< 2回目の入力待ち
	};

	// スプライトの遅延生成
	void EnsureSprite();
	// typeに応じてテクスチャを差し替える
	void ApplyTextureForType();
	// Diveの進行更新
	void UpdateDive(float dt);
	// チュートリアル完了（時間を戻して解除）
	void Finish();

	// 確定入力（Aボタン または スペース）が押された瞬間か
	static bool IsConfirmTriggered();

	struct TutorialEventData : public CalyxEngine::SerializableObject {

		int			type			   = 0;
		float		inputBlockDuration = 0.5f; //< 連打対策で入力を無視する時間
		float		diveResumeDuration = 0.3f; //< Dive時に時間を進める長さ
		std::string name_;

		TutorialEventData() {
			AddField("TutorialType", type);
			AddField("InputBlockDuration", inputBlockDuration);
			AddField("DiveResumeDuration", diveResumeDuration);
		}

		void SetEventName(const std::string& name) {
			name_ = name;
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, name_ + "tutorialType", "StageGimmick"};
		}
	};

private:

	TutorialEventData eventData_;
	bool			  hasSerializedEventData_ = false;

	// 表示用スプライト
	std::unique_ptr<CalyxEngine::SpriteObject2d> sprite_ = nullptr;

	// typeごとのテクスチャパス（仮、GUIで変更可）
	std::string jumpTexturePath_	   = "Textures/tutorial/key_rule01.png";
	std::string diveTexturePath_	   = "Textures/tutorial/key_rule02.png";
	std::string purposeTexturePath_	   = "Textures/tutorial/key_rule03.png";
	std::string jumpTexturePadPath_	   = "Textures/tutorial/cont_rule01.png";
	std::string diveTexturePadPath_	   = "Textures/tutorial/cont_rule01.png";
	std::string purposeTexturePadPath_ = "Textures/tutorial/cont_rule01.png";

	// スプライトの表示レイアウト（GUIで変更・保存可）
	CalyxEngine::Vector2 spritePosition_ = {960.0f, 360.0f};
	CalyxEngine::Vector2 spriteScale_	 = {640.0f, 600.0f};

	// 進行状態
	TutorialState state_	 = TutorialState::Idle;
	DivePhase	  divePhase_ = DivePhase::WaitFirstInput;

	float lastDt_ = 0.0f;
	// 入力ブロックタイマー（連打対策）
	float inputBlockTimer_ = 0.0f;
	// Diveの時間再開タイマー
	float diveResumeTimer_ = 0.0f;
};

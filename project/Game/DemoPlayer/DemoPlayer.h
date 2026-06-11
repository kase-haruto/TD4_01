#pragma once
#include "Engine/Application/Effects/EffectAsset.h"
#include "Engine\Objects\3D\Actor\Actor.h"
#include "Engine\Foundation\Serialization\SerializableObject.h"
#include "Engine\Scene\Utility\SceneUtility.h"

#include <Game\DemoHammer\DemoHammer.h>

class TutorialEvent;

class DemoPlayer :
public Actor {
public:
	DemoPlayer();
	DemoPlayer(const std::string&		  modelName,
			   std::optional<std::string> objectName);
	~DemoPlayer() override;

public:
	void Initialize() override;
	void Update(float dt) override;

	//--------- ui/gui --------------------------------------------------

	void DerivativeGui() override;

	//--------- Collision -----------------------------------------------

	void OnCollisionEnter(Collider* other) override;

	void TakeDamage(int32_t damage);
	void StartPitfallRecovery();

	// 触れているチュートリアルイベントを登録する（連打対策中はジャンプ入力を止める）
	void SetActiveTutorial(const TutorialEvent* tutorial) { activeTutorial_ = tutorial; }
	void SetIsTutorialClear(bool is) { isTutorialClear_ = is; }

	//--------- accessor ------------------------------------------------
	std::string_view GetObjectClassName() const override { return "DemoPlayer"; }
	int32_t			 GetLife() const { return life_; }
	int32_t			 GetMaxHP() const { return param_.playerHP; }
	bool			 GetIsTutorialClear() { return isTutorialClear_; }

private:
	struct JumpEvents {
		bool jumpStart = false;
		bool diveStart = false;
	};

	struct LandingEvents {
		bool landed = false;
		bool hardLanded = false;
	};

	void Move(float dt);
	JumpEvents HandleJump(float dt);
	LandingEvents ApplyGravity(float dt);
	void UpdatePopScale(float dt);
	void HammerControl(float dt);
	void DamageFlash(float dt);
	void UpdatePitfallRecovery(float dt);

private:

	struct PlayerParameter : public CalyxEngine::SerializableObject {
		int	  playerHP = 4;
		float moveSpeed = 2.0f;
		float jumpForce = 15.0f;
		float diveForce = -30.0f;
		float gravity	= 40.0f;
		float limit	= 10.0f;

		float stiffness = 400.0f;
		float damping	= 20.0f;

		float diveRotationTime = 0.5f;

		float defaultShockScale = 1.0f;
		float strongShockScale = 1.5f;

		float damageFlashDuration = 1.0f;

		float variableJumpMultiplier = 2.0f;

		float jumpInvincibleTime = 0.3f;

		float pitfallBottomY	= -2.0f;
		float pitfallFallSpeed	= 5.0f;
		float pitfallJumpHeight = 1.0f;

		CalyxEngine::Vector3 colliderOffset = {0.0f, 0.4f, 0.0f};
		CalyxEngine::Vector3 colliderSize = {0.8f, 1.6f, 0.5f};

		CalyxEngine::Vector3 jumpScale = {0.8f, 1.3f, 0.8f};
		CalyxEngine::Vector3 diveScale = {0.9f, 1.2f, 0.9f};
		CalyxEngine::Vector3 landScale = {1.4f, 0.5f, 1.4f};

		CalyxEngine::Vector3 hammerSwingScale = {2.5f, 1.5f, 2.5f};

		struct ShockParm {
			float duration = 0.2f;
			float intensity = 0.5f;
		}shakeParm;

		PlayerParameter() {
			AddField("HP", playerHP).Category("Base Param").Tooltip("体力");
			AddField("Move Speed", moveSpeed).Category("Move Param").Tooltip("移動速度");
			AddField("Jump Force", jumpForce).Category("Move Param").Tooltip("ジャンプの初速");
			AddField("Dive Force", diveForce).Category("Move Param").Tooltip("ダイブの初速");
			AddField("Gravity", gravity).Category("Move Param").Tooltip("重力加速度");
			AddField("MoveLimit", limit).Category("Move Param").Tooltip("左右移動限界");
			AddField("Stiffness", stiffness).Category("Pop Scale").Tooltip("ポップスケールの硬さ（大きいほど速く元に戻る）");
			AddField("Damping", damping).Category("Pop Scale").Tooltip("ポップスケールの減衰（大きいほど揺れが早く収まる）");
			AddField("Dive Rotation Time", diveRotationTime).Category("Move Param");
			AddField("Default ShockScale", defaultShockScale).Category("Shock Power");
			AddField("Strong ShockScale", strongShockScale).Category("Shock Power");
			AddField("Damage Flash Duration", damageFlashDuration).Category("Damage Param");
			AddField("Jump Invincible Time", jumpInvincibleTime).Category("Damage Param").Tooltip("一回押しジャンプ開始時の無敵時間（秒）");
			AddField("Pitfall Bottom Y", pitfallBottomY).Category("Pitfall Param").Tooltip("落とし穴で落下するY座標");
			AddField("Pitfall Fall Speed", pitfallFallSpeed).Category("Pitfall Param").Tooltip("落とし穴に落ちる速度");
			AddField("Pitfall Jump Height", pitfallJumpHeight).Category("Pitfall Param").Tooltip("落とし穴から復帰する時に元のY座標からどれだけ上まで跳ぶか");
			AddField("Collider Offset", colliderOffset).Category("Collider Param");
			AddField("Collider Size", colliderSize).Category("Collider Param");
			AddField("Jump Scale", jumpScale).Category("Pop Scale");
			AddField("Dive Scale", diveScale).Category("Pop Scale");
			AddField("Land Scale", landScale).Category("Pop Scale");
			AddField("Hammer Swing Scale", hammerSwingScale).Category("Pop Scale");

			AddField("cameraShakeDuration",shakeParm.duration).Category("Shock Power").Tooltip("カメラシェイクの時間");
			AddField("cameraShakeIntensity",shakeParm.intensity).Category("Shock Power").Tooltip("カメラシェイクの強さ");
		}
		
		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "DemoPlayer", "Player"};
		}
	};

	PlayerParameter param_;
	bool					isJumping_ = false;
	bool					isDiving_  = false;
	bool					isDivingInvincible_ = false;
	float					jumpInvincibleTimer_ = 0.0f;

	// 触れているチュートリアルイベント（連打対策中はジャンプ入力を止める）
	const TutorialEvent*	activeTutorial_ = nullptr;
	CalyxEngine::Quaternion preRotate_;
	bool					isTutorialClear_ = false;

	// 落とし穴復帰用
	enum class PitfallRecoveryState {
		None,
		Falling,
		JumpingBack,
	};
	PitfallRecoveryState pitfallRecoveryState_ = PitfallRecoveryState::None;
	float				 pitfallReturnY_	   = 0.0f;

	// ダメージ点滅用
	float damageFlashTimer_ = 0.0f;
	bool  isInvincible_	 = false;

	// ハンマーリカバリー用
	bool  isRecovering_	 = false;
	float recoveryTimer_ = 0.0f;
	float recoveryDuration_ = 0.3f; // リカバリー時間（秒）

	// 回転補間用
	float rotationSpeed_ = 10.0f;
	CalyxEngine::Quaternion baseRotation_ = CalyxEngine::Quaternion::MakeIdentity();

	// ジャンプ回転用
	float jumpRotation_			= 0.0f; // 現在の回転角
	float jumpRotationSpeed_	= 0.0f; // 現在の回転速度
	float jumpRotationRemaining_ = 0.0f; // 残りの回転量

	// Pop Scale
	CalyxEngine::Vector3 targetScale_ = {1.0f, 1.0f, 1.0f};
	CalyxEngine::Vector3 scaleVelocity_ = {0.0f, 0.0f, 0.0f};

	CalyxEngine::Vector3 hammerTargetScale_ = {1.0f, 1.0f, 1.0f};
	CalyxEngine::Vector3 hammerScaleVelocity_ = {0.0f, 0.0f, 0.0f};
	CalyxEngine::Vector3 hammerInitialScale_ = {1.0f, 1.0f, 1.0f};

	// ハンマー
	std::shared_ptr<DemoHammer> hammer_;
	bool						firstSetting_ = true;

	CalyxEngine::EffectAsset effectData_;
	CalyxEngine::EffectAsset effectStrongData_;
	CalyxEngine::EffectAsset rollingEffect_;
	CalyxEngine::EffectHandle rollingFxHandle_{};
	CalyxEngine::EffectAsset  jumpEffect_;

	CalyxEngine::Audio hitAudio_;
	CalyxEngine::Audio shockwaveAudio_;
	CalyxEngine::Audio damageAudio_;
};
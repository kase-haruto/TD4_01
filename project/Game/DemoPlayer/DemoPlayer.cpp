#include "DemoPlayer.h"
#include "Engine/Foundation/Input/Input.h"
#include "Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h"
#include "Engine/Scene/Utility/SceneUtility.h"
#include <Game/DemoShockwave/ShockwaveManager.h>
#include <algorithm>
#include <cmath>
#include <numbers>

REGISTER_SCENE_OBJECT(DemoPlayer)

DemoPlayer::DemoPlayer() : Actor() {
	param_.LoadParams();
	moveSpeed_ = param_.moveSpeed;
}

DemoPlayer::DemoPlayer(const std::string& modelName, std::optional<std::string> objectName) : Actor::Actor(modelName, objectName) {
	param_.LoadParams();
	moveSpeed_ = param_.moveSpeed;
}

DemoPlayer::~DemoPlayer() = default;

void DemoPlayer::Initialize() {
	param_.LoadParams();
	moveSpeed_					= param_.moveSpeed;
	life_						= param_.playerHP;
	worldTransform_.translation = {0.0f, 0.0f, 0.0f};
	worldTransform_.scale		= {1.0f, 1.0f, 1.0f};
	velocity_					= {0.0f, 0.0f, 0.0f};
	isJumping_					= false;
	isDiving_					= false;
	isRecovering_				= false;
	recoveryTimer_				= 0.0f;

	damageFlashTimer_ = 0.0f;
	isInvincible_	  = false;
	SetDrawEnable(true);

	baseRotation_				= CalyxEngine::Quaternion::MakeIdentity();
	jumpRotation_				= 0.0f;
	jumpRotationSpeed_			= 0.0f;
	jumpRotationRemaining_		= 0.0f;

	// Pop Scale
	targetScale_   = {1.0f, 1.0f, 1.0f};
	scaleVelocity_ = {0.0f, 0.0f, 0.0f};

	InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		// 敵、イベントオブジェクト、ステージギミックを対象にする
		collider_->SetTargetType(ColliderType::Type_Enemy | ColliderType::Type_EnemyAttack | ColliderType::Type_EventObject | ColliderType::Type_StageGimmick);
		collider_->SetOffset(param_.colliderOffset);
	}

	// 衝撃波マネージャーの初期化（プール作成）
	ShockwaveManager::GetInstance()->Initialize(10);
}

void DemoPlayer::Update(float dt) {
	if(firstSetting_) {
		// 子オブジェクトからハンマーを探す
		hammer_ = nullptr;
		for(auto& child : children_) {
			if(auto h = std::dynamic_pointer_cast<DemoHammer>(child)) {
				hammer_ = h;
				firstSetting_ = false;
				break;
			}
		}
	}

	DamageFlash(dt);

	moveSpeed_ = param_.moveSpeed;

	Move(dt);
	ApplyGravity(dt);
	UpdatePopScale(dt);
	HammerControl(dt);
}

void DemoPlayer::TakeDamage(int32_t damage) {
	if(isInvincible_) {
		return;
	}

	life_ -= damage;
	if(life_ <= 0) {
		life_	 = 0;
		isAlive_ = false;
	}

	damageFlashTimer_ = param_.damageFlashDuration;
	isInvincible_	  = true;
}

void DemoPlayer::DerivativeGui() {
	param_.ShowGui();
}

void DemoPlayer::OnCollisionEnter(Collider* other) {
	BaseGameObject* otherObj = other->GetOwner();
	if(otherObj) {

	}
}

void DemoPlayer::Move(float dt) {
	// 水平移動の入力
	CalyxEngine::Vector3 horizonVelocity = {0.0f, 0.0f, 0.0f};

	if(CalyxFoundation::Input::PushKey(DIK_A)) {horizonVelocity.x -= 1.0f;}
	if(CalyxFoundation::Input::PushKey(DIK_D)) {horizonVelocity.x += 1.0f;}
#ifdef DEVELOP
	if(CalyxFoundation::Input::PushKey(DIK_L)) {
		worldTransform_.translation = {0.0f, worldTransform_.translation.y, 0.0f};
	}
	if(CalyxFoundation::Input::PushKey(DIK_K)) {
		TakeDamage(1);
	}
#endif
	// ずっと前へすすむ
	horizonVelocity.z += 1.0f;

	CalyxEngine::Vector2 leftStick = CalyxFoundation::Input::GetInstance()->GetLeftStick();
	horizonVelocity.x += leftStick.x;

	if(horizonVelocity.Length() > 0.0f) {
		horizonVelocity.Normalize();

		// 回転（移動方向を向く目標）
		CalyxEngine::Vector3	from		   = CalyxEngine::Vector3::Forward();
		CalyxEngine::Vector3	to			   = horizonVelocity;
		CalyxEngine::Quaternion targetRotation = CalyxEngine::Quaternion::FromToQuaternion(from, to);

		// 線形補間(SLERP)による滑らかな回転
		baseRotation_ = CalyxEngine::Quaternion::Slerp(baseRotation_, targetRotation, rotationSpeed_ * dt);
	}

	// 速度の更新（水平成分のみ上書き、垂直成分は維持）
	velocity_.x = horizonVelocity.x * moveSpeed_;
	velocity_.z = horizonVelocity.z * moveSpeed_;

	// ジャンプ入力
	bool jumpTrigger = CalyxFoundation::Input::TriggerKey(DIK_SPACE) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::A);

	float pi = std::numbers::pi_v<float>;
	if(jumpTrigger && !isRecovering_) {
		if(!isJumping_) {
			// ジャンプ開始
			velocity_.y	  = param_.jumpForce;
			isJumping_	  = true;
			isDiving_	  = false;
			jumpRotation_ = 0.0f;
			// 接地状態からジャンプして着地するまでの時間を計算
			float airTime		   = 2.0f * param_.jumpForce / param_.gravity;
			jumpRotationSpeed_	   = (2.0f * pi) / airTime;
			jumpRotationRemaining_ = 2.0f * pi;

			// ジャンプ時に縦に伸びる
			worldTransform_.scale = param_.jumpScale;
			scaleVelocity_		  = {0.0f, 0.0f, 0.0f};

			// 衝撃波を発生（ハンマー振り下ろし）
			ShockwaveManager::GetInstance()->Emit(worldTransform_.translation, param_.defaultShockScale);
		} else if(!isDiving_ && velocity_.y >= -10.0f) {
			velocity_.y = param_.jumpForce;
			isDiving_	= true;
			// 2回追加で回る
			jumpRotationSpeed_	   = (4.0f * pi) / param_.diveRotationTime;
			jumpRotationRemaining_ = 4.0f * pi;
		}
	}

	// ジャンプ回転の更新
	if(isJumping_) {
		if(jumpRotationRemaining_ > 0.0f) {
			float rotateAmount = jumpRotationSpeed_ * dt;
			if(rotateAmount > jumpRotationRemaining_) {
				rotateAmount = jumpRotationRemaining_;
			}
			jumpRotation_ += rotateAmount;
			jumpRotationRemaining_ -= rotateAmount;

			// 回りきったら急降下
			if(isDiving_ && jumpRotationRemaining_ <= 0.0f) {
				velocity_.y	  = param_.diveForce;
				jumpRotation_ = 0.0f; // 回転をデフォルトに戻す

				// 急降下開始時に少し縦に伸ばす
				worldTransform_.scale = param_.diveScale;
			}
		}
	}

	// 最終的な回転を適用 (向き + ジャンプ回転 + 傾き)
	CalyxEngine::Quaternion flipRotation = CalyxEngine::Quaternion::MakeRotateX(jumpRotation_);
	worldTransform_.rotation			 = baseRotation_ * flipRotation;

	worldTransform_.translation += velocity_ * dt;
}

void DemoPlayer::ApplyGravity(float dt) {
	// 重力加算
	velocity_.y -= param_.gravity * dt;

	// 接地判定（Y=0を床とする）
	if(worldTransform_.translation.y <= 0.0f) {
		// 着地した瞬間
		if(isJumping_) {
			// 着地衝撃波（急降下中なら大きく、通常なら少し大きめ）
			if(isDiving_) {
				ShockwaveManager::GetInstance()->Emit(worldTransform_.translation, param_.strongShockScale);
				// リカバリー開始
				isRecovering_  = true;
				recoveryTimer_ = 0.0f;
			}

			worldTransform_.scale = param_.landScale;
			scaleVelocity_		  = {0.0f, 0.0f, 0.0f};
		}

		worldTransform_.translation.y = 0.0f;
		velocity_.y					  = 0.0f;
		isJumping_					  = false;
		isDiving_					  = false;
		jumpRotation_				  = 0.0f;
		jumpRotationRemaining_		  = 0.0f;
	}
}

void DemoPlayer::UpdatePopScale(float dt) {

	CalyxEngine::Vector3 diff		  = worldTransform_.scale - targetScale_;
	CalyxEngine::Vector3 acceleration = (diff * -param_.stiffness) - (scaleVelocity_ * param_.damping);

	scaleVelocity_ += acceleration * dt;
	worldTransform_.scale += scaleVelocity_ * dt;
}

void DemoPlayer::HammerControl(float dt) {
	if(hammer_) {
		float hammerAngle = 0.0f;
		float pi		  = std::numbers::pi_v<float>;

		if(isRecovering_) {
			// 着地後のリカバリー
			recoveryTimer_ += dt;
			float progress = std::clamp(recoveryTimer_ / recoveryDuration_, 0.0f, 1.0f);
			// 90度(pi/2)から0度へ
			hammerAngle = std::lerp(pi * 0.5f, 0.0f, progress);

			if(progress >= 1.0f) {
				isRecovering_ = false;
			}
		} else if(isJumping_) {
			if(isDiving_) {
				// 急降下中は90度
				hammerAngle = pi * 0.5f;
			} else {
				if(jumpRotation_ <= pi / 6.0f) {
					float t		= jumpRotation_ / (pi / 6.0f);
					hammerAngle = std::lerp(0.0f, pi * 0.5f, t);
				} else if(jumpRotation_ <= pi) {
					hammerAngle = pi * 0.5f;
				} else {
					float t		= (jumpRotation_ - pi) / pi;
					hammerAngle = std::lerp(pi * 0.5f, 0.0f, t);
				}
			}
		}

		CalyxEngine::Vector3 swingAxis = CalyxEngine::Quaternion::RotateVector({1.0f, 0.0f, .0f}, worldTransform_.rotation);
		hammer_->SetSwingAngle(swingAxis, -hammerAngle);
	}
}

void DemoPlayer::DamageFlash(float dt) {
	// ダメージ点滅
	if(damageFlashTimer_ > 0.0f) {
		damageFlashTimer_ -= dt;
		// 0.1秒おきに表示/非表示を切り替える
		bool isVisible = (static_cast<int>(damageFlashTimer_ * 10) % 2 == 0);
		SetDrawEnable(isVisible);
		if(hammer_) {
			hammer_->SetDrawEnable(isVisible);
		}

		if(damageFlashTimer_ <= 0.0f) {
			damageFlashTimer_ = 0.0f;
			isInvincible_	  = false;
			SetDrawEnable(true);
			if(hammer_) {
				hammer_->SetDrawEnable(true);
			}
		}
	}
}

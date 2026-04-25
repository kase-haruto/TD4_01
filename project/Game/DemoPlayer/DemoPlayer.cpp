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
	worldTransform_.translation = {0.0f, 0.0f, 0.0f};
	worldTransform_.scale		= {1.0f, 1.0f, 1.0f};
	velocity_					= {0.0f, 0.0f, 0.0f};
	isJumping_					= false;
	isDiving_					= false;
	baseRotation_				= CalyxEngine::Quaternion::MakeIdentity();
	jumpRotation_				= 0.0f;
	jumpRotationSpeed_			= 0.0f;
	jumpRotationRemaining_		= 0.0f;

	// Pop Scale
	targetScale_   = {1.0f, 1.0f, 1.0f};
	scaleVelocity_ = {0.0f, 0.0f, 0.0f};

	// 衝撃波マネージャーの初期化（プール作成）
	ShockwaveManager::GetInstance()->Initialize(10);
}

void DemoPlayer::Update(float dt) {
	moveSpeed_ = param_.moveSpeed;

	Move(dt);
	ApplyGravity(dt);
	UpdatePopScale(dt);
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

	if(CalyxFoundation::Input::PushKey(DIK_A)) {
		horizonVelocity.x -= 1.0f;
	}
	if(CalyxFoundation::Input::PushKey(DIK_D)) {
		horizonVelocity.x += 1.0f;
	}
	if(CalyxFoundation::Input::PushKey(DIK_L)) {
		worldTransform_.translation = {0.0f, 0.0f, 0.0f};
	}
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
	if(jumpTrigger) {
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
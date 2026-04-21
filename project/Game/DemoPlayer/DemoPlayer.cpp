#include "DemoPlayer.h"
#include "Engine/Foundation/Input/Input.h"
#include "Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h"
#include <algorithm>
#include <cmath>
#include <numbers>

REGISTER_SCENE_OBJECT(DemoPlayer)

DemoPlayer::DemoPlayer() : Actor() {
	moveSpeed_ = 7.0f;
}

DemoPlayer::DemoPlayer(const std::string& modelName, std::optional<std::string> objectName) :
Actor::Actor(modelName, objectName){
	moveSpeed_ = 7.0f;
}

DemoPlayer::~DemoPlayer() = default;

void DemoPlayer::Initialize() {
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
	leanAngle_	   = 0.0f;
}

void DemoPlayer::Update(float dt) {
	Move(dt);
	ApplyGravity(dt);
	UpdatePopScale(dt);
}

void DemoPlayer::Move(float dt) {
	// 水平移動の入力
	CalyxEngine::Vector3 horizonVelocity = {0.0f, 0.0f, 0.0f};

	if(CalyxFoundation::Input::PushKey(DIK_A)) {horizonVelocity.x -= 0.2f;}
	if(CalyxFoundation::Input::PushKey(DIK_D)) {horizonVelocity.x += 0.2f;}
	// ずっと前へすすむ
	horizonVelocity.z += 0.2f;

	CalyxEngine::Vector2 leftStick = CalyxFoundation::Input::GetInstance()->GetLeftStick();
	horizonVelocity.x += leftStick.x;

	if(horizonVelocity.Length() > 0.0f) {
		horizonVelocity.Normalize();

		// 回転（移動方向を向く目標）
		CalyxEngine::Vector3 from = CalyxEngine::Vector3::Forward();
		CalyxEngine::Vector3 to	  = horizonVelocity;
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
			velocity_.y			= jumpForce_;
			isJumping_			= true;
			isDiving_			= false;
			jumpRotation_		= 0.0f;
			// 接地状態からジャンプして着地するまでの時間を計算
			float airTime = 2.0f * jumpForce_ / gravity_;
			jumpRotationSpeed_ = (2.0f * pi) / airTime;
			jumpRotationRemaining_ = 2.0f * pi;

			// ジャンプ時に縦に伸びる
			worldTransform_.scale = {0.8f, 1.3f, 0.8f};
			scaleVelocity_		  = {0.0f, 0.0f, 0.0f};
		} else if(!isDiving_ && velocity_.y >= -10.0f) {
			velocity_.y = jumpForce_;
			isDiving_	= true;
			// 2回追加で回る
			float rotationTime = 0.5f; // 0.5秒で回りきる
			jumpRotationSpeed_ = (4.0f * pi) / rotationTime;
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
				velocity_.y = diveForce_;
				jumpRotation_ = 0.0f; // 回転をデフォルトに戻す

				// 急降下開始時に少し縦に伸ばす
				worldTransform_.scale = {0.9f, 1.2f, 0.9f};
			}
		}
	}

	// 最終的な回転を適用 (向き + ジャンプ回転 + 傾き)
	CalyxEngine::Quaternion flipRotation = CalyxEngine::Quaternion::MakeRotateX(jumpRotation_);
	CalyxEngine::Quaternion leanRotation = CalyxEngine::Quaternion::MakeRotateZ(leanAngle_);
	worldTransform_.rotation = baseRotation_ * leanRotation * flipRotation;

	worldTransform_.translation += velocity_ * dt;
}

void DemoPlayer::ApplyGravity(float dt) {
	// 重力加算
	velocity_.y -= gravity_ * dt;

	// 接地判定（Y=0を床とする）
	if(worldTransform_.translation.y <= 0.0f) {
		// 着地した瞬間
		if(isJumping_) {
			worldTransform_.scale = {1.4f, 0.5f, 1.4f};
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
	float stiffness = 400.0f;
	float damping	= 20.0f;

	CalyxEngine::Vector3 diff = worldTransform_.scale - targetScale_;
	CalyxEngine::Vector3 acceleration = (diff * -stiffness) - (scaleVelocity_ * damping);

	scaleVelocity_ += acceleration * dt;
	worldTransform_.scale += scaleVelocity_ * dt;

}
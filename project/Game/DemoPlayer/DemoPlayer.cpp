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
	isDoubleJumping_			= false;
	baseRotation_				= CalyxEngine::Quaternion::MakeIdentity();
	jumpRotation_				= 0.0f;
	jumpRotationTarget_			= 0.0f;
}

void DemoPlayer::Update(float dt) {
	Move(dt);
	ApplyGravity(dt);
}

void DemoPlayer::Move(float dt) {
	// 水平移動の入力
	CalyxEngine::Vector3 horizonVelocity = {0.0f, 0.0f, 0.0f};

	if(CalyxFoundation::Input::PushKey(DIK_A)) {horizonVelocity.x -= 1.0f;}
	if(CalyxFoundation::Input::PushKey(DIK_D)) {horizonVelocity.x += 1.0f;}
	// ずっと前へすすむ
	horizonVelocity.z += 1.0f;

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
			isDoubleJumping_	= false;
			jumpRotation_		= 0.0f;
			jumpRotationTarget_ = 2.0f * pi; // 一回転
		} else if(!isDoubleJumping_) {
			// 二段ジャンプ
			velocity_.y			= jumpForce_;
			isDoubleJumping_	= true;
			jumpRotationTarget_ += 2.0f * pi; // さらに一回転
		}
	}

	// ジャンプ回転の更新
	if(isJumping_) {
		// 回転を滑らかに補間
		jumpRotation_ = std::lerp(jumpRotation_, jumpRotationTarget_, 10.0f * dt);
	}

	// 最終的な回転を適用 (向き + ジャンプ回転)
	CalyxEngine::Quaternion flipRotation = CalyxEngine::Quaternion::MakeRotateX(jumpRotation_);
	worldTransform_.rotation = baseRotation_ * flipRotation;

	worldTransform_.translation += velocity_ * dt;
}

void DemoPlayer::ApplyGravity(float dt) {
	// 重力加算
	velocity_.y -= gravity_ * dt;

	// 接地判定（Y=0を床とする）
	if(worldTransform_.translation.y <= 0.0f) {
		worldTransform_.translation.y = 0.0f;
		velocity_.y					  = 0.0f;
		isJumping_					  = false;
		isDoubleJumping_			  = false;
		jumpRotation_				  = 0.0f;
		jumpRotationTarget_			  = 0.0f;
	}
}
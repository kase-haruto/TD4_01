#include "DemoPlayer.h"
#include "Engine/Foundation/Input/Input.h"
#include "Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h"

REGISTER_SCENE_OBJECT(DemoPlayer)

DemoPlayer::DemoPlayer() : Actor() {
	moveSpeed_ = 7.0f;
}

DemoPlayer::DemoPlayer(const std::string& modelName, std::optional<std::string> objectName) {
	Actor(modelName, objectName);
	moveSpeed_ = 7.0f;
}

DemoPlayer::~DemoPlayer() {
}

void DemoPlayer::Initialize() {
	worldTransform_.translation = {0.0f, 0.0f, 0.0f};
	worldTransform_.scale		= {1.0f, 1.0f, 1.0f};
	velocity_					= {0.0f, 0.0f, 0.0f};
	isJumping_					= false;
}

void DemoPlayer::Update(float dt) {
	Move(dt);
	ApplyGravity(dt);
}

void DemoPlayer::Move(float dt) {
	// 水平移動の入力
	CalyxEngine::Vector3 horizonVelocity = {0.0f, 0.0f, 0.0f};

	//if(CalyxFoundation::Input::PushKey(DIK_W)) {horizonVelocity.z += 1.0f;}
	//if(CalyxFoundation::Input::PushKey(DIK_S)) {horizonVelocity.z -= 1.0f;}
	if(CalyxFoundation::Input::PushKey(DIK_A)) {horizonVelocity.x -= 1.0f;}
	if(CalyxFoundation::Input::PushKey(DIK_D)) {horizonVelocity.x += 1.0f;}
	// ずっと前へすすむ
	horizonVelocity.z += 1.0f;

	CalyxEngine::Vector2 leftStick = CalyxFoundation::Input::GetInstance()->GetLeftStick();
	horizonVelocity.x += leftStick.x;
	//horizonVelocity.z += leftStick.y;

	if(horizonVelocity.Length() > 0.0f) {
		horizonVelocity.Normalize();

		// 回転（移動方向を向く）
		CalyxEngine::Vector3 from = CalyxEngine::Vector3::Forward();
		CalyxEngine::Vector3 to	  = horizonVelocity;
		worldTransform_.rotation  = CalyxEngine::Quaternion::FromToQuaternion(from, to);
	}

	// 速度の更新（水平成分のみ上書き、垂直成分は維持）
	velocity_.x = horizonVelocity.x * moveSpeed_;
	velocity_.z = horizonVelocity.z * moveSpeed_;

	// ジャンプ・急降下入力
	bool jumpTrigger = CalyxFoundation::Input::TriggerKey(DIK_SPACE) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::A);

	if(jumpTrigger) {
		if(!isJumping_) {
			// ジャンプ開始
			velocity_.y = jumpForce_;
			isJumping_	= true;
		} else {
			// 空中での再入力：急降下
			velocity_.y = diveForce_;
		}
	}

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
	}
}

#include "BreakableWallObject.h"

#include <numbers>

#include "Engine/Objects/Collider/BoxCollider.h"
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(BreakableWallObject)

BreakableWallObject::BreakableWallObject(
	const std::string&		   modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void BreakableWallObject::Break(const CalyxEngine::Vector3& position) {

	// 壊れているなら何もしない
	if(isBroken_) {
		return;
	}
	isBroken_ = true;

	// コライダーとモデルを無効化
	if(collider_) {
		collider_->SetCollisionEnabled(false);
	}
	if(model_) {
		BaseGameObject::SetDrawEnable(false);
	}

	const CalyxEngine::Vector3 wallPosition = worldTransform_.GetWorldPosition();
	const float				   maxYaw		= std::numbers::pi_v<float> / 4.0f;
	const float				   yawAngle		= std::clamp(
		   std::atan2(std::abs(position.x - wallPosition.x), std::abs(position.z - wallPosition.z)),
		   0.0f,
		   maxYaw);

	float yaw = 0.0f;
	if(position.x < wallPosition.x) {
		// 引数座標が左側なら、上から見て時計回りに倒す。
		yaw = yawAngle;
	} else if(position.x > wallPosition.x) {
		// 引数座標が右側なら、上から見て反時計回りに倒す。
		yaw = -yawAngle;
	}

	CalyxEngine::Vector3 fallDirection = {
		wallPosition.x - position.x,
		0.0f,
		wallPosition.z - position.z};
	if(fallDirection.LengthSquared() <= 0.0001f) {
		fallDirection = CalyxEngine::Quaternion::RotateVector(
			CalyxEngine::Vector3::Forward(),
			CalyxEngine::Quaternion::MakeRotateY(yaw));
	} else {
		fallDirection = fallDirection.Normalize();
	}
	velocity_ = fallDirection * 1.0f;
	velocity_.y = 1.0f;

	worldTransform_.translation.y = 0.1f;
	worldTransform_.rotation = CalyxEngine::Quaternion::MakeRotateX(-std::numbers::pi_v<float> / 4.0f);
	breakRotation_ =
		CalyxEngine::Quaternion::MakeRotateY(yaw) *
		CalyxEngine::Quaternion::MakeRotateX(std::numbers::pi_v<float> / 2.0f);
}

void BreakableWallObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_StageGimmick);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
		if(auto* radius = dynamic_cast<BoxCollider*>(collider_.get())) {
			radius->SetSize(worldTransform_.scale);
		}
	}
	isBroken_ = false;
}

void BreakableWallObject::ObjectUpdate(float dt) {

	if(isBroken_) {
		if(worldTransform_.GetWorldPosition().y >= 0.0f) {
			const float speed = 5.0f;
			worldTransform_.translation += velocity_ * dt * speed;
			velocity_.y -= 0.1f;
			velocity_ = CalyxEngine::Vector3::Lerp(velocity_, {0.0f, velocity_.y, 0.0f}, 0.1f);
		}
		worldTransform_.rotation = CalyxEngine::Quaternion::Slerp(
			 worldTransform_.rotation, breakRotation_, 0.2f);
	}
}

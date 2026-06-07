#include "DiceSocketObject.h"

#include <numbers>

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine\Foundation\Utility\Ease\CxEase.h>

REGISTER_SCENE_OBJECT(DiceSocketObject)

DiceSocketObject::DiceSocketObject(
	const std::string& modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {

	worldTransform_.scale *= 1.3f;
}

const CalyxEngine::Vector3 DiceSocketObject::GetSocketPos() {
	CalyxEngine::Vector3 position = worldTransform_.translation;
	float scale = 0.47f;
	int index = 1 - clearCount_ + (diceSocketCount_ * 2);
	position.x += scale * static_cast<float>(index);
	return position;
}

const CalyxEngine::Quaternion DiceSocketObject::GetSameNumberRotation() {

	CalyxEngine::Vector3 axis  = CalyxEngine::Vector3::Up();
	float angle = 0.0f;

	if(sameNumbers_ == 1) {
		angle = std::numbers::pi_v<float> / 2.0f;
		axis  = CalyxEngine::Vector3::Right();
	} else if(sameNumbers_ == 3) {
		angle = -std::numbers::pi_v<float> / 2.0f;
	} else if(sameNumbers_ == 4) {
		angle = std::numbers::pi_v<float> / 2.0f;
	} else if(sameNumbers_ == 5) {
		angle = std::numbers::pi_v<float>;
	} else if(sameNumbers_ == 6) {
		angle = -std::numbers::pi_v<float> / 2.0f;
		axis  = CalyxEngine::Vector3::Right();
	}

	return CalyxEngine::Quaternion::MakeRotateAxisQuaternion(axis, angle);
}

void DiceSocketObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	if(collider_) {
		collider_->SetType(ColliderType::Type_StageGimmick);
		collider_->SetTargetType(ColliderType::Type_None);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}

	BaseGameObject::SetColor({1.0f, 0.0f, 0.0f, 1.0f});

	worldTransform_.inheritScale = false;

	sameNumbers_ = static_cast<uint32_t>(rand() % 6 + 1);

	effectLeftData_.Load("CrackerLeftEffect");
	effectRightData_.Load("CrackerRightEffect");

	AudioAPI::Load(socketAudio_, "test");
}

void DiceSocketObject::ObjectUpdate(float dt) {

	if(!isCracker_ && clearCount_ <= diceSocketCount_) {
		if(crackerInterval_ > 0.0f) {
			crackerInterval_ -= dt;
		} else {
			isCracker_ = true;
			CalyxEngine::Vector3 offset = crackerPos_ + worldTransform_.GetWorldPosition();
			offset.x = static_cast<float>(clearCount_ + 1u) * 0.5f + crackerPos_.x;
			EffectAPI::Play(effectRightData_, offset);
			offset.x *= -1.0f;
			EffectAPI::Play(effectLeftData_, offset);
			AudioAPI::Play(socketAudio_, false, 0.5f);
		}
	}
}

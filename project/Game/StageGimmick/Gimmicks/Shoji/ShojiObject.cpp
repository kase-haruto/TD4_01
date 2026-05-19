#include "ShojiObject.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(ShojiObject);

ShojiObject::ShojiObject(
	const std::string&		   modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void ShojiObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
	worldTransform_.translation.y += 1.5f;
	worldTransform_.translation.z += 5.0f;
	worldTransform_.scale = CalyxEngine::Vector3::One() * param_.shojiScale;
	worldTransform_.inheritScale = false;

	const std::string objectName = "ShojiPaperObject";

	for(size_t i = 0; i < paperObjs_.size(); ++i) {

		// 既に存在している場合は生成しない
		if(paperObjs_[i].lock()) {
			continue;
		}
		auto paper = SceneAPI::Instantiate<ShojiPaperObject>("shojiPaper.obj", objectName);
		if(!paper) {
			continue;
		}
		float width	= static_cast<float>(i % 3) * param_.interval;
		float height = static_cast<float>(i / 3) * param_.interval;
		CalyxEngine::Vector3 center = param_.centerPos + CalyxEngine::Vector3{width, -height, 0.0f};
		paper->SetTranslate(center);
		paper->SetParent(shared_from_this());
		paper->Initialize();
		paperObjs_[i] = paper;
	}
}

void ShojiObject::ObjectUpdate(float) {


}

#include "ProjectileObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>

#include "Game\DemoPlayer\DemoPlayer.h"

REGISTER_SCENE_OBJECT(ProjectileObject)

ProjectileObject::ProjectileObject(
	const std::string&		   modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void ProjectileObject::OnCollisionEnter(Collider* other) {

	// ギミックなどへの干渉 or 相手側に追加する
	BaseGameObject* otherObj = other->GetOwner();
	bool isPlayerAttack = other->GetType() == ColliderType::Type_PlayerAttack;
	bool isPlayer = other->GetType() == ColliderType::Type_Player;
	if(otherObj && !isPlayerAttack && !isPlayer) {
		return;
	}
	// コライダーとモデルを無効化
	isHit_		= isPlayer;
	isParry_	= isPlayerAttack;
	if(isHit_) {
		isFlying_ = false;
	}
	targetTime_ = 0.0f;
	if(collider_) {
		collider_->SetCollisionEnabled(false);
	}
}

void ProjectileObject::ObjectInitialize() {

	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player | ColliderType::Type_PlayerAttack);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
	isFlying_ = false;

	worldTransform_.inheritScale = false;

	effectData_.Load("CraneStarEffect");

	AudioAPI::Load(startAudio_, "test");
	AudioAPI::Load(parryAudio_, "test");
}

void ProjectileObject::ObjectUpdate(float dt) {

	if(worldTransform_.translation.y > param_.parryPositionY) {
		return;
	}

	if(worldTransform_.GetWorldPosition().y < 0.0f) {
		velocity_ = CalyxEngine::Vector3::Zero();
		return;
	}

	if(!isFlying_) {
		if(targetTime_< 1.0f && isHit_) {
			targetTime_ += dt;
			if(targetTime_ >= 1.0f) {
				SetDrawEnable(false);
				velocity_ = CalyxEngine::Vector3::Zero();
			}
		}
		// パリーされた時に飛んでいく座標と回転を設定する
		targetPosition_	  = worldTransform_.translation;
		targetPosition_.y = param_.parryPositionY;
		targetRotation_	  = CalyxEngine::Quaternion::LookAt(
			targetPosition_,
			worldTransform_.translation,
			CalyxEngine::Vector3::Up());
	} else {
		if(effectTime_ < 1.0f) {
			if(effectTime_ == 0.0f) {
				EffectAPI::Play(effectData_, worldTransform_.GetWorldPosition() + CalyxEngine::Vector3{0.0f, 0.0f, -2.5f});
				AudioAPI::Play(startAudio_, false, 0.5f);
			}
			effectTime_ += dt;
			return;
		}
		if(param_.targetTime > targetTime_ && !isParry_) {
			targetTime_ += dt;
			auto player = SceneContext::Current()->FindObjectByName<DemoPlayer>("DemoPlayer")->GetWorldTransform().GetWorldPosition();
			velocity_	= CalyxEngine::Vector3{player.x, 0.0f, player.z} - GetWorldTransform().GetWorldPosition();
		}
		if(isParry_) {
			// パリーされたらターゲット方向に時間で飛んでいく
			if(targetTime_ < param_.parryTime) {
				velocity_ = CalyxEngine::Vector3::Zero();
				targetTime_ += dt;
				targetTime_ = std::clamp(targetTime_, 0.0f, param_.parryTime);
				float t = targetTime_ / param_.parryTime;
				if(t == 1.0f) {
					AudioAPI::Play(parryAudio_, false, 0.5f);
				}

				//移動処理
				worldTransform_.translation = CalyxEngine::Vector3::Lerp(
					worldTransform_.translation,
					targetPosition_,
					t);
				// 回転処理
				worldTransform_.rotation = CalyxEngine::Quaternion::Slerp(
					worldTransform_.rotation, targetRotation_, 0.2f);

			} else {
				velocity_ = CalyxEngine::Vector3::Zero();
			}
		}
	}

	if(velocity_.Length() != 0.0f) {
		// 移動処理
		velocity_ = velocity_.Normalize() * param_.speed * dt;
		CalyxEngine::Vector3 prePos = worldTransform_.translation;
		worldTransform_.translation += velocity_;
		// 回転処理
		auto rotation = CalyxEngine::Quaternion::LookAt(
			worldTransform_.translation,
			prePos,
			CalyxEngine::Vector3::Up());
		worldTransform_.rotation = CalyxEngine::Quaternion::Slerp(
			worldTransform_.rotation, rotation, 0.2f);
	}
}

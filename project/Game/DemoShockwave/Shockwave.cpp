#include "Shockwave.h"
#include "Engine/Objects/Collider/SphereCollider.h"
#include "Game\StageGimmick\Gimmicks\GroundSpike\GroundSpikeObject.h"
#include "Game\StageGimmick\Gimmicks\Projectile\ProjectileObject.h"
#include "Game\StageGimmick\Gimmicks\BreakableWall\BreakableWallEvent.h"
#include <algorithm>

Shockwave::Shockwave() : Actor() {
	param_.LoadParams();
}

Shockwave::Shockwave(const std::string& modelName, std::optional<std::string> objectName) : Actor::Actor(modelName, objectName) {
	param_.LoadParams();
}

void Shockwave::Initialize() {
	param_.LoadParams();

	// 球体コライダーを初期化
	InitializeCollider(ColliderKind::Sphere);
	if (collider_) {
		collider_->SetType(ColliderType::Type_PlayerAttack);
		// 敵、イベントオブジェクト、ステージギミックを対象にする
		collider_->SetTargetType(ColliderType::Type_Enemy | ColliderType::Type_EnemyAttack | ColliderType::Type_EventObject | ColliderType::Type_StageGimmick);
		collider_->SetOwner(this);
	}

	SetDrawEnable(false);
	isActive_ = false;
	isStrong_ = false;
	pendingDamages_.clear();
	readyStageDamage_ = 0;
}

void Shockwave::Update(float dt) {
	// 遅延中のダメージを各々カウントダウンし、確定したものを加算する
	for(auto it = pendingDamages_.begin(); it != pendingDamages_.end();) {
		it->timer -= dt;
		if(it->timer <= 0.0f) {
			readyStageDamage_ += it->amount;
			it = pendingDamages_.erase(it);
		} else {
			++it;
		}
	}
	if(collider_) {
		if(!collider_->GetOwner()) {
			collider_->SetOwner(this);
		}
	}
	if (!isActive_) return;

	timer_ += dt;
	if (timer_ >= param_.lifeTime) {
		Deactivate();
		return;
	}

	// 時間経過に合わせてスケールを拡大させる
	float progress = timer_ / param_.lifeTime;
	float easeOut = 1.0f - std::pow(1.0f - progress, 3.0f);
	float currentScale = std::lerp(param_.startScale, currentMaxScale_, easeOut);
	
	if(collider_) {
		if(auto* radius = dynamic_cast<SphereCollider*>(collider_.get())) {
			radius->SetRadius(currentScale);
		}
 		collider_->Update(worldTransform_.translation, worldTransform_.rotation);
	}
	//worldTransform_.scale = {currentScale, currentScale * 0.25f, currentScale};
	worldTransform_.scale = {currentScale, currentScale, currentScale};
	
}

void Shockwave::Activate(const CalyxEngine::Vector3& pos, float scaleMultiplier, bool strong) {
	worldTransform_.translation = pos;
	worldTransform_.scale		= {param_.startScale, param_.startScale, param_.startScale};
	currentMaxScale_			= param_.endScale * scaleMultiplier;
	scaleMultiplier_			= scaleMultiplier;
	timer_						= 0.0f;
	isActive_					= true;
 	isStrong_					= strong;
	pendingDamages_.clear();
	readyStageDamage_			= 0;

	//SetDrawEnable(true);
	if (collider_) {
		collider_->SetCollisionEnabled(true);
	}
}

void Shockwave::Deactivate() {
	isActive_ = false;
	SetDrawEnable(false);
	if (collider_) {
		collider_->SetCollisionEnabled(false);
	}
}

void Shockwave::DerivativeGui() {
	param_.ShowGui();
}

void Shockwave::OnCollisionEnter(Collider* other) {
	if (!isActive_) return;

	// ギミックなどへの干渉 or 相手側に追加する
	BaseGameObject* otherObj = other->GetOwner();
	if(!otherObj) return;

	if(auto* spike = dynamic_cast<GroundSpikeObject*>(otherObj)) {
		// スパイクは即時確定（強衝撃波なら2ダメージ）
		pendingDamages_.push_back({0.0f, isStrong_ ? 2 : 1});
	}

	if(auto* projectile = dynamic_cast<ProjectileObject*>(otherObj)) {
		// ヒットごとに1件積む（複数ヒットが上書きで潰れないように）
		pendingDamages_.push_back({projectile->GetDamageTime(), 1});
	}

}

int Shockwave::ConsumeStageDamage() {
	int result = readyStageDamage_;
	readyStageDamage_ = 0;
	return result;
}

void Shockwave::SetPendingDamage(float time, int damage) {
	pendingDamages_.push_back({time, damage});
}

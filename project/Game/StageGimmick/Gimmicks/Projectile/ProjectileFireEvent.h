#pragma once

#include "Game\StageGimmick\Gimmicks\Projectile\ProjectileObject.h"
#include "Game\StageGimmick\Base\StageGimmickEventBase.h"

/// <summary>
/// 飛んでくる弾のイベントクラス
/// </summary>
class ProjectileFireEvent : public StageGimmickEventBase 
{
public:

	ProjectileFireEvent() = default;
	ProjectileFireEvent(const std::string& name);
	~ProjectileFireEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "ProjectileFireEvent";
	}

	// ターゲットをセットする
	void SetTarget(const std::shared_ptr<ProjectileObject>& target);

	// 衝突開始時コールバック
	void OnCollisionEnter(Collider* other) override;

protected:

	// 初期化
	void EventInitialize() override;

	// 更新
	void EventUpdate(float dt) override;

private:

	// ターゲットの地面スパイクオブジェクト
	std::weak_ptr<ProjectileObject> targetObject_;

	// ホーミングするか
	bool isHoming_ = false;

};

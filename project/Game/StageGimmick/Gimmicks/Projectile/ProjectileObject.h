#pragma once

#include "Game/StageGimmick/Base/StageGimmickObjectBase.h"

/// <summary>
/// 飛んでくる弾のオブジェクトクラス
/// </summary>
class ProjectileObject : public StageGimmickObjectBase 
{
public:

	ProjectileObject() = default;
	ProjectileObject(const std::string& modelName,
					 std::optional<std::string> objectName = std::nullopt);
	~ProjectileObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "ProjectileObject";
	}

	void SetIsFlying(bool isFlying) { isFlying_ = isFlying; }
	void SetIsHoming(bool isHoming) { isHoming_ = isHoming; }

	void OnCollisionEnter(Collider* other) override;

protected:

	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:

	// 飛んでいるか
	bool isFlying_ = false;

	// ホーミングするか
	bool isHoming_ = false;

	// 調整するパラメーター
	float speed_ = 5.0f;


};

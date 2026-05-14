#pragma once

#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

class DiceSocketObject;

/// <summary>
/// サイコロのオブジェクト
/// </summary>
class DiceProjectileObject : public StageGimmickObjectBase
{
public:

	DiceProjectileObject() = default;
	DiceProjectileObject(const std::string& modelName,
					std::optional<std::string> objectName = std::nullopt);
	~DiceProjectileObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "DiceProjectileObject";
	}
	
	// 衝突開始時コールバック
	void OnCollisionEnter(Collider* other) override;

	void SetParam(const DiceProjectileParam& param) {
		param_ = param;
	}
	void SetIsFlying(bool flag) {
		isFlying_ = flag;
	}
	void SetSocket(DiceSocketObject* socket) {
		socket_ = socket;
	}

protected:

	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:

	// 収納箱のポインタ
	DiceSocketObject* socket_ = nullptr;

	// パラメータ
	DiceProjectileParam param_;

	// 飛んでいく座標
	CalyxEngine::Vector3 targetPos_;
	// 飛んでいるか
	bool isFlying_ = false;
	bool isPary_   = false;

};

#pragma once

#include <vector>

#include "Game\StageGimmick\Base\StageGimmickEventBase.h"
#include "Game\StageGimmick\Gimmicks\DroolRain\DroolRainObject.h"

class DroolRainEvent : public StageGimmickEventBase
{
public:

	DroolRainEvent() = default;
	DroolRainEvent(const std::string& name);
	~DroolRainEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "DroolRainEvent";
	}

	// 衝突開始時コールバック
	void OnCollisionEnter(Collider* other) override;
	void OnCollisionExit(Collider* other) override;

protected:

	// 初期化
	void EventInitialize() override;

	// 更新
	void EventUpdate(float dt) override;

private:

	// ターゲットのよだれ雨オブジェクト
	std::vector<std::weak_ptr<DroolRainObject>> droolObjects_;

	// 調整項目
	uint32_t droolCount_ = 5;


};

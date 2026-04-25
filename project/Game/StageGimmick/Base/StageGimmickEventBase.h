#pragma once

#include "Engine\Objects\Event\BaseEventObject.h"

/// <summary>
/// ギミックを発生させるイベントの基底クラス
/// </summary>
class StageGimmickEventBase : public BaseEventObject 
{
public:

	StageGimmickEventBase() = default;
	StageGimmickEventBase(const std::string& name);
	~StageGimmickEventBase() override = default;

	void Initialize() override;
	void AlwaysUpdate(float dt) override;

	std::string_view GetObjectClassName() const override {
		return "StageGimmickEventBase";
	}

protected:
	
	// ギミックの初期化
	virtual void EventInitialize() = 0;

	// ギミックの更新
	virtual void EventUpdate(float dt) = 0;

	// ギミックが作動しているか
	bool isActive_ = true;
};

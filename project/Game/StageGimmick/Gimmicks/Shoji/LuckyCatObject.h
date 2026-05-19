#pragma once

#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

/// <summary>
/// 招き猫オブジェクトクラス
/// </summary>
class LuckyCatObject : public StageGimmickObjectBase 
{
public:

	LuckyCatObject() = default;
	LuckyCatObject(const std::string& modelName,
					 std::optional<std::string> objectName = std::nullopt);
	~LuckyCatObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "LuckyCatObject";
	}

	void SetParam(const ShojiParam& param) {
		param_ = param;
	}	

protected:

	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:

	// パラメータ
	ShojiParam param_;

};

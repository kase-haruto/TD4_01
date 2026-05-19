#pragma once
#include <array>

#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"
#include "Game\StageGimmick\Gimmicks\Shoji\ShojiPaperObject.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

/// <summary>
/// 障子のオブジェクトクラス
/// </summary>
class ShojiObject : public StageGimmickObjectBase
{
public:

	ShojiObject() = default;
	ShojiObject(const std::string& modelName,
						 std::optional<std::string> objectName = std::nullopt);
	~ShojiObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "ShojiObject";
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

	// 障子の配列
	std::array<std::weak_ptr<ShojiPaperObject>, 12> paperObjs_;
	// パラメータ
	ShojiParam param_;

};

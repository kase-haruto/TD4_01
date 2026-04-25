#pragma once

#include <memory>
#include <vector>
#define NOMINMAX

#include "Game/StageGimmick/Gimmicks/BreakableFloor/BreakableFloorEvent.h"
#include "Game/StageGimmick/Gimmicks/BreakableFloor/BreakableFloorObject.h"

/// <summary>
/// ステージギミックを管理するクラス
/// </summary>
class StageGimmickManager {
public:

	// 初期化
	void Initialize();

	// 更新
	void Update(float dt);

	// デバッグ用GUIの表示
	void ShowGui();

private:

	// 壊れる床を作成する
	void CreateBreakableFloor();

	// 壊れる床を削除する
	void DeleteBreakableFloor(size_t index);

	void ReindexBreakableFloorNames();

private:

	// 壊れる床イベントとオブジェクトのリスト
	std::vector<std::shared_ptr<BreakableFloorEvent>>  breakableFloorEvents_;
	std::vector<std::shared_ptr<BreakableFloorObject>> breakableFloorObjects_;
};

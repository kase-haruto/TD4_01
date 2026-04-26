#pragma once

#include <memory>
#include <vector>
#define NOMINMAX

#include "Game/StageGimmick/Gimmicks/BreakableFloor/BreakableFloorEvent.h"
#include "Game/StageGimmick/Gimmicks/BreakableFloor/BreakableFloorObject.h"
#include "Game/StageGimmick/Gimmicks/GroundSpike/GroundSpikeEvent.h"
#include "Game/StageGimmick/Gimmicks/GroundSpike/GroundSpikeObject.h"

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
	void GimmickShowGui(const std::string& gimmickName);

private:

	/// <summary>
	/// ギミックのエントリを表す構造体
	/// </summary>
	struct GimmickEntry {
		std::shared_ptr<StageGimmickEventBase>	event;
		std::shared_ptr<StageGimmickObjectBase> object;
		std::string								name;
	};

private:

	// ギミックを再読み込みする
	void ReloadGimmicks(const std::string& gimmickName);
	// ギミックを作成する
	void CreateGimmick(const std::string& gimmickName);
	// ギミックを削除する
	void DeleteGimmick(size_t index);
	// ギミックの名前を再インデックスする
	void ReindexGimmickNames(const std::string& gimmickName);

private:

	// ギミックのイベントとオブジェクトのリスト
	std::vector<GimmickEntry> gimmicks_;
};

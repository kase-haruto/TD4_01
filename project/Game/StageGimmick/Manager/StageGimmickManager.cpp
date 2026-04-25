#include "StageGimmickManager.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <externals/imgui/imgui.h>

void StageGimmickManager::Initialize() {

	const std::string eventPrefix = "BreakableFloorEvent(";
	const std::string objectPrefix = "BreakableFloorObject(";

	// シーン内のすべてのイベントを確認する
	uint32_t index = 0;
	// 対応するオブジェクト名を作る
	std::string targetName = eventPrefix + std::to_string(index) + ")";
	// シーンから対応するオブジェクトを探す
	auto event = SceneContext::Current()->FindObjectByName<BreakableFloorEvent>(targetName);

	// イベントが見つからなくなるまでループする
	while(event) {
		auto object = SceneContext::Current()->FindObjectByName<BreakableFloorObject>(
			objectPrefix + std::to_string(index) + ")");
		if(object) {
			breakableFloorEvents_.push_back(event);
			breakableFloorObjects_.push_back(object);
		}
		++index;
		targetName = eventPrefix + std::to_string(index) + ")";
		event = SceneContext::Current()->FindObjectByName<BreakableFloorEvent>(targetName);
	}
}

void StageGimmickManager::Update(float dt) {
	dt;
}

void StageGimmickManager::ShowGui() {
	ImGui::Begin("StageGimmickManager");

	if(ImGui::Button("+ BreakableFloor")) {
		CreateBreakableFloor();
	}
	for(int i = 0; i < breakableFloorEvents_.size(); ++i) {
		ImGui::PushID(i);

		std::string label = "Delete BreakableFloor(" + std::to_string(i) + ")";
		if(ImGui::Button(label.c_str())) {
			DeleteBreakableFloor(i);
			ImGui::PopID();
			break;
		}

		ImGui::PopID();
	}
	ImGui::End();
}

void StageGimmickManager::CreateBreakableFloor() {

	// オブジェクトとイベントの名前を作成
	const std::string objectName = "BreakableFloorObject";
	const std::string eventName	 = "BreakableFloorEvent";

	// オブジェクトとイベントをシーンに生成
	auto object = SceneAPI::Instantiate<BreakableFloorObject>(
		"debugCube.obj",
		objectName);

	auto event = SceneAPI::Instantiate<BreakableFloorEvent>(
		eventName);

	event->SetTarget(object);
	object->SetParent(event);

	// リストに追加
	breakableFloorObjects_.push_back(object);
	breakableFloorEvents_.push_back(event);
	ReindexBreakableFloorNames();
}

void StageGimmickManager::DeleteBreakableFloor(size_t index) {
	// インデックスが範囲外なら何もしない
	if(index >= breakableFloorEvents_.size()) {
		return;
	}

	// シーンからオブジェクトとイベントを削除
	auto event	= breakableFloorEvents_[index];
	auto object = breakableFloorObjects_[index];

	// シーンコンテキストが存在する場合に削除処理を行う
	if(auto* ctx = SceneContext::Current()) {
		if(event) ctx->RemoveObject(event);
		if(object) ctx->RemoveObject(object);
	}

	// リストから削除
	breakableFloorEvents_.erase(breakableFloorEvents_.begin() + index);
	breakableFloorObjects_.erase(breakableFloorObjects_.begin() + index);

	ReindexBreakableFloorNames();
}

void StageGimmickManager::ReindexBreakableFloorNames() {
	const size_t n = std::min(breakableFloorEvents_.size(), breakableFloorObjects_.size());

	for(size_t i = 0; i < n; ++i) {
		if(breakableFloorEvents_[i]) {
			breakableFloorEvents_[i]->SetName("BreakableFloorEvent(" + std::to_string(i) + ")", std::nullopt);
		}
		if(breakableFloorObjects_[i]) {
			breakableFloorObjects_[i]->SetName("BreakableFloorObject(" + std::to_string(i) + ")");
		}
	}
}

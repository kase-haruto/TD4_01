#include "StageGimmickManager.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <externals/imgui/imgui.h>

void StageGimmickManager::Initialize() {

	// シーン内のギミックを再読み込みする
	ReloadGimmicks("BreakableFloor");
	ReloadGimmicks("GroundSpike");
}

void StageGimmickManager::Update(float dt) {
	dt;
}

void StageGimmickManager::ShowGui() {

	ImGui::Begin("StageGimmickManager");

	GimmickShowGui("BreakableFloor");
	GimmickShowGui("GroundSpike");
	
	ImGui::End();
}

void StageGimmickManager::GimmickShowGui(const std::string& gimmickName) {

	ImGui::Separator();
	std::string addButtonLabel = "Add_" + gimmickName;
	if(ImGui::Button(addButtonLabel.c_str())) {
		CreateGimmick(gimmickName);
	}
	ImGui::Separator();

	for(int i = 0; i < gimmicks_.size(); ++i) {
		if(gimmickName != gimmicks_[i].name) { continue; }
		ImGui::PushID(i);
		std::string label = "Delete_" + gimmicks_[i].event->GetName();
		if(ImGui::Button(label.c_str())) {
			DeleteGimmick(i);
			ImGui::PopID();
			break;
		}

		ImGui::PopID();
	}
	ImGui::Separator();
}

void StageGimmickManager::ReloadGimmicks(const std::string& gimmickName) {

	const std::string eventPrefix  = gimmickName + "Event(";
	const std::string objectPrefix = gimmickName + "Object(";

	// シーン内のすべてのイベントを確認する
	uint32_t index = 0;
	// 対応するオブジェクト名を作る
	std::string targetName = eventPrefix + std::to_string(index) + ")";
	// オブジェクトとイベントをシーンに生成
	std::shared_ptr<StageGimmickObjectBase> object;
	std::shared_ptr<StageGimmickEventBase>	event;
	// シーンから対応するオブジェクトを探す
	if(gimmickName == "BreakableFloor") {
		auto breakableEvent = SceneContext::Current()->FindObjectByName<BreakableFloorEvent>(targetName);
		event  = breakableEvent;
	} else if(gimmickName == "GroundSpike") {
		auto spikeEvent	 = SceneContext::Current()->FindObjectByName<GroundSpikeEvent>(targetName);
		event  = spikeEvent;
	}

	// イベントが見つからなくなるまでループする
	while(event) {
		if(gimmickName == "BreakableFloor") {
			object = SceneContext::Current()->FindObjectByName<BreakableFloorObject>(
				objectPrefix + std::to_string(index) + ")");
		} else if(gimmickName == "GroundSpike") {
			object = SceneContext::Current()->FindObjectByName<GroundSpikeObject>(
				objectPrefix + std::to_string(index) + ")");
		}
		if(object) {
			gimmicks_.push_back({event, object, gimmickName});
		}
		++index;
		targetName = eventPrefix + std::to_string(index) + ")";
		if(gimmickName == "BreakableFloor") {
			event = SceneContext::Current()->FindObjectByName<BreakableFloorEvent>(targetName);
		} else if(gimmickName == "GroundSpike") {
			event = SceneContext::Current()->FindObjectByName<GroundSpikeEvent>(targetName);
		}
	}

}

void StageGimmickManager::CreateGimmick(const std::string& gimmickName) {

	// オブジェクトとイベントの名前を作成
	const std::string objectName = gimmickName + "Object";
	const std::string eventName	 = gimmickName + "Event";

	// オブジェクトとイベントをシーンに生成
	std::shared_ptr<StageGimmickObjectBase> object;
	std::shared_ptr<StageGimmickEventBase> event;
	if(gimmickName == "BreakableFloor") {
		auto breakableObject = SceneAPI::Instantiate<BreakableFloorObject>("debugCube.obj", objectName);
		auto breakableEvent = SceneAPI::Instantiate<BreakableFloorEvent>(eventName);
		breakableEvent->SetTarget(breakableObject);
		object = breakableObject;
		event  = breakableEvent;
	} else if(gimmickName == "GroundSpike") {
		auto spikeObject = SceneAPI::Instantiate<GroundSpikeObject>("debugCube.obj", objectName);
		auto spikeEvent = SceneAPI::Instantiate<GroundSpikeEvent>(eventName);
		spikeEvent->SetTarget(spikeObject);
		object = spikeObject;
		event  = spikeEvent;
	} else {
		return;
	}
	object->SetParent(event);
	event->Initialize();
	object->Initialize();

	// リストに追加
	gimmicks_.push_back({event, object, gimmickName});
	ReindexGimmickNames(gimmickName);
}

void StageGimmickManager::DeleteGimmick(size_t index) {
	// インデックスが範囲外なら何もしない
	if(index >= gimmicks_.size()) {
		return;
	}

	// シーンからオブジェクトとイベントを削除
	auto event	= gimmicks_[index].event;
	auto object = gimmicks_[index].object;
	const auto name = gimmicks_[index].name;

	// シーンコンテキストが存在する場合に削除処理を行う
	if(auto* ctx = SceneContext::Current()) {
		if(event) ctx->RemoveObject(event);
		if(object) ctx->RemoveObject(object);
	}

	// リストから削除
	gimmicks_.erase(gimmicks_.begin() + index);

	ReindexGimmickNames(name);
}

void StageGimmickManager::ReindexGimmickNames(const std::string& gimmickName) {

	const size_t n = gimmicks_.size();

	size_t index = 0;
	for(size_t i = 0; i < n; ++i) {
		if(gimmicks_[i].name != gimmickName) {
			continue;
		}
		if(gimmicks_[i].event) {
			gimmicks_[i].event->SetName(gimmickName  + "Event(" + std::to_string(index) + ")", std::nullopt);
		}
		if(gimmicks_[i].object) {
			gimmicks_[i].object->SetName(gimmickName + "Object(" + std::to_string(index) + ")");
		}
		++index;
	}
}

#include "StageGimmickManager.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <externals/imgui/imgui.h>

#include "Game/StageGimmick/Gimmicks/BreakableFloor/BreakableFloorEvent.h"
#include "Game/StageGimmick/Gimmicks/GroundSpike/GroundSpikeEvent.h"
#include "Game/StageGimmick/Gimmicks/DroolRain/DroolRainEvent.h"
#include "Game\StageGimmick\Gimmicks\Projectile\ProjectileFireEvent.h"

void StageGimmickManager::Initialize() {

	// シーン内のギミックを再読み込みする
	ReloadGimmicks("BreakableFloor");
	ReloadGimmicks("GroundSpike");
	ReloadGimmicks("DroolRain");
	ReloadGimmicks("Projectile");
}

void StageGimmickManager::Update(float dt) {
	dt;
}

void StageGimmickManager::ShowGui() {

	ImGui::Begin("StageGimmickManager");

	GimmickShowGui("BreakableFloor");
	GimmickShowGui("GroundSpike");
	GimmickShowGui("DroolRain");
	GimmickShowGui("Projectile");
	
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
	std::shared_ptr<StageGimmickEventBase>	event;
	// シーンから対応するオブジェクトを探す
	if(gimmickName == "BreakableFloor") {
		auto breakableEvent = SceneContext::Current()->FindObjectByName<BreakableFloorEvent>(targetName);
		event  = breakableEvent;
	} else if(gimmickName == "GroundSpike") {
		auto spikeEvent	 = SceneContext::Current()->FindObjectByName<GroundSpikeEvent>(targetName);
		event  = spikeEvent;
	} else if(gimmickName == "DroolRain") {
		auto droolEvent = SceneContext::Current()->FindObjectByName<DroolRainEvent>(targetName);
		event  = droolEvent;
	} else if(gimmickName == "Projectile") {
		auto projectileEvent = SceneContext::Current()->FindObjectByName<ProjectileFireEvent>(targetName);
		event  = projectileEvent;
	}
	if(event) {
		event->Initialize();
	}

	// イベントが見つからなくなるまでループする
	while(event) {
		std::vector<std::shared_ptr<StageGimmickObjectBase>> objects;

		if(gimmickName == "BreakableFloor") {
			auto object = SceneContext::Current()->FindObjectByName<BreakableFloorObject>(
				objectPrefix + std::to_string(index) + ")");
			if(object) {
				objects.push_back(object);
			}
		} else if(gimmickName == "GroundSpike") {
			auto object = SceneContext::Current()->FindObjectByName<GroundSpikeObject>(
				objectPrefix + std::to_string(index) + ")");
			if(object) {
				objects.push_back(object);
			}
		} else if(gimmickName == "DroolRain") {
			for(uint32_t i = 0; i < event->GetObjectCount(); ++i) {
				auto object = SceneContext::Current()->FindObjectByName<DroolRainObject>(
					targetName + "/" + objectPrefix + std::to_string(i) + ")");
				if(object) {
					objects.push_back(object);
				}
			}
		} else if(gimmickName == "Projectile") {
			auto object = SceneContext::Current()->FindObjectByName<ProjectileObject>(
				objectPrefix + std::to_string(index) + ")");
			if(object) {
				objects.push_back(object);
			}
		}

		if(!objects.empty()) {
			gimmicks_.push_back({event, objects, gimmickName});
		}

		++index;
		targetName = eventPrefix + std::to_string(index) + ")";

		if(gimmickName == "BreakableFloor") {
			event = SceneContext::Current()->FindObjectByName<BreakableFloorEvent>(targetName);
		} else if(gimmickName == "GroundSpike") {
			event = SceneContext::Current()->FindObjectByName<GroundSpikeEvent>(targetName);
		}else if(gimmickName == "DroolRain") {
			event = SceneContext::Current()->FindObjectByName<DroolRainEvent>(targetName);
		}else if(gimmickName == "Projectile") {
			event = SceneContext::Current()->FindObjectByName<ProjectileFireEvent>(targetName);
		}
	}

}

void StageGimmickManager::CreateGimmick(const std::string& gimmickName) {

	// オブジェクトとイベントの名前を作成
	const std::string objectName = gimmickName + "Object";
	const std::string eventName	 = gimmickName + "Event";

	// オブジェクトとイベントをシーンに生成
	std::vector<std::shared_ptr<StageGimmickObjectBase>> objects;
	std::shared_ptr<StageGimmickEventBase> event;
	if(gimmickName == "BreakableFloor") {

		auto breakableObject = SceneAPI::Instantiate<BreakableFloorObject>("debugCube.obj", objectName);
		auto breakableEvent	 = SceneAPI::Instantiate<BreakableFloorEvent>(eventName);
		breakableEvent->SetTarget(breakableObject);
		breakableObject->SetParent(breakableEvent);
		breakableEvent->Initialize();
		breakableObject->Initialize();
		objects.push_back(breakableObject);
		event = breakableEvent;

	} else if(gimmickName == "GroundSpike") {

		auto spikeObject = SceneAPI::Instantiate<GroundSpikeObject>("debugCube.obj", objectName);
		auto spikeEvent	 = SceneAPI::Instantiate<GroundSpikeEvent>(eventName);
		spikeEvent->SetTarget(spikeObject);
		spikeObject->SetParent(spikeEvent);
		spikeEvent->Initialize();
		spikeObject->Initialize();
		objects.push_back(spikeObject);
		event = spikeEvent;

	} else if(gimmickName == "DroolRain") {
		auto droolEvent = SceneAPI::Instantiate<DroolRainEvent>(eventName);
		droolEvent->Initialize();
		event = droolEvent;
		for(uint32_t i = 0; i < event->GetObjectCount(); ++i) {
			std::string indexedObjectName = objectName + "(" + std::to_string(i) + ")";
			if(event->GetName() == "DroolRainEvent") {
				indexedObjectName = "DroolRainEvent(0)/" + indexedObjectName;
			}
			auto droolObject = SceneAPI::Instantiate<DroolRainObject>("debugCube.obj", indexedObjectName);
			droolObject->SetParent(droolEvent);
			droolObject->Initialize();
			objects.push_back(droolObject);
			droolEvent->SetTarget(droolObject);
		}
	} else if(gimmickName == "Projectile") {

		auto projectileObject = SceneAPI::Instantiate<ProjectileObject>("debugCube.obj", objectName);
		auto projectileEvent = SceneAPI::Instantiate<ProjectileFireEvent>(eventName);
		projectileEvent->SetTarget(projectileObject);
		projectileObject->SetParent(projectileEvent);
		projectileEvent->Initialize();
		projectileObject->Initialize();
		objects.push_back(projectileObject);
		event = projectileEvent;
	}

	gimmicks_.push_back({event, objects, gimmickName});
	ReindexGimmickNames(gimmickName);
}

void StageGimmickManager::DeleteGimmick(size_t index) {
	// インデックスが範囲外なら何もしない
	if(index >= gimmicks_.size()) {
		return;
	}

	// シーンからオブジェクトとイベントを削除
	auto event   = gimmicks_[index].event;
	auto objects = gimmicks_[index].objects;
	const auto name = gimmicks_[index].name;

	// シーンコンテキストが存在する場合に削除処理を行う
	if(auto* ctx = SceneContext::Current()) {
		if(event) { ctx->RemoveObject(event); }
		for(auto& object : objects) {
			if(object) {
				ctx->RemoveObject(object);
			}
		}
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
			gimmicks_[i].event->SetName(
				gimmickName + "Event(" + std::to_string(index) + ")",
				std::nullopt);
		}
		for(size_t objectIndex = 0; objectIndex < gimmicks_[i].objects.size(); ++objectIndex) {
			auto& object = gimmicks_[i].objects[objectIndex];
			const auto& eventName = gimmicks_[i].event->GetName();
			if(!object) {
				continue;
			}
			// 今は1ギミックにつきObjectが1つなので、従来通りの名前にする
			if(gimmicks_[i].objects.size() == 1) {
				object->SetName(
					gimmickName + "Object(" + std::to_string(index) + ")");
			}
			// 将来的に1つのEventに複数Objectを持たせる場合
			else {
				object->SetName(
					eventName + "/" + gimmickName + "Object(" + std::to_string(objectIndex) + ")");
			}
		}
		++index;
	}
}

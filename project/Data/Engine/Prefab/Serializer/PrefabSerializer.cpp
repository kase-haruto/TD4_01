#include "PrefabSerializer.h"
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Foundation/Json/JsonUtils.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>
#include <Engine/Objects/LightObject/PointLight.h>

#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace {

	WorldTransformConfig MakePrefabRootTransformConfig(WorldTransform& transform) {
		WorldTransformConfig config = transform.ExtractConfig();
		config.translation = CalyxEngine::Vector3::Zero();
		config.inheritTranslate = true;
		config.inheritRotate = true;
		config.inheritScale = true;
		return config;
	}

	void WriteSceneObjectMetadata(SceneObject& obj,
								  nlohmann::json& j,
								  const std::unordered_set<SceneObject*>& prefabRoots,
								  bool resetRootTransform,
								  bool usePrefabSourceGuids) {
		j["type"] = obj.GetObjectClassName();
		const Guid guid = (usePrefabSourceGuids && obj.GetPrefabSourceGuid().isValid())
			? obj.GetPrefabSourceGuid()
			: obj.GetGuid();
		j["guid"] = guid;
		j["name"] = obj.GetName();
		j["objectType"] = static_cast<int>(obj.GetObjectType());
		j["drawEnable"] = obj.IsDrawEnable();
		j["castShadow"] = obj.IsCastShadow();
		j["outlineEnabled"] = obj.IsOutlineEnabled();
		j["outlineThickness"] = obj.GetOutlineSettings().thickness;
		j["outlineColor"] = obj.GetOutlineSettings().color;
		if(resetRootTransform && prefabRoots.contains(&obj)) {
			j["worldTransform"] = MakePrefabRootTransformConfig(obj.GetWorldTransform());
		} else {
			j["worldTransform"] = obj.GetWorldTransform().ExtractConfig();
		}
		if(auto parent = obj.GetParent()) {
			if(usePrefabSourceGuids && parent->GetPrefabSourceGuid().isValid()) {
				j["parentGuid"] = parent->GetPrefabSourceGuid();
			} else {
				j["parentGuid"] = parent->GetGuid();
			}
		} else {
			j["parentGuid"] = Guid::Empty();
		}
	}

	void ApplySceneObjectMetadata(SceneObject& obj, const nlohmann::json& j) {
		const int objectType = j.value("objectType", static_cast<int>(obj.GetObjectType()));
		obj.SetName(j.value("name", obj.GetName()), static_cast<ObjectType>(objectType));
		if(obj.GetObjectClassName() == std::string_view("SceneObject")) {
			obj.SetDrawEnable(j.value("drawEnable", obj.IsDrawEnable()));
		}
		obj.SetCastShadow(j.value("castShadow", obj.IsCastShadow()));
		obj.SetOutlineEnabled(j.value("outlineEnabled", obj.IsOutlineEnabled()));
		obj.SetOutlineThickness(j.value("outlineThickness", obj.GetOutlineSettings().thickness));
		obj.SetOutlineColor(j.value("outlineColor", obj.GetOutlineSettings().color));
		if(j.contains("worldTransform")) {
			obj.GetWorldTransform().ApplyConfig(j.at("worldTransform").get<WorldTransformConfig>());
		}
	}

} // namespace

bool PrefabSerializer::Save(const std::vector<SceneObject*>& roots,
							const std::string& path){
	return Save(roots, path, SaveOptions{});
}

bool PrefabSerializer::Save(const std::vector<SceneObject*>& roots,
							const std::string& path,
							const SaveOptions& options){
	nlohmann::json jArray = nlohmann::json::array();
	std::unordered_set<SceneObject*> prefabRoots;
	for(auto* root : roots) {
		if(root) prefabRoots.insert(root);
	}

	std::function<void(SceneObject*)> serializeRec;
	serializeRec = [&] (SceneObject* obj){
		if (!obj || !obj->IsSerializable()) return;

		nlohmann::json j;
		if (auto* cfg = dynamic_cast< IConfigurable* >(obj)){
			cfg->ExtractConfigToJson(j);
		}
		WriteSceneObjectMetadata(*obj, j, prefabRoots, options.resetRootTransform, options.usePrefabSourceGuids);
		jArray.push_back(std::move(j));

		for (auto& childSp : obj->GetChildren()){
			if (childSp) serializeRec(childSp.get());
		}
		};

	for (auto* root : roots) serializeRec(root);

	return CalyxEngine::JsonUtils::Save(path, jArray);
}

std::vector<std::shared_ptr<SceneObject>> PrefabSerializer::Load(const std::string& path) {
	return Load(path, LoadOptions{});
}

std::vector<std::shared_ptr<SceneObject>> PrefabSerializer::Load(const std::string& path,
																 const LoadOptions& options) {
	nlohmann::json jArray;
	if (!CalyxEngine::JsonUtils::Load(path, jArray)) return {};

	std::unordered_map<Guid, std::shared_ptr<SceneObject>> oldToObject;
	std::unordered_map<Guid, Guid> oldToNewGuid;
	std::unordered_map<Guid, std::shared_ptr<SceneObject>> guidMap;

	// 1. インスタンス生成と設定適用 + 新 GUID 割り当て
	for (const auto& j : jArray) {
		std::string typeName = j.value("type", "");
		if (typeName.empty()) continue;

		auto sp = SceneObjectRegistry::Get().Create(typeName);
		if (!sp) continue;

		if (auto* cfg = dynamic_cast<IConfigurable*>(sp.get())) {
			cfg->ApplyConfigFromJson(j);
		}
		ApplySceneObjectMetadata(*sp, j);

		Guid oldGuid = j.value("guid", Guid{});
		Guid newGuid = options.preserveGuids ? oldGuid : Guid::New();
		if(!newGuid.isValid()) {
			newGuid = Guid::New();
		}
		sp->SetGuid(newGuid);
		if(options.prefabAssetGuid.isValid() && oldGuid.isValid()) {
			sp->SetPrefabLink(options.prefabAssetGuid, oldGuid);
		} else if(options.preserveGuids) {
			sp->ClearPrefabLink();
		}
		sp->Initialize();

		oldToNewGuid[oldGuid] = newGuid;
		oldToObject[oldGuid] = sp;
		guidMap[newGuid] = sp;
	}

	// 親子リンク復元
	for (const auto& j : jArray) {
		Guid oldChild = j.value("guid", Guid{});
		Guid oldParent = j.value("parentGuid", Guid{});

		auto newChildIt = oldToNewGuid.find(oldChild);
		auto newParentIt = oldToNewGuid.find(oldParent);
		if (newChildIt == oldToNewGuid.end()) continue;

		auto childSp = guidMap[newChildIt->second];
		if (!childSp) continue;

		if (newParentIt != oldToNewGuid.end()) {
			auto parentSp = guidMap[newParentIt->second];
			if (parentSp) {
				// SetParent のみ（children_ は内部で処理される想定）
				auto& childTransform = childSp->GetWorldTransform();
				childSp->SetParent(parentSp, childTransform.inheritScale);
			}
		}
	}

	// ルートだけでなく、すべてのオブジェクトを返す
	std::vector<std::shared_ptr<SceneObject>> allObjects;
	allObjects.reserve(guidMap.size());
	for (auto& [g, sp] : guidMap) {
		allObjects.push_back(sp);
	}
	return allObjects;
}

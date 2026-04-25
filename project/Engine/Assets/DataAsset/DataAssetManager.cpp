#include "DataAssetManager.h"

#include "MaterialAsset.h"

#include <Engine\Foundation\Serialization\SerializableField.h>
#include <Engine\Foundation\Utility\FileSystem\FileSystemHelper.h>
#include <externals\nlohmann\json.hpp>

#include <fstream>

namespace CalyxEngine {
	namespace {
		nlohmann::json FieldsToJson(const DataAsset& asset) {
			nlohmann::json fields = nlohmann::json::object();
			for(const auto& field : asset.Fields()) {
				nlohmann::json value;
				WriteValue(value, field.ptr);
				fields[field.key] = std::move(value);
			}
			return fields;
		}

		void ApplyFieldsFromJson(DataAsset& asset, const nlohmann::json& root) {
			const nlohmann::json* fields = nullptr;
			if(root.contains("fields") && root["fields"].is_object()) {
				fields = &root["fields"];
			} else if(root.is_object()) {
				fields = &root;
			}
			if(!fields) return;

			for(auto& field : asset.FieldsMutable()) {
				if(!fields->contains(field.key)) continue;
				ReadValue((*fields)[field.key], field.ptr);
			}
		}

		bool ReadJsonFile(const std::filesystem::path& path, nlohmann::json& out) {
			std::ifstream ifs(path);
			if(!ifs) return false;
			try {
				ifs >> out;
			} catch(...) {
				return false;
			}
			return true;
		}

		bool WriteJsonFile(const std::filesystem::path& path, const nlohmann::json& j) {
			FileSystemHelper::CreateDirectoryPath(path.parent_path().string());
			std::ofstream ofs(path);
			if(!ofs) return false;
			ofs << j.dump(2);
			return true;
		}
	}

	void DataAssetManager::RegisterAsset(const std::shared_ptr<DataAsset>& asset) {
		if (!asset) return;
		assets_[asset->GetGuid()] = asset;
	}

	std::shared_ptr<DataAsset> DataAssetManager::GetAsset(const Guid& guid) const {
		auto it = assets_.find(guid);
		if (it != assets_.end()) {
			return it->second;
		}
		return nullptr;
	}

	std::shared_ptr<DataAsset> DataAssetManager::GetAssetByName(const std::string& name) const {
		for (auto& pair : assets_) {
			if (pair.second->GetName() == name) {
				return pair.second;
			}
		}
		return nullptr;
	}

	void DataAssetManager::UnregisterAsset(const Guid& guid) {
		assets_.erase(guid);
	}

	std::shared_ptr<MaterialAsset> DataAssetManager::LoadMaterialAsset(const std::filesystem::path& path, const Guid& guid) {
		if(!guid.isValid()) return nullptr;

		auto material = std::make_shared<MaterialAsset>();
		material->SetGuid(guid);
		material->SetName(path.stem().string());

		nlohmann::json root;
		if(ReadJsonFile(path, root)) {
			material->SetName(root.value("name", material->GetName()));
			ApplyFieldsFromJson(*material, root);
			if(root.contains("graph")) {
				material->graph = root.at("graph").get<NodeGraph>();
			}
		} else {
			SaveAsset(*material, path);
		}

		RegisterAsset(material);
		return material;
	}

	bool DataAssetManager::SaveAsset(const DataAsset& asset, const std::filesystem::path& path) const {
		nlohmann::json root;
		root["type"] = asset.GetAssetTypeName();
		root["guid"] = asset.GetGuid();
		root["name"] = asset.GetName();
		root["fields"] = FieldsToJson(asset);
		if(auto material = dynamic_cast<const MaterialAsset*>(&asset)) {
			root["graph"] = material->graph;
		}
		return WriteJsonFile(path, root);
	}

	std::shared_ptr<MaterialAsset> DataAssetManager::CreateMaterialAsset(const std::filesystem::path& path, const Guid& guid, const std::string& name) {
		auto material = std::make_shared<MaterialAsset>();
		material->SetGuid(guid.isValid() ? guid : Guid::New());
		material->SetName(name.empty() ? path.stem().string() : name);
		if(!SaveAsset(*material, path)) return nullptr;
		RegisterAsset(material);
		return material;
	}

}

#include "DataAssetManager.h"

namespace CalyxEngine {

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

}

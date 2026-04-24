#pragma once

#include "DataAsset.h"
#include <unordered_map>
#include <memory>
#include <string>

namespace CalyxEngine {

	/**
	 * @brief データアセットを一括管理するクラス
	 */
	class DataAssetManager {
	public:
		DataAssetManager() = default;
		~DataAssetManager() = default;

		/**
		 * @brief アセットを登録する
		 */
		inline void RegisterAsset(const std::shared_ptr<DataAsset>& asset) {
			if (!asset) return;
			assets_[asset->GetGuid()] = asset;
		}

		/**
		 * @brief GUIDからアセットを取得する
		 */
		inline std::shared_ptr<DataAsset> GetAsset(const Guid& guid) const {
			auto it = assets_.find(guid);
			if (it != assets_.end()) {
				return it->second;
			}
			return nullptr;
		}

		/**
		 * @brief 型を指定してアセットを取得する（内部でキャスト）
		 */
		template <typename T>
		std::shared_ptr<T> GetAsset(const Guid& guid) const {
			return std::dynamic_pointer_cast<T>(GetAsset(guid));
		}

		/**
		 * @brief 名前からアセットを取得する（主にデバッグ・初期化用）
		 */
		inline std::shared_ptr<DataAsset> GetAssetByName(const std::string& name) const {
			for (auto& pair : assets_) {
				if (pair.second->GetName() == name) {
					return pair.second;
				}
			}
			return nullptr;
		}

		/**
		 * @brief 全アセットのマップを取得
		 */
		const std::unordered_map<Guid, std::shared_ptr<DataAsset>>& GetAssets() const { return assets_; }

		/**
		 * @brief アセットを削除する
		 */
		inline void UnregisterAsset(const Guid& guid) {
			assets_.erase(guid);
		}

	private:
		std::unordered_map<Guid, std::shared_ptr<DataAsset>> assets_;
	};

}

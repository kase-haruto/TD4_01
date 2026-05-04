#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Data/Engine/Configs/Scene/Objects/Material/MaterialConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Transform/UvTransformConfig.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string>

struct BaseModelConfig {
	static constexpr size_t kMaxTextureSlots = 4;

	Guid			  materialGuid{}; // マテリアル GUID
	Transform2DConfig uvTransConfig;  // UV 2D 変換
	int				  blendMode = 5;  // 初期値は通常ブレンド
	std::string		  modelName;	  // モデル名/パス
	Guid			  textureGuid{};  // テクスチャ GUID
	std::array<Guid, kMaxTextureSlots> textureGuids{}; // 複数テクスチャ GUID
	CalyxEngine::Vector4 textureBlendWeights = {1.0f, 0.0f, 0.0f, 0.0f};
	int textureBlendMode = 0; // 0: weighted blend, 1: multiply

	std::optional<std::string> legacyTextureName;
};

inline void to_json(nlohmann::json& j, const BaseModelConfig& c) {
	j = nlohmann::json{
		{"materialGuid", c.materialGuid},
		{"uvTransConfig", c.uvTransConfig},
		{"blendMode", c.blendMode},
		{"modelName", c.modelName},
		{"textureGuid", c.textureGuid},
		{"textureGuids", c.textureGuids},
		{"textureBlendWeights", c.textureBlendWeights},
		{"textureBlendMode", c.textureBlendMode},
	};
	// legacyTextureName は保存しない
}

inline void from_json(const nlohmann::json& j, BaseModelConfig& c) {
	if(auto it = j.find("materialGuid"); it != j.end() && !it->is_null()) {
		c.materialGuid = it->get<Guid>();
	} else {
		c.materialGuid = Guid::Empty();
	}
	j.at("uvTransConfig").get_to(c.uvTransConfig);
	j.at("blendMode").get_to(c.blendMode);
	j.at("modelName").get_to(c.modelName);

	if(auto it = j.find("textureGuid"); it != j.end() && !it->is_null()) {
		c.textureGuid = it->get<Guid>();
	} else {
		c.textureGuid = Guid::Empty();
	}

	c.textureGuids.fill(Guid::Empty());
	if(auto it = j.find("textureGuids"); it != j.end() && it->is_array()) {
		const size_t count = it->size() < c.textureGuids.size() ? it->size() : c.textureGuids.size();
		for(size_t i = 0; i < count; ++i) {
			if(!(*it)[i].is_null()) {
				c.textureGuids[i] = (*it)[i].get<Guid>();
			}
		}
	}
	if(!c.textureGuids[0].isValid()) {
		c.textureGuids[0] = c.textureGuid;
	}

	if(auto it = j.find("textureBlendWeights"); it != j.end() && !it->is_null()) {
		c.textureBlendWeights = it->get<CalyxEngine::Vector4>();
	} else {
		c.textureBlendWeights = {1.0f, 0.0f, 0.0f, 0.0f};
	}

	if(auto it = j.find("textureBlendMode"); it != j.end() && it->is_number_integer()) {
		c.textureBlendMode = it->get<int>();
	} else {
		c.textureBlendMode = 0;
	}

	if(auto it2 = j.find("textureName"); it2 != j.end() && it2->is_string()) {
		c.legacyTextureName = it2->get<std::string>();
	} else {
		c.legacyTextureName.reset();
	}
}

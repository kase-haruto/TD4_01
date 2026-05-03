#include "MaterialAsset.h"

namespace CalyxEngine {

	MaterialAsset::MaterialAsset() {
		name_ = "New Material";
		RegisterFields();
	}

	void MaterialAsset::RegisterFields() {
		// SerializableObject の AddField を使用してパラメータを登録
		AddField("color", color);
		AddField("lightingMode", lightingMode);
		AddField("shininess", shininess);
		AddField("isReflect", isReflect);
		AddField("envirometCoefficient", envirometCoefficient);
		AddField("roughness", roughness);
		// uvTransform のシリアライズは必要に応じて追加
	}

}

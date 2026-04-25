#pragma once

#include "DataAsset.h"
#include <Engine/Editor/NodeEditor/NodeGraph.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Lighting/LightData.h>

namespace CalyxEngine {

	/**
	 * @brief マテリアルのパラメータを保持するデータアセット
	 */
	class MaterialAsset : public DataAsset {
	public:
		MaterialAsset();
		virtual ~MaterialAsset() = default;

		std::string GetAssetTypeName() const override { return "MaterialAsset"; }

		// --- Parameters ---
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		int32_t lightingMode = 2; // Default to Toon (as previously requested)
		float   shininess = 20.0f;
		bool    isReflect = false;
		float   envirometCoefficient = 0.5f;
		float   roughness = 0.5f;
		Matrix4x4 uvTransform = Matrix4x4::MakeIdentity();
		NodeGraph graph;

	private:
		void RegisterFields();
	};

}

#pragma once

#include <Engine\Application\UI\EngineUI\IEngineUI.h>
#include <Engine\Editor\NodeEditor\NodeEditorCanvas.h>
#include <Engine\Foundation\Utility\Guid\Guid.h>

namespace CalyxEngine {
	class MaterialAsset;

	class MaterialNodeEditorPanel : public IEngineUI {
	public:
		MaterialNodeEditorPanel();
		void Render() override;

	private:
		void DrawMaterialList();
		void DrawToolbar(MaterialAsset& material);
		bool DrawContextMenu(MaterialAsset& material, const NodeEditorCanvas::ContextMenu& menu);
		void CreateMaterialAsset();
		bool DrawNodeBody(Node& node);
		void AddColorNode(MaterialAsset& material, Vector2 position);
		void AddFloatNode(MaterialAsset& material, const char* type, const char* title, Vector2 position);
		void AddBoolNode(MaterialAsset& material, const char* type, const char* title, Vector2 position);
		void AddBinaryNode(MaterialAsset& material, const char* type, const char* title, NodeValueType valueType, Vector2 position);
		void EnsureOutputNode(MaterialAsset& material);
		Vector4 EvaluateColor(const MaterialAsset& material, int32_t inputPinId, const Vector4& fallback) const;
		float EvaluateFloat(const MaterialAsset& material, int32_t inputPinId, float fallback) const;
		bool EvaluateBool(const MaterialAsset& material, int32_t inputPinId, bool fallback) const;
		void Evaluate(MaterialAsset& material);
		void Save(MaterialAsset& material);

	private:
		Guid selectedMaterial_;
		NodeEditorCanvas canvas_;
	};
}

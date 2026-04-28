#pragma once

#include <Engine\Application\UI\EngineUI\IEngineUI.h>
#include <Engine\Editor\NodeEditor\NodeEditorCanvas.h>
#include <Engine\Foundation\Utility\Guid\Guid.h>

#include <array>
#include <filesystem>

namespace CalyxEngine {
	class MaterialAsset;

	class MaterialNodeEditorPanel : public IEngineUI {
	public:
		MaterialNodeEditorPanel();
		void Render() override;

	private:
		void DrawMaterialList();
		void DrawToolbar(MaterialAsset& material);
		bool DrawAddNodeMenu(MaterialAsset& material, Vector2 position);
		bool DrawContextMenu(MaterialAsset& material, const NodeEditorCanvas::ContextMenu& menu);
		bool DrawLightingModePopup(MaterialAsset& material);
		void CreateMaterialAsset();
		void BeginRenameMaterial(const Guid& guid, const std::filesystem::path& path);
		void CommitRenameMaterial();
		void CancelRenameMaterial();
		bool DrawNodeBody(Node& node);
		void AddColorNode(MaterialAsset& material, Vector2 position);
		void AddFloatNode(MaterialAsset& material, const char* type, const char* title, Vector2 position);
		void AddBoolNode(MaterialAsset& material, const char* type, const char* title, Vector2 position);
		void AddLightingModeNode(MaterialAsset& material, Vector2 position);
		void AddBinaryNode(MaterialAsset& material, const char* type, const char* title, NodeValueType valueType, Vector2 position);
		void EnsureOutputNode(MaterialAsset& material);
		Vector4 EvaluateColor(const MaterialAsset& material, int32_t inputPinId, const Vector4& fallback) const;
		float EvaluateFloat(const MaterialAsset& material, int32_t inputPinId, float fallback) const;
		bool EvaluateBool(const MaterialAsset& material, int32_t inputPinId, bool fallback) const;
		int32_t EvaluateInt(const MaterialAsset& material, int32_t inputPinId, int32_t fallback) const;
		void Evaluate(MaterialAsset& material);
		void Save(MaterialAsset& material);

	private:
		Guid selectedMaterial_;
		NodeEditorCanvas canvas_;
		Guid renamingMaterial_;
		std::filesystem::path renamingPath_;
		std::array<char, 256> renameBuffer_{};
		bool focusRename_ = false;
		bool renamePopupRequested_ = false;
		bool lightingModePopupRequested_ = false;
		int32_t lightingModePopupNodeId_ = 0;
		Vector2 lightingModePopupPos_{0.0f, 0.0f};
	};
}

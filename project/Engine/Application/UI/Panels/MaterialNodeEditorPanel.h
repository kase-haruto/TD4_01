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
		bool DrawNodeBody(Node& node);
		void AddColorNode(MaterialAsset& material);
		void AddFloatNode(MaterialAsset& material, const char* type, const char* title);
		void AddBoolNode(MaterialAsset& material, const char* type, const char* title);
		void EnsureOutputNode(MaterialAsset& material);
		void Evaluate(MaterialAsset& material);
		void Save(MaterialAsset& material);

	private:
		Guid selectedMaterial_;
		NodeEditorCanvas canvas_;
	};
}

#include "MaterialNodeEditorPanel.h"

#include <Engine\Assets\Database\AssetDatabase.h>
#include <Engine\Assets\DataAsset\MaterialAsset.h>
#include <Engine\Assets\Manager\AssetManager.h>
#include <externals\imgui\imgui.h>

#include <algorithm>

namespace CalyxEngine {
	MaterialNodeEditorPanel::MaterialNodeEditorPanel()
		: IEngineUI("Material Graph"), canvas_("MaterialGraphCanvas") {}

	void MaterialNodeEditorPanel::Render() {
		if(!IsShow()) return;
		bool open = true;
		if(ImGui::Begin(panelName_.c_str(), &open)) {
			ImGui::Columns(2, nullptr, true);
			ImGui::SetColumnWidth(0, 220.0f);
			DrawMaterialList();
			ImGui::NextColumn();

			auto material = AssetManager::GetInstance()->GetDataAssetManager()->GetAsset<MaterialAsset>(selectedMaterial_);
			if(material) {
				EnsureOutputNode(*material);
				DrawToolbar(*material);
				if(canvas_.Draw(material->graph, [this](Node& node) { return DrawNodeBody(node); })) {
					Evaluate(*material);
				}
			} else {
				ImGui::TextUnformatted("Select a material.");
			}
			ImGui::Columns(1);
		}
		ImGui::End();
		if(!open) SetShow(false);
	}

	void MaterialNodeEditorPanel::DrawMaterialList() {
		ImGui::TextUnformatted("Materials");
		ImGui::Separator();
		for(auto* rec : AssetDatabase::GetInstance()->GetView()) {
			if(!rec || rec->type != AssetType::Material) continue;
			bool selected = rec->guid == selectedMaterial_;
			if(ImGui::Selectable(rec->sourcePath.filename().string().c_str(), selected)) {
				selectedMaterial_ = rec->guid;
			}
		}
	}

	void MaterialNodeEditorPanel::DrawToolbar(MaterialAsset& material) {
		if(ImGui::Button("Color")) AddColorNode(material);
		ImGui::SameLine();
		if(ImGui::Button("Shininess")) AddFloatNode(material, "Shininess", "Shininess");
		ImGui::SameLine();
		if(ImGui::Button("Roughness")) AddFloatNode(material, "Roughness", "Roughness");
		ImGui::SameLine();
		if(ImGui::Button("Reflect")) AddBoolNode(material, "Reflect", "Reflect");
		ImGui::SameLine();
		if(ImGui::Button("Save")) Save(material);
		ImGui::Separator();
	}

	bool MaterialNodeEditorPanel::DrawNodeBody(Node& node) {
		bool changed = false;
		ImGui::PushID(node.id);
		if(node.type == "Color") {
			changed |= ImGui::ColorEdit4("Color", &node.colorValue.x);
		} else if(node.type == "Shininess" || node.type == "Roughness") {
			changed |= ImGui::DragFloat("Value", &node.floatValue, 0.01f, 0.0f, 256.0f);
		} else if(node.type == "Reflect") {
			changed |= ImGui::Checkbox("Value", &node.boolValue);
		} else if(node.type == "Output") {
			ImGui::TextUnformatted("Material Output");
		}
		ImGui::PopID();
		return changed;
	}

	void MaterialNodeEditorPanel::AddColorNode(MaterialAsset& material) {
		Node node;
		node.id = material.graph.AllocateId();
		node.type = "Color";
		node.title = "Color";
		node.position = {40.0f, 80.0f};
		node.colorValue = material.color;
		node.outputs.push_back({material.graph.AllocateId(), "Color", NodePinKind::Output, NodeValueType::Color});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::AddFloatNode(MaterialAsset& material, const char* type, const char* title) {
		Node node;
		node.id = material.graph.AllocateId();
		node.type = type;
		node.title = title;
		node.position = {40.0f, 220.0f};
		node.floatValue = node.type == "Roughness" ? material.roughness : material.shininess;
		node.outputs.push_back({material.graph.AllocateId(), "Value", NodePinKind::Output, NodeValueType::Float});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::AddBoolNode(MaterialAsset& material, const char* type, const char* title) {
		Node node;
		node.id = material.graph.AllocateId();
		node.type = type;
		node.title = title;
		node.position = {40.0f, 360.0f};
		node.boolValue = material.isReflect;
		node.outputs.push_back({material.graph.AllocateId(), "Value", NodePinKind::Output, NodeValueType::Bool});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::EnsureOutputNode(MaterialAsset& material) {
		for(const auto& node : material.graph.nodes) if(node.type == "Output") return;
		Node node;
		node.id = material.graph.AllocateId();
		node.type = "Output";
		node.title = "Output";
		node.position = {420.0f, 120.0f};
		node.inputs.push_back({material.graph.AllocateId(), "BaseColor", NodePinKind::Input, NodeValueType::Color});
		node.inputs.push_back({material.graph.AllocateId(), "Shininess", NodePinKind::Input, NodeValueType::Float});
		node.inputs.push_back({material.graph.AllocateId(), "Roughness", NodePinKind::Input, NodeValueType::Float});
		node.inputs.push_back({material.graph.AllocateId(), "Reflect", NodePinKind::Input, NodeValueType::Bool});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::Evaluate(MaterialAsset& material) {
		for(const auto& link : material.graph.links) {
			const Node* fromNode = nullptr;
			const Node* toNode = nullptr;
			const NodePin* fromPin = material.graph.FindPin(link.fromPinId, &fromNode);
			const NodePin* toPin = material.graph.FindPin(link.toPinId, &toNode);
			if(!fromPin || !toPin || !fromNode || !toNode || toNode->type != "Output") continue;
			if(toPin->name == "BaseColor" && fromPin->valueType == NodeValueType::Color) material.color = fromNode->colorValue;
			if(toPin->name == "Shininess" && fromPin->valueType == NodeValueType::Float) material.shininess = fromNode->floatValue;
			if(toPin->name == "Roughness" && fromPin->valueType == NodeValueType::Float) material.roughness = fromNode->floatValue;
			if(toPin->name == "Reflect" && fromPin->valueType == NodeValueType::Bool) material.isReflect = fromNode->boolValue;
		}
	}

	void MaterialNodeEditorPanel::Save(MaterialAsset& material) {
		Evaluate(material);
		for(auto* rec : AssetDatabase::GetInstance()->GetView()) {
			if(rec && rec->type == AssetType::Material && rec->guid == material.GetGuid()) {
				AssetManager::GetInstance()->GetDataAssetManager()->SaveAsset(material, rec->sourcePath);
				return;
			}
		}
	}
}

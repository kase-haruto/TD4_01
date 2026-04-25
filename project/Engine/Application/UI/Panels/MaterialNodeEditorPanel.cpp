#include "MaterialNodeEditorPanel.h"

#include <Engine\Assets\Database\AssetDatabase.h>
#include <Engine\Assets\DataAsset\MaterialAsset.h>
#include <Engine\Assets\Manager\AssetManager.h>
#include <externals\imgui\imgui.h>

#include <algorithm>
#include <filesystem>
#include <fstream>

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

			if(!selectedMaterial_.isValid()) {
				for(auto* rec : AssetDatabase::GetInstance()->GetView()) {
					if(rec && rec->type == AssetType::Material) {
						selectedMaterial_ = rec->guid;
						break;
					}
				}
			}

			auto material = AssetManager::GetInstance()->GetDataAssetManager()->GetAsset<MaterialAsset>(selectedMaterial_);
			if(material) {
				EnsureOutputNode(*material);
				DrawToolbar(*material);
				if(canvas_.Draw(
					   material->graph,
					   [this](Node& node) { return DrawNodeBody(node); },
					   [this, material](const NodeEditorCanvas::ContextMenu& menu) { return DrawContextMenu(*material, menu); })) {
					Evaluate(*material);
				}
			} else {
				ImGui::TextUnformatted("No material selected.");
				if(ImGui::Button("Create Material")) {
					CreateMaterialAsset();
				}
			}
			ImGui::Columns(1);
		}
		ImGui::End();
		if(!open) SetShow(false);
	}

	void MaterialNodeEditorPanel::DrawMaterialList() {
		ImGui::TextUnformatted("Materials");
		if(ImGui::Button("Create Material")) {
			CreateMaterialAsset();
		}
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
		if(ImGui::BeginMenu("Add Node")) {
			Vector2 pos{40.0f, 80.0f};
			if(ImGui::MenuItem("Color")) AddColorNode(material, pos);
			if(ImGui::MenuItem("Shininess")) AddFloatNode(material, "Shininess", "Shininess", pos);
			if(ImGui::MenuItem("Roughness")) AddFloatNode(material, "Roughness", "Roughness", pos);
			if(ImGui::MenuItem("Reflect")) AddBoolNode(material, "Reflect", "Reflect", pos);
			ImGui::Separator();
			if(ImGui::MenuItem("Multiply Color")) AddBinaryNode(material, "MultiplyColor", "Multiply Color", NodeValueType::Color, pos);
			if(ImGui::MenuItem("Multiply Float")) AddBinaryNode(material, "MultiplyFloat", "Multiply Float", NodeValueType::Float, pos);
			ImGui::EndMenu();
		}
		ImGui::SameLine();
		if(ImGui::Button("Save")) Save(material);
		ImGui::Separator();
	}

	bool MaterialNodeEditorPanel::DrawContextMenu(MaterialAsset& material, const NodeEditorCanvas::ContextMenu& menu) {
		bool changed = false;
		if(menu.type == NodeEditorCanvas::ContextMenuType::Background) {
			if(ImGui::MenuItem("Color")) { AddColorNode(material, menu.canvasPosition); changed = true; }
			if(ImGui::MenuItem("Shininess")) { AddFloatNode(material, "Shininess", "Shininess", menu.canvasPosition); changed = true; }
			if(ImGui::MenuItem("Roughness")) { AddFloatNode(material, "Roughness", "Roughness", menu.canvasPosition); changed = true; }
			if(ImGui::MenuItem("Reflect")) { AddBoolNode(material, "Reflect", "Reflect", menu.canvasPosition); changed = true; }
			ImGui::Separator();
			if(ImGui::MenuItem("Multiply Color")) { AddBinaryNode(material, "MultiplyColor", "Multiply Color", NodeValueType::Color, menu.canvasPosition); changed = true; }
			if(ImGui::MenuItem("Multiply Float")) { AddBinaryNode(material, "MultiplyFloat", "Multiply Float", NodeValueType::Float, menu.canvasPosition); changed = true; }
		} else {
			ImGui::TextDisabled("Delete: select node and press Delete");
		}
		return changed;
	}

	void MaterialNodeEditorPanel::CreateMaterialAsset() {
		auto* db = AssetDatabase::GetInstance();
		std::filesystem::path folder = db->GetRoot() / "Materials";
		std::filesystem::create_directories(folder);

		std::filesystem::path path = folder / "New Material.mat";
		for(int i = 1; std::filesystem::exists(path); ++i) {
			path = folder / ("New Material " + std::to_string(i) + ".mat");
		}

		{
			std::ofstream ofs(path);
			if(!ofs) return;
		}

		const Guid guid = db->RegisterOrUpdate(path, AssetType::Material);
		if(auto* dataAssets = AssetManager::GetInstance()->GetDataAssetManager()) {
			auto asset = dataAssets->GetAsset<MaterialAsset>(guid);
			if(asset) {
				asset->SetName(path.stem().string());
				dataAssets->SaveAsset(*asset, path);
			}
		}
		db->Scan();
		selectedMaterial_ = guid;
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
		} else if(node.type == "MultiplyColor" || node.type == "MultiplyFloat") {
			ImGui::TextDisabled("A * B");
		} else if(node.type == "Output") {
			ImGui::TextUnformatted("Material Output");
		}
		ImGui::PopID();
		return changed;
	}

	void MaterialNodeEditorPanel::AddColorNode(MaterialAsset& material, Vector2 position) {
		Node node;
		node.id = material.graph.AllocateId();
		node.type = "Color";
		node.title = "Color";
		node.position = position;
		node.colorValue = material.color;
		node.outputs.push_back({material.graph.AllocateId(), "Color", NodePinKind::Output, NodeValueType::Color});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::AddFloatNode(MaterialAsset& material, const char* type, const char* title, Vector2 position) {
		Node node;
		node.id = material.graph.AllocateId();
		node.type = type;
		node.title = title;
		node.position = position;
		node.floatValue = node.type == "Roughness" ? material.roughness : material.shininess;
		node.outputs.push_back({material.graph.AllocateId(), "Value", NodePinKind::Output, NodeValueType::Float});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::AddBoolNode(MaterialAsset& material, const char* type, const char* title, Vector2 position) {
		Node node;
		node.id = material.graph.AllocateId();
		node.type = type;
		node.title = title;
		node.position = position;
		node.boolValue = material.isReflect;
		node.outputs.push_back({material.graph.AllocateId(), "Value", NodePinKind::Output, NodeValueType::Bool});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::AddBinaryNode(MaterialAsset& material, const char* type, const char* title, NodeValueType valueType, Vector2 position) {
		Node node;
		node.id = material.graph.AllocateId();
		node.type = type;
		node.title = title;
		node.position = position;
		node.inputs.push_back({material.graph.AllocateId(), "A", NodePinKind::Input, valueType});
		node.inputs.push_back({material.graph.AllocateId(), "B", NodePinKind::Input, valueType});
		node.outputs.push_back({material.graph.AllocateId(), "Result", NodePinKind::Output, valueType});
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

	Vector4 MaterialNodeEditorPanel::EvaluateColor(const MaterialAsset& material, int32_t inputPinId, const Vector4& fallback) const {
		for(const auto& link : material.graph.links) {
			if(link.toPinId != inputPinId) continue;
			const Node* fromNode = nullptr;
			const NodePin* fromPin = material.graph.FindPin(link.fromPinId, &fromNode);
			if(!fromNode || !fromPin || fromPin->valueType != NodeValueType::Color) return fallback;
			if(fromNode->type == "Color") return fromNode->colorValue;
			if(fromNode->type == "MultiplyColor") {
				return EvaluateColor(material, fromNode->inputs[0].id, {1, 1, 1, 1}) *
					   EvaluateColor(material, fromNode->inputs[1].id, {1, 1, 1, 1});
			}
		}
		return fallback;
	}

	float MaterialNodeEditorPanel::EvaluateFloat(const MaterialAsset& material, int32_t inputPinId, float fallback) const {
		for(const auto& link : material.graph.links) {
			if(link.toPinId != inputPinId) continue;
			const Node* fromNode = nullptr;
			const NodePin* fromPin = material.graph.FindPin(link.fromPinId, &fromNode);
			if(!fromNode || !fromPin || fromPin->valueType != NodeValueType::Float) return fallback;
			if(fromNode->type == "Shininess" || fromNode->type == "Roughness") return fromNode->floatValue;
			if(fromNode->type == "MultiplyFloat") {
				return EvaluateFloat(material, fromNode->inputs[0].id, 1.0f) *
					   EvaluateFloat(material, fromNode->inputs[1].id, 1.0f);
			}
		}
		return fallback;
	}

	bool MaterialNodeEditorPanel::EvaluateBool(const MaterialAsset& material, int32_t inputPinId, bool fallback) const {
		for(const auto& link : material.graph.links) {
			if(link.toPinId != inputPinId) continue;
			const Node* fromNode = nullptr;
			const NodePin* fromPin = material.graph.FindPin(link.fromPinId, &fromNode);
			if(!fromNode || !fromPin || fromPin->valueType != NodeValueType::Bool) return fallback;
			if(fromNode->type == "Reflect") return fromNode->boolValue;
		}
		return fallback;
	}

	void MaterialNodeEditorPanel::Evaluate(MaterialAsset& material) {
		for(const auto& node : material.graph.nodes) {
			if(node.type != "Output") continue;
			for(const auto& pin : node.inputs) {
				if(pin.name == "BaseColor") material.color = EvaluateColor(material, pin.id, material.color);
				if(pin.name == "Shininess") material.shininess = EvaluateFloat(material, pin.id, material.shininess);
				if(pin.name == "Roughness") material.roughness = EvaluateFloat(material, pin.id, material.roughness);
				if(pin.name == "Reflect") material.isReflect = EvaluateBool(material, pin.id, material.isReflect);
			}
			return;
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

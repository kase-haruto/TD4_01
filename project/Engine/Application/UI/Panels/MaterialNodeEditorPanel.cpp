#include "MaterialNodeEditorPanel.h"

#include <Engine\Assets\DataAsset\MaterialAsset.h>
#include <Engine\Assets\Database\AssetDatabase.h>
#include <Engine\Assets\Manager\AssetManager.h>
#include <externals\imgui\imgui.h>

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace CalyxEngine {
	namespace {
		constexpr const char* kLightingModes[] = {
			"Half-Lambert",
			"Lambert",
			"Toon",
			"No Lighting",
			"Unlit Color"};
		constexpr int32_t kLightingModeCount = static_cast<int32_t>(std::size(kLightingModes));
	} // namespace

	MaterialNodeEditorPanel::MaterialNodeEditorPanel()
		: IEngineUI("Material Graph"), canvas_("MaterialGraphCanvas") {
		isShow_ = false;
	}

	void MaterialNodeEditorPanel::Render() {
		if(!IsShow()) return;
		bool open = true;
		if(ImGui::Begin(panelName_.c_str(), &open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
			if(!selectedMaterial_.isValid()) {
				for(auto* rec : AssetDatabase::GetInstance()->GetView()) {
					if(rec && rec->type == AssetType::Material) {
						selectedMaterial_ = rec->guid;
						break;
					}
				}
			}

			const float sidebarWidth = 260.0f;
			ImGui::BeginChild("##material-sidebar", ImVec2(sidebarWidth, 0.0f), true);
			DrawMaterialList();
			ImGui::EndChild();
			ImGui::SameLine();

			ImGui::BeginChild("##material-workspace", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
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
				if(DrawLightingModePopup(*material)) {
					Evaluate(*material);
				}
			} else {
				ImGui::Dummy(ImVec2(0.0f, 36.0f));
				ImGui::TextDisabled("No material selected.");
				if(ImGui::Button("Create Material", ImVec2(150.0f, 28.0f))) {
					CreateMaterialAsset();
				}
			}
			ImGui::EndChild();
		}
		ImGui::End();
		if(!open) SetShow(false);
	}

	void MaterialNodeEditorPanel::DrawMaterialList() {
		ImGui::TextUnformatted("Materials");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 78.0f);
		if(ImGui::Button("New", ImVec2(58.0f, 24.0f))) {
			CreateMaterialAsset();
		}
		ImGui::Separator();

		ImGui::BeginChild("##material-list", ImVec2(0.0f, -32.0f), false);
		for(auto* rec : AssetDatabase::GetInstance()->GetView()) {
			if(!rec || rec->type != AssetType::Material) continue;
			bool selected = rec->guid == selectedMaterial_;
			ImGui::PushID(rec->guid.ToString().c_str());
			const std::string label = rec->sourcePath.stem().string();
			if(ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0.0f, 28.0f))) {
				selectedMaterial_ = rec->guid;
				if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					BeginRenameMaterial(rec->guid, rec->sourcePath);
				}
			}
			if(ImGui::BeginPopupContextItem("MaterialContext")) {
				if(ImGui::MenuItem("Rename")) BeginRenameMaterial(rec->guid, rec->sourcePath);
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
		ImGui::EndChild();

		if(renamePopupRequested_) {
			ImGui::OpenPopup("Rename Material");
			renamePopupRequested_ = false;
		}
		if(ImGui::BeginPopupModal("Rename Material", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			if(focusRename_) {
				ImGui::SetKeyboardFocusHere();
				focusRename_ = false;
			}
			ImGui::SetNextItemWidth(260.0f);
			const bool submitted = ImGui::InputText("Name", renameBuffer_.data(), renameBuffer_.size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
			if(submitted) CommitRenameMaterial();
			if(ImGui::Button("Rename", ImVec2(96.0f, 0.0f))) CommitRenameMaterial();
			ImGui::SameLine();
			if(ImGui::Button("Cancel", ImVec2(96.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
				CancelRenameMaterial();
			}
			ImGui::EndPopup();
		}
	}

	void MaterialNodeEditorPanel::DrawToolbar(MaterialAsset& material) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
		ImGui::TextUnformatted(material.GetName().c_str());
		ImGui::SameLine();
		ImGui::TextDisabled("GUID: %s", material.GetGuid().ToString().substr(0, 8).c_str());
		ImGui::SameLine();
		if(ImGui::Button("+ Add Node", ImVec2(112.0f, 0.0f))) ImGui::OpenPopup("AddNodeToolbar");
		if(ImGui::BeginPopup("AddNodeToolbar")) {
			DrawAddNodeMenu(material, {60.0f, 100.0f});
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		if(ImGui::Button("Save", ImVec2(76.0f, 0.0f))) Save(material);
		ImGui::PopStyleVar();
		ImGui::Separator();
	}

	bool MaterialNodeEditorPanel::DrawAddNodeMenu(MaterialAsset& material, Vector2 position) {
		bool changed = false;
		if(ImGui::BeginMenu("Parameters")) {
			if(ImGui::MenuItem("Color")) {
				AddColorNode(material, position);
				changed = true;
			}
			if(ImGui::MenuItem("Shininess")) {
				AddFloatNode(material, "Shininess", "Shininess", position);
				changed = true;
			}
			if(ImGui::MenuItem("Roughness")) {
				AddFloatNode(material, "Roughness", "Roughness", position);
				changed = true;
			}
			if(ImGui::MenuItem("Reflect")) {
				AddBoolNode(material, "Reflect", "Reflect", position);
				changed = true;
			}
			if(ImGui::MenuItem("Lighting Mode")) {
				AddLightingModeNode(material, position);
				changed = true;
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Math")) {
			if(ImGui::MenuItem("Multiply Color")) {
				AddBinaryNode(material, "MultiplyColor", "Multiply Color", NodeValueType::Color, position);
				changed = true;
			}
			if(ImGui::MenuItem("Multiply Float")) {
				AddBinaryNode(material, "MultiplyFloat", "Multiply Float", NodeValueType::Float, position);
				changed = true;
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();
		if(ImGui::MenuItem("Color##quick")) {
			AddColorNode(material, position);
			changed = true;
		}
		if(ImGui::MenuItem("Lighting Mode##quick")) {
			AddLightingModeNode(material, position);
			changed = true;
		}
		return changed;
	}

	bool MaterialNodeEditorPanel::DrawContextMenu(MaterialAsset& material, const NodeEditorCanvas::ContextMenu& menu) {
		bool changed = false;
		if(menu.type == NodeEditorCanvas::ContextMenuType::Background) {
			changed |= DrawAddNodeMenu(material, menu.canvasPosition);
		} else {
			ImGui::TextDisabled("Delete: select node and press Delete");
		}
		return changed;
	}

	bool MaterialNodeEditorPanel::DrawLightingModePopup(MaterialAsset& material) {
		bool changed = false;
		if(lightingModePopupRequested_) {
			ImGui::OpenPopup("LightingModeSelect");
			lightingModePopupRequested_ = false;
		}

		ImGui::SetNextWindowPos(ImVec2(lightingModePopupPos_.x, lightingModePopupPos_.y), ImGuiCond_Appearing);
		if(ImGuiViewport* viewport = ImGui::GetWindowViewport()) {
			ImGui::SetNextWindowViewport(viewport->ID);
		}
		ImGui::SetNextWindowSizeConstraints(ImVec2(180.0f, 0.0f), ImVec2(240.0f, 260.0f));
		if(ImGui::BeginPopup("LightingModeSelect")) {
			Node* target = nullptr;
			for(auto& node : material.graph.nodes) {
				if(node.id == lightingModePopupNodeId_) {
					target = &node;
					break;
				}
			}

			if(target && target->type == "LightingMode") {
				for(int32_t i = 0; i < kLightingModeCount; ++i) {
					const bool selected = target->intValue == i;
					if(ImGui::Selectable(kLightingModes[i], selected)) {
						target->intValue	  = i;
						material.lightingMode = i;
						changed				  = true;
						ImGui::CloseCurrentPopup();
					}
					if(selected) ImGui::SetItemDefaultFocus();
				}
			} else {
				ImGui::TextDisabled("No lighting mode node.");
			}

			if(ImGui::IsKeyPressed(ImGuiKey_Escape)) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		return changed;
	}

	void MaterialNodeEditorPanel::CreateMaterialAsset() {
		auto*				  db	 = AssetDatabase::GetInstance();
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

	void MaterialNodeEditorPanel::BeginRenameMaterial(const Guid& guid, const std::filesystem::path& path) {
		renamingMaterial_	   = guid;
		renamingPath_		   = path;
		const std::string stem = path.stem().string();
		std::fill(renameBuffer_.begin(), renameBuffer_.end(), '\0');
		std::copy_n(stem.c_str(), std::min(stem.size(), renameBuffer_.size() - 1), renameBuffer_.data());
		focusRename_		  = true;
		renamePopupRequested_ = true;
	}

	void MaterialNodeEditorPanel::CommitRenameMaterial() {
		std::string newName = renameBuffer_.data();
		auto		first	= newName.find_first_not_of(" \t\r\n");
		auto		last	= newName.find_last_not_of(" \t\r\n");
		if(first == std::string::npos) return;
		newName = newName.substr(first, last - first + 1);
		if(newName.empty() || renamingPath_.empty()) return;

		std::filesystem::path newPath = renamingPath_.parent_path() / (newName + renamingPath_.extension().string());
		if(newPath != renamingPath_ && std::filesystem::exists(newPath)) return;

		std::error_code ec;
		if(newPath != renamingPath_) {
			std::filesystem::rename(renamingPath_, newPath, ec);
			if(ec) return;
			std::filesystem::path oldMeta = renamingPath_;
			oldMeta += ".meta";
			std::filesystem::path newMeta = newPath;
			newMeta += ".meta";
			if(std::filesystem::exists(oldMeta)) {
				std::filesystem::rename(oldMeta, newMeta, ec);
			}
		}

		if(auto* dataAssets = AssetManager::GetInstance()->GetDataAssetManager()) {
			if(auto asset = dataAssets->GetAsset<MaterialAsset>(renamingMaterial_)) {
				asset->SetName(newName);
				dataAssets->SaveAsset(*asset, newPath);
			}
		}
		AssetDatabase::GetInstance()->RegisterOrUpdate(newPath, AssetType::Material);
		AssetDatabase::GetInstance()->Scan();
		selectedMaterial_ = renamingMaterial_;
		CancelRenameMaterial();
	}

	void MaterialNodeEditorPanel::CancelRenameMaterial() {
		renamingMaterial_ = Guid::Empty();
		renamingPath_.clear();
		focusRename_		  = false;
		renamePopupRequested_ = false;
		ImGui::CloseCurrentPopup();
	}

	bool MaterialNodeEditorPanel::DrawNodeBody(Node& node) {
		bool changed = false;
		ImGui::PushID(node.id);
		if(node.type == "Color") {
			ImGui::SetNextItemWidth(170.0f);
			changed |= ImGui::ColorEdit4("Color", &node.colorValue.x);
		} else if(node.type == "Shininess" || node.type == "Roughness") {
			ImGui::SetNextItemWidth(160.0f);
			changed |= ImGui::DragFloat("Value", &node.floatValue, 0.01f, 0.0f, 256.0f);
		} else if(node.type == "Reflect") {
			changed |= ImGui::Checkbox("Value", &node.boolValue);
		} else if(node.type == "LightingMode") {
			node.intValue		= std::clamp(node.intValue, 0, kLightingModeCount - 1);
			const char* current = kLightingModes[node.intValue];
			ImGui::SetNextItemWidth(146.0f);
			if(ImGui::Button(current, ImVec2(146.0f, 0.0f))) {
				lightingModePopupRequested_ = true;
				lightingModePopupNodeId_	= node.id;
				const ImVec2 itemMin		= ImGui::GetItemRectMin();
				const ImVec2 itemMax		= ImGui::GetItemRectMax();
				ImVec2		 popupPos(itemMax.x + 8.0f, itemMin.y);
				if(ImGuiViewport* viewport = ImGui::GetWindowViewport()) {
					const ImVec2 workMin		 = viewport->WorkPos;
					const ImVec2 workMax		 = ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y);
					const float	 estimatedWidth	 = 220.0f;
					const float	 estimatedHeight = 180.0f;
					if(popupPos.x + estimatedWidth > workMax.x) {
						popupPos.x = itemMin.x - estimatedWidth - 8.0f;
					}
					if(popupPos.x < workMin.x) {
						popupPos.x = workMin.x;
					}
					if(popupPos.y + estimatedHeight > workMax.y) {
						popupPos.y = workMax.y - estimatedHeight;
						if(popupPos.y < workMin.y) {
							popupPos.y = workMin.y;
						}
					}
				}
				lightingModePopupPos_.x = popupPos.x;
				lightingModePopupPos_.y = popupPos.y;
			}
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
		node.id			= material.graph.AllocateId();
		node.type		= "Color";
		node.title		= "Color";
		node.position	= position;
		node.colorValue = material.color;
		node.outputs.push_back({material.graph.AllocateId(), "Color", NodePinKind::Output, NodeValueType::Color});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::AddFloatNode(MaterialAsset& material, const char* type, const char* title, Vector2 position) {
		Node node;
		node.id			= material.graph.AllocateId();
		node.type		= type;
		node.title		= title;
		node.position	= position;
		node.floatValue = node.type == "Roughness" ? material.roughness : material.shininess;
		node.outputs.push_back({material.graph.AllocateId(), "Value", NodePinKind::Output, NodeValueType::Float});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::AddBoolNode(MaterialAsset& material, const char* type, const char* title, Vector2 position) {
		Node node;
		node.id		   = material.graph.AllocateId();
		node.type	   = type;
		node.title	   = title;
		node.position  = position;
		node.boolValue = material.isReflect;
		node.outputs.push_back({material.graph.AllocateId(), "Value", NodePinKind::Output, NodeValueType::Bool});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::AddLightingModeNode(MaterialAsset& material, Vector2 position) {
		Node node;
		node.id		  = material.graph.AllocateId();
		node.type	  = "LightingMode";
		node.title	  = "Lighting Mode";
		node.position = position;
		node.intValue = std::clamp(material.lightingMode, 0, kLightingModeCount - 1);
		node.inputs.push_back({material.graph.AllocateId(), "Value", NodePinKind::Input, NodeValueType::Int});
		node.outputs.push_back({material.graph.AllocateId(), "Mode", NodePinKind::Output, NodeValueType::Int});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::AddBinaryNode(MaterialAsset& material, const char* type, const char* title, NodeValueType valueType, Vector2 position) {
		Node node;
		node.id		  = material.graph.AllocateId();
		node.type	  = type;
		node.title	  = title;
		node.position = position;
		node.inputs.push_back({material.graph.AllocateId(), "A", NodePinKind::Input, valueType});
		node.inputs.push_back({material.graph.AllocateId(), "B", NodePinKind::Input, valueType});
		node.outputs.push_back({material.graph.AllocateId(), "Result", NodePinKind::Output, valueType});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::EnsureOutputNode(MaterialAsset& material) {
		for(auto& node : material.graph.nodes) {
			if(node.type != "Output") continue;
			const auto hasLightingMode = std::any_of(node.inputs.begin(), node.inputs.end(), [](const NodePin& pin) {
				return pin.name == "Lighting Mode";
			});
			if(!hasLightingMode) {
				node.inputs.push_back({material.graph.AllocateId(), "Lighting Mode", NodePinKind::Input, NodeValueType::Int});
			}
			return;
		}
		Node node;
		node.id		  = material.graph.AllocateId();
		node.type	  = "Output";
		node.title	  = "Output";
		node.position = {420.0f, 120.0f};
		node.inputs.push_back({material.graph.AllocateId(), "BaseColor", NodePinKind::Input, NodeValueType::Color});
		node.inputs.push_back({material.graph.AllocateId(), "Shininess", NodePinKind::Input, NodeValueType::Float});
		node.inputs.push_back({material.graph.AllocateId(), "Roughness", NodePinKind::Input, NodeValueType::Float});
		node.inputs.push_back({material.graph.AllocateId(), "Reflect", NodePinKind::Input, NodeValueType::Bool});
		node.inputs.push_back({material.graph.AllocateId(), "Lighting Mode", NodePinKind::Input, NodeValueType::Int});
		material.graph.nodes.push_back(std::move(node));
	}

	Vector4 MaterialNodeEditorPanel::EvaluateColor(const MaterialAsset& material, int32_t inputPinId, const Vector4& fallback) const {
		for(const auto& link : material.graph.links) {
			if(link.toPinId != inputPinId) continue;
			const Node*	   fromNode = nullptr;
			const NodePin* fromPin	= material.graph.FindPin(link.fromPinId, &fromNode);
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
			const Node*	   fromNode = nullptr;
			const NodePin* fromPin	= material.graph.FindPin(link.fromPinId, &fromNode);
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
			const Node*	   fromNode = nullptr;
			const NodePin* fromPin	= material.graph.FindPin(link.fromPinId, &fromNode);
			if(!fromNode || !fromPin || fromPin->valueType != NodeValueType::Bool) return fallback;
			if(fromNode->type == "Reflect") return fromNode->boolValue;
		}
		return fallback;
	}

	int32_t MaterialNodeEditorPanel::EvaluateInt(const MaterialAsset& material, int32_t inputPinId, int32_t fallback) const {
		for(const auto& link : material.graph.links) {
			if(link.toPinId != inputPinId) continue;
			const Node*	   fromNode = nullptr;
			const NodePin* fromPin	= material.graph.FindPin(link.fromPinId, &fromNode);
			if(!fromNode || !fromPin || fromPin->valueType != NodeValueType::Int) return fallback;
			if(fromNode->type == "LightingMode") {
				const int32_t base = std::clamp(fromNode->intValue, 0, kLightingModeCount - 1);
				if(!fromNode->inputs.empty()) {
					return EvaluateInt(material, fromNode->inputs[0].id, base);
				}
				return base;
			}
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
				if(pin.name == "Lighting Mode") material.lightingMode = EvaluateInt(material, pin.id, material.lightingMode);
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
} // namespace CalyxEngine

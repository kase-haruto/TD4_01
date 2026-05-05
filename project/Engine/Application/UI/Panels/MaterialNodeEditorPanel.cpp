#include "MaterialNodeEditorPanel.h"

#include <Engine\Assets\DataAsset\MaterialAsset.h>
#include <Engine\Assets\Database\AssetDatabase.h>
#include <Engine\Assets\Manager\AssetManager.h>
#include <Engine\Graphics\MaterialGraph\MaterialGraphCompiler.h>
#include <externals\imgui\imgui.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

namespace CalyxEngine {
	namespace {
		constexpr const char* kLightingModes[] = {
			"Half-Lambert",
			"Lambert",
			"Toon",
			"No Lighting",
			"Unlit Color"};
		constexpr int32_t kLightingModeCount = static_cast<int32_t>(std::size(kLightingModes));

		bool IsObsoleteToonOutputPin(const std::string& name) {
			return name == "Toon Highlight" ||
				   name == "Toon Base" ||
				   name == "Toon Mid Shadow" ||
				   name == "Toon Shadow" ||
				   name == "Toon Threshold 1" ||
				   name == "Toon Threshold 2" ||
				   name == "Toon Threshold 3" ||
				   name == "Toon Edge Softness" ||
				   name == "Toon Spec Threshold" ||
				   name == "Toon Spec Softness" ||
				   name == "Toon Spec Intensity";
		}

		bool IsLegacyOutputPin(const std::string& name) {
			return name == "BaseColor" ||
				   name == "Shininess" ||
				   name == "Roughness" ||
				   name == "Reflect" ||
				   name == "Lighting Mode" ||
				   IsObsoleteToonOutputPin(name);
		}

		bool IsLightingNode(const std::string& type) {
			return type == "HalfLambertLighting" ||
				   type == "LambertLighting" ||
				   type == "ToonLighting" ||
				   type == "NoLighting" ||
				   type == "UnlitColorLighting";
		}

		int32_t LightingModeFromNodeType(const std::string& type, int32_t fallback) {
			if(type == "HalfLambertLighting") return 0;
			if(type == "LambertLighting") return 1;
			if(type == "ToonLighting") return 2;
			if(type == "NoLighting") return 3;
			if(type == "UnlitColorLighting") return 4;
			return fallback;
		}

		const NodePin* FindInputPin(const Node& node, const char* name) {
			for(const auto& pin : node.inputs) {
				if(pin.name == name) return &pin;
			}
			return nullptr;
		}

		bool IsInputLinked(const MaterialAsset& material, const Node& node, const char* name) {
			const NodePin* pin = FindInputPin(node, name);
			if(!pin) return false;
			return std::any_of(material.graph.links.begin(), material.graph.links.end(), [pin](const NodeLink& link) {
				return link.toPinId == pin->id;
			});
		}

		float GetFloatProperty(const Node& node, const char* key, float fallback) {
			if(!node.properties.contains(key)) return fallback;
			return node.properties.value(key, fallback);
		}

		void SetFloatProperty(Node& node, const char* key, float value) {
			node.properties[key] = value;
		}

		Vector4 GetColorProperty(const Node& node, const char* key, const Vector4& fallback) {
			auto it = node.properties.find(key);
			if(it == node.properties.end() || !it->is_array() || it->size() != 4) return fallback;
			return {it->at(0).get<float>(), it->at(1).get<float>(), it->at(2).get<float>(), it->at(3).get<float>()};
		}

		void SetColorProperty(Node& node, const char* key, const Vector4& value) {
			node.properties[key] = {value.x, value.y, value.z, value.w};
		}

		void SetDefaultToonProperties(Node& node) {
			SetColorProperty(node, "toonHighlightColor", {1.15f, 1.10f, 1.00f, 1.0f});
			SetColorProperty(node, "toonBaseColor", {1.0f, 1.0f, 1.0f, 1.0f});
			SetColorProperty(node, "toonMidShadowColor", {0.72f, 0.76f, 0.86f, 1.0f});
			SetColorProperty(node, "toonShadowColor", {0.42f, 0.46f, 0.58f, 1.0f});
			SetFloatProperty(node, "toonThreshold1", -0.15f);
			SetFloatProperty(node, "toonThreshold2", 0.25f);
			SetFloatProperty(node, "toonThreshold3", 0.82f);
			SetFloatProperty(node, "toonEdgeSoftness", 0.03f);
			SetFloatProperty(node, "toonSpecularThreshold", 0.96f);
			SetFloatProperty(node, "toonSpecularSoftness", 0.02f);
			SetFloatProperty(node, "toonSpecularIntensity", 0.35f);
		}

		void SetDefaultToonMasterProperties(Node& node) {
			SetColorProperty(node, "baseColor", {1, 1, 1, 1});
			SetColorProperty(node, "highlightColor", {1.08f, 1.06f, 1.02f, 1.0f});
			SetColorProperty(node, "firstShadeColor", {0.72f, 0.76f, 0.86f, 1.0f});
			SetColorProperty(node, "secondShadeColor", {0.42f, 0.46f, 0.58f, 1.0f});
			SetFloatProperty(node, "baseStep", 0.25f);
			SetFloatProperty(node, "baseFeather", 0.03f);
			SetFloatProperty(node, "shadeStep", -0.15f);
			SetFloatProperty(node, "shadeFeather", 0.03f);
			SetFloatProperty(node, "specularThreshold", 0.96f);
			SetFloatProperty(node, "specularSoftness", 0.02f);
			SetFloatProperty(node, "specularIntensity", 0.35f);
		}
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
					   [this, material](Node& node) { return DrawNodeBody(*material, node); },
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
			Vector2 position = canvas_.GetLastViewCenter();
			position.x -= 110.0f;
			position.y -= 60.0f;
			DrawAddNodeMenu(material, position);
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		if(ImGui::Button("Save", ImVec2(76.0f, 0.0f))) Save(material);
		ImGui::PopStyleVar();
		ImGui::Separator();
	}

	bool MaterialNodeEditorPanel::DrawAddNodeMenu(MaterialAsset& material, Vector2 position) {
		bool changed = false;
		if(ImGui::BeginMenu("Material Parameters")) {
			if(ImGui::MenuItem("Color")) {
				AddColorNode(material, position);
				changed = true;
			}
			if(ImGui::MenuItem("Float")) {
				AddFloatNode(material, "Float", "Float", position);
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
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Lighting")) {
			if(ImGui::MenuItem("Half-Lambert")) {
				AddLightingNode(material, "HalfLambertLighting", "Half-Lambert", 0, position);
				changed = true;
			}
			if(ImGui::MenuItem("Lambert")) {
				AddLightingNode(material, "LambertLighting", "Lambert", 1, position);
				changed = true;
			}
			if(ImGui::MenuItem("Toon Lighting")) {
				AddLightingNode(material, "ToonLighting", "Toon Lighting", 2, position);
				changed = true;
			}
			if(ImGui::MenuItem("No Lighting")) {
				AddLightingNode(material, "NoLighting", "No Lighting", 3, position);
				changed = true;
			}
			if(ImGui::MenuItem("Unlit Color")) {
				AddLightingNode(material, "UnlitColorLighting", "Unlit Color", 4, position);
				changed = true;
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Master")) {
			if(ImGui::MenuItem("Toon Master")) {
				AddToonMasterNode(material, position);
				changed = true;
			}
			if(ImGui::MenuItem("Lit Master")) {
				AddLitMasterNode(material, position);
				changed = true;
			}
			if(ImGui::MenuItem("Unlit Master")) {
				AddUnlitMasterNode(material, position);
				changed = true;
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Operators")) {
			if(ImGui::MenuItem("Multiply Color")) {
				AddBinaryNode(material, "MultiplyColor", "Multiply Color", NodeValueType::Color, position);
				changed = true;
			}
			if(ImGui::MenuItem("Multiply Float")) {
				AddBinaryNode(material, "MultiplyFloat", "Multiply Float", NodeValueType::Float, position);
				changed = true;
			}
			if(ImGui::MenuItem("Lerp Color")) {
				AddLerpNode(material, "LerpColor", "Lerp Color", NodeValueType::Color, position);
				changed = true;
			}
			if(ImGui::MenuItem("Lerp Float")) {
				AddLerpNode(material, "LerpFloat", "Lerp Float", NodeValueType::Float, position);
				changed = true;
			}
			if(ImGui::MenuItem("Saturate Float")) {
				AddUnaryFloatNode(material, "SaturateFloat", "Saturate Float", position);
				changed = true;
			}
			if(ImGui::MenuItem("One Minus Float")) {
				AddUnaryFloatNode(material, "OneMinusFloat", "One Minus Float", position);
				changed = true;
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();
		if(ImGui::MenuItem("Color##quick")) {
			AddColorNode(material, position);
			changed = true;
		}
		if(ImGui::MenuItem("Toon Lighting##quick")) {
			AddToonMasterNode(material, position);
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

	bool MaterialNodeEditorPanel::DrawNodeBody(MaterialAsset& material, Node& node) {
		bool changed = false;
		ImGui::PushID(node.id);
		if(node.type == "Color") {
			ImGui::SetNextItemWidth(188.0f);
			changed |= ImGui::ColorEdit4("Color", &node.colorValue.x);
		} else if(node.type == "Float" || node.type == "Shininess" || node.type == "Roughness") {
			ImGui::SetNextItemWidth(178.0f);
			const float minValue = node.type == "Float" ? -1000.0f : 0.0f;
			const float maxValue = node.type == "Float" ? 1000.0f : 256.0f;
			changed |= ImGui::DragFloat("Value", &node.floatValue, 0.01f, minValue, maxValue);
		} else if(node.type == "Reflect") {
			changed |= ImGui::Checkbox("Value", &node.boolValue);
		} else if(node.type == "LightingMode") {
			node.intValue		= std::clamp(node.intValue, 0, kLightingModeCount - 1);
			const char* current = kLightingModes[node.intValue];
			ImGui::SetNextItemWidth(178.0f);
			if(ImGui::Button(current, ImVec2(178.0f, 0.0f))) {
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
		} else if(node.type == "ToonMaster") {
			if(ImGui::Button("Reset Toon Master", ImVec2(188.0f, 0.0f))) {
				SetDefaultToonMasterProperties(node);
				changed = true;
			}

			Vector4 base = GetColorProperty(node, "baseColor", {1, 1, 1, 1});
			Vector4 highlight = GetColorProperty(node, "highlightColor", {1.08f, 1.06f, 1.02f, 1.0f});
			Vector4 firstShade = GetColorProperty(node, "firstShadeColor", {0.72f, 0.76f, 0.86f, 1.0f});
			Vector4 secondShade = GetColorProperty(node, "secondShadeColor", {0.42f, 0.46f, 0.58f, 1.0f});
			float baseStep = GetFloatProperty(node, "baseStep", 0.25f);
			float baseFeather = GetFloatProperty(node, "baseFeather", 0.03f);
			float shadeStep = GetFloatProperty(node, "shadeStep", -0.15f);
			float shadeFeather = GetFloatProperty(node, "shadeFeather", 0.03f);
			float specThreshold = GetFloatProperty(node, "specularThreshold", 0.96f);
			float specSoftness = GetFloatProperty(node, "specularSoftness", 0.02f);
			float specIntensity = GetFloatProperty(node, "specularIntensity", 0.35f);

			auto drawColorFallback = [&](const char* pinName, const char* label, const char* property, Vector4& value) {
				const bool linked = IsInputLinked(material, node, pinName);
				ImGui::BeginDisabled(linked);
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::ColorEdit4(label, &value.x)) {
					SetColorProperty(node, property, value);
					changed = true;
				}
				ImGui::EndDisabled();
				if(linked) {
					ImGui::SameLine();
					ImGui::TextDisabled("Linked");
				}
			};
			auto drawFloatFallback = [&](const char* pinName, const char* label, const char* property, float& value, float minValue, float maxValue) {
				const bool linked = IsInputLinked(material, node, pinName);
				ImGui::BeginDisabled(linked);
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat(label, &value, minValue, maxValue)) {
					SetFloatProperty(node, property, value);
					changed = true;
				}
				ImGui::EndDisabled();
				if(linked) {
					ImGui::SameLine();
					ImGui::TextDisabled("Linked");
				}
			};

			drawColorFallback("Base Color", "Base", "baseColor", base);
			drawColorFallback("Highlight", "Highlight", "highlightColor", highlight);
			drawColorFallback("1st Shade", "1st Shade", "firstShadeColor", firstShade);
			drawColorFallback("2nd Shade", "2nd Shade", "secondShadeColor", secondShade);
			drawFloatFallback("Base Step", "Base Step", "baseStep", baseStep, -1.0f, 1.0f);
			drawFloatFallback("Base Feather", "Base Feather", "baseFeather", baseFeather, 0.0f, 0.25f);
			drawFloatFallback("Shade Step", "Shade Step", "shadeStep", shadeStep, -1.0f, 1.0f);
			drawFloatFallback("Shade Feather", "Shade Feather", "shadeFeather", shadeFeather, 0.0f, 0.25f);
			drawFloatFallback("Spec Threshold", "Spec Threshold", "specularThreshold", specThreshold, 0.0f, 1.0f);
			drawFloatFallback("Spec Softness", "Spec Softness", "specularSoftness", specSoftness, 0.0f, 0.25f);
			drawFloatFallback("Spec Intensity", "Spec Intensity", "specularIntensity", specIntensity, 0.0f, 4.0f);
		} else if(node.type == "LitMaster") {
			ImGui::TextDisabled("Standard Lit Surface");
		} else if(node.type == "UnlitMaster") {
			ImGui::TextDisabled("Unlit Surface");
		} else if(IsLightingNode(node.type)) {
			if(node.type == "ToonLighting") {
				if(ImGui::Button("Reset Toon Defaults", ImVec2(188.0f, 0.0f))) {
					SetDefaultToonProperties(node);
					changed = true;
				}

				Vector4 highlight = GetColorProperty(node, "toonHighlightColor", {1.15f, 1.10f, 1.00f, 1.0f});
				Vector4 base = GetColorProperty(node, "toonBaseColor", {1.0f, 1.0f, 1.0f, 1.0f});
				Vector4 midShadow = GetColorProperty(node, "toonMidShadowColor", {0.72f, 0.76f, 0.86f, 1.0f});
				Vector4 shadow = GetColorProperty(node, "toonShadowColor", {0.42f, 0.46f, 0.58f, 1.0f});
				float threshold1 = GetFloatProperty(node, "toonThreshold1", -0.15f);
				float threshold2 = GetFloatProperty(node, "toonThreshold2", 0.25f);
				float threshold3 = GetFloatProperty(node, "toonThreshold3", 0.82f);
				float edgeSoftness = GetFloatProperty(node, "toonEdgeSoftness", 0.03f);
				float specThreshold = GetFloatProperty(node, "toonSpecularThreshold", 0.96f);
				float specSoftness = GetFloatProperty(node, "toonSpecularSoftness", 0.02f);
				float specIntensity = GetFloatProperty(node, "toonSpecularIntensity", 0.35f);

				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::ColorEdit4("Highlight", &highlight.x)) {
					SetColorProperty(node, "toonHighlightColor", highlight);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::ColorEdit4("Base", &base.x)) {
					SetColorProperty(node, "toonBaseColor", base);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::ColorEdit4("Mid Shadow", &midShadow.x)) {
					SetColorProperty(node, "toonMidShadowColor", midShadow);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::ColorEdit4("Shadow", &shadow.x)) {
					SetColorProperty(node, "toonShadowColor", shadow);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat("Threshold 1", &threshold1, -1.0f, 1.0f)) {
					SetFloatProperty(node, "toonThreshold1", threshold1);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat("Threshold 2", &threshold2, -1.0f, 1.0f)) {
					SetFloatProperty(node, "toonThreshold2", threshold2);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat("Threshold 3", &threshold3, -1.0f, 1.0f)) {
					SetFloatProperty(node, "toonThreshold3", threshold3);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat("Edge Softness", &edgeSoftness, 0.0f, 0.25f)) {
					SetFloatProperty(node, "toonEdgeSoftness", edgeSoftness);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat("Spec Threshold", &specThreshold, 0.0f, 1.0f)) {
					SetFloatProperty(node, "toonSpecularThreshold", specThreshold);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat("Spec Softness", &specSoftness, 0.0f, 0.25f)) {
					SetFloatProperty(node, "toonSpecularSoftness", specSoftness);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat("Spec Intensity", &specIntensity, 0.0f, 4.0f)) {
					SetFloatProperty(node, "toonSpecularIntensity", specIntensity);
					changed = true;
				}
			} else {
				ImGui::TextDisabled("%s", kLightingModes[LightingModeFromNodeType(node.type, 0)]);
			}
		} else if(node.type == "MultiplyColor" || node.type == "MultiplyFloat") {
			ImGui::TextDisabled("A * B");
		} else if(node.type == "LerpColor" || node.type == "LerpFloat") {
			ImGui::TextDisabled("lerp(A, B, T)");
		} else if(node.type == "SaturateFloat") {
			ImGui::TextDisabled("saturate(Value)");
		} else if(node.type == "OneMinusFloat") {
			ImGui::TextDisabled("1 - Value");
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
		if(node.type == "Float") node.floatValue = 0.0f;
		else node.floatValue = node.type == "Roughness" ? material.roughness : material.shininess;
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

	void MaterialNodeEditorPanel::AddLightingNode(MaterialAsset& material, const char* type, const char* title, int32_t mode, Vector2 position) {
		Node node;
		node.id		  = material.graph.AllocateId();
		node.type	  = type;
		node.title	  = title;
		node.position = position;
		node.intValue = mode;
		node.outputs.push_back({material.graph.AllocateId(), "Lighting", NodePinKind::Output, NodeValueType::Int});
		if(node.type == "ToonLighting") {
			SetDefaultToonProperties(node);
		}
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::AddToonMasterNode(MaterialAsset& material, Vector2 position) {
		Node node;
		node.id		  = material.graph.AllocateId();
		node.type	  = "ToonMaster";
		node.title	  = "Toon Master";
		node.position = position;
		SetDefaultToonMasterProperties(node);
		node.inputs.push_back({material.graph.AllocateId(), "Base Color", NodePinKind::Input, NodeValueType::Color});
		node.inputs.push_back({material.graph.AllocateId(), "Highlight", NodePinKind::Input, NodeValueType::Color});
		node.inputs.push_back({material.graph.AllocateId(), "1st Shade", NodePinKind::Input, NodeValueType::Color});
		node.inputs.push_back({material.graph.AllocateId(), "2nd Shade", NodePinKind::Input, NodeValueType::Color});
		node.inputs.push_back({material.graph.AllocateId(), "Base Step", NodePinKind::Input, NodeValueType::Float});
		node.inputs.push_back({material.graph.AllocateId(), "Base Feather", NodePinKind::Input, NodeValueType::Float});
		node.inputs.push_back({material.graph.AllocateId(), "Shade Step", NodePinKind::Input, NodeValueType::Float});
		node.inputs.push_back({material.graph.AllocateId(), "Shade Feather", NodePinKind::Input, NodeValueType::Float});
		node.inputs.push_back({material.graph.AllocateId(), "Spec Threshold", NodePinKind::Input, NodeValueType::Float});
		node.inputs.push_back({material.graph.AllocateId(), "Spec Softness", NodePinKind::Input, NodeValueType::Float});
		node.inputs.push_back({material.graph.AllocateId(), "Spec Intensity", NodePinKind::Input, NodeValueType::Float});
		node.outputs.push_back({material.graph.AllocateId(), "Surface", NodePinKind::Output, NodeValueType::Material});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::AddLitMasterNode(MaterialAsset& material, Vector2 position) {
		Node node;
		node.id		  = material.graph.AllocateId();
		node.type	  = "LitMaster";
		node.title	  = "Lit Master";
		node.position = position;
		node.properties["lightingMode"] = 0.0f;
		node.properties["shininess"] = material.shininess;
		node.properties["roughness"] = material.roughness;
		node.inputs.push_back({material.graph.AllocateId(), "Base Color", NodePinKind::Input, NodeValueType::Color});
		node.inputs.push_back({material.graph.AllocateId(), "Shininess", NodePinKind::Input, NodeValueType::Float});
		node.inputs.push_back({material.graph.AllocateId(), "Roughness", NodePinKind::Input, NodeValueType::Float});
		node.inputs.push_back({material.graph.AllocateId(), "Reflect", NodePinKind::Input, NodeValueType::Bool});
		node.outputs.push_back({material.graph.AllocateId(), "Surface", NodePinKind::Output, NodeValueType::Material});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::AddUnlitMasterNode(MaterialAsset& material, Vector2 position) {
		Node node;
		node.id		  = material.graph.AllocateId();
		node.type	  = "UnlitMaster";
		node.title	  = "Unlit Master";
		node.position = position;
		node.inputs.push_back({material.graph.AllocateId(), "Base Color", NodePinKind::Input, NodeValueType::Color});
		node.outputs.push_back({material.graph.AllocateId(), "Surface", NodePinKind::Output, NodeValueType::Material});
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

	void MaterialNodeEditorPanel::AddLerpNode(MaterialAsset& material, const char* type, const char* title, NodeValueType valueType, Vector2 position) {
		Node node;
		node.id		  = material.graph.AllocateId();
		node.type	  = type;
		node.title	  = title;
		node.position = position;
		node.inputs.push_back({material.graph.AllocateId(), "A", NodePinKind::Input, valueType});
		node.inputs.push_back({material.graph.AllocateId(), "B", NodePinKind::Input, valueType});
		node.inputs.push_back({material.graph.AllocateId(), "T", NodePinKind::Input, NodeValueType::Float});
		node.outputs.push_back({material.graph.AllocateId(), "Result", NodePinKind::Output, valueType});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::AddUnaryFloatNode(MaterialAsset& material, const char* type, const char* title, Vector2 position) {
		Node node;
		node.id		  = material.graph.AllocateId();
		node.type	  = type;
		node.title	  = title;
		node.position = position;
		node.inputs.push_back({material.graph.AllocateId(), "Value", NodePinKind::Input, NodeValueType::Float});
		node.outputs.push_back({material.graph.AllocateId(), "Result", NodePinKind::Output, NodeValueType::Float});
		material.graph.nodes.push_back(std::move(node));
	}

	void MaterialNodeEditorPanel::EnsureOutputNode(MaterialAsset& material) {
		for(auto& node : material.graph.nodes) {
			if(node.type != "ToonMaster") continue;
			auto ensureMasterInput = [&material, &node](const char* name, NodeValueType type) {
				const auto exists = std::any_of(node.inputs.begin(), node.inputs.end(), [name](const NodePin& pin) {
					return pin.name == name;
				});
				if(!exists) node.inputs.push_back({material.graph.AllocateId(), name, NodePinKind::Input, type});
			};
			ensureMasterInput("Base Color", NodeValueType::Color);
			ensureMasterInput("Highlight", NodeValueType::Color);
			ensureMasterInput("1st Shade", NodeValueType::Color);
			ensureMasterInput("2nd Shade", NodeValueType::Color);
			ensureMasterInput("Base Step", NodeValueType::Float);
			ensureMasterInput("Base Feather", NodeValueType::Float);
			ensureMasterInput("Shade Step", NodeValueType::Float);
			ensureMasterInput("Shade Feather", NodeValueType::Float);
			ensureMasterInput("Spec Threshold", NodeValueType::Float);
			ensureMasterInput("Spec Softness", NodeValueType::Float);
			ensureMasterInput("Spec Intensity", NodeValueType::Float);
		}

		for(auto& node : material.graph.nodes) {
			if(node.type != "Output") continue;
			auto ensureInput = [&material, &node](const char* name, NodeValueType type) {
				const auto exists = std::any_of(node.inputs.begin(), node.inputs.end(), [name](const NodePin& pin) {
					return pin.name == name;
				});
				if(!exists) node.inputs.push_back({material.graph.AllocateId(), name, NodePinKind::Input, type});
			};
			ensureInput("Surface", NodeValueType::Material);

			const auto surfaceIt = std::find_if(node.inputs.begin(), node.inputs.end(), [](const NodePin& pin) {
				return pin.name == "Surface";
			});
			const bool hasSurfaceLink = surfaceIt != node.inputs.end() &&
				std::any_of(material.graph.links.begin(), material.graph.links.end(), [&surfaceIt](const NodeLink& link) {
					return link.toPinId == surfaceIt->id;
				});

			if(hasSurfaceLink) {
				std::vector<int32_t> removedPinIds;
				for(const auto& pin : node.inputs) {
					if(IsLegacyOutputPin(pin.name)) {
						removedPinIds.push_back(pin.id);
					}
				}
				node.inputs.erase(
					std::remove_if(node.inputs.begin(), node.inputs.end(), [](const NodePin& pin) {
						return IsLegacyOutputPin(pin.name);
					}),
					node.inputs.end());
				material.graph.links.erase(
					std::remove_if(material.graph.links.begin(), material.graph.links.end(), [&removedPinIds](const NodeLink& link) {
						return std::find(removedPinIds.begin(), removedPinIds.end(), link.toPinId) != removedPinIds.end() ||
							   std::find(removedPinIds.begin(), removedPinIds.end(), link.fromPinId) != removedPinIds.end();
					}),
					material.graph.links.end());
				return;
			}

			ensureInput("Lighting Mode", NodeValueType::Int);
			return;
		}
		Node node;
		node.id		  = material.graph.AllocateId();
		node.type	  = "Output";
		node.title	  = "Output";
		node.position = {420.0f, 120.0f};
		node.inputs.push_back({material.graph.AllocateId(), "Surface", NodePinKind::Input, NodeValueType::Material});
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
			if(IsLightingNode(fromNode->type)) {
				return LightingModeFromNodeType(fromNode->type, fallback);
			}
		}
		return fallback;
	}

	void MaterialNodeEditorPanel::ApplyToonLightingNode(MaterialAsset& material, const Node& node) const {
		if(node.type != "ToonLighting") return;
		material.toonHighlightColor = GetColorProperty(node, "toonHighlightColor", material.toonHighlightColor);
		material.toonBaseColor = GetColorProperty(node, "toonBaseColor", material.toonBaseColor);
		material.toonMidShadowColor = GetColorProperty(node, "toonMidShadowColor", material.toonMidShadowColor);
		material.toonShadowColor = GetColorProperty(node, "toonShadowColor", material.toonShadowColor);
		material.toonThreshold1 = GetFloatProperty(node, "toonThreshold1", material.toonThreshold1);
		material.toonThreshold2 = GetFloatProperty(node, "toonThreshold2", material.toonThreshold2);
		material.toonThreshold3 = GetFloatProperty(node, "toonThreshold3", material.toonThreshold3);
		material.toonEdgeSoftness = GetFloatProperty(node, "toonEdgeSoftness", material.toonEdgeSoftness);
		material.toonShadeStep = material.toonThreshold1;
		material.toonShadeFeather = material.toonEdgeSoftness;
		material.toonBaseStep = material.toonThreshold2;
		material.toonBaseFeather = material.toonEdgeSoftness;
		material.toonSpecularThreshold = GetFloatProperty(node, "toonSpecularThreshold", material.toonSpecularThreshold);
		material.toonSpecularSoftness = GetFloatProperty(node, "toonSpecularSoftness", material.toonSpecularSoftness);
		material.toonSpecularIntensity = GetFloatProperty(node, "toonSpecularIntensity", material.toonSpecularIntensity);
	}

	void MaterialNodeEditorPanel::Evaluate(MaterialAsset& material) {
		MaterialGraphCompiler::Compile(material);
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

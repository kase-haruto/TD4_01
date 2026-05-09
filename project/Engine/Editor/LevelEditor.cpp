#include "LevelEditor.h"

// engine
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Application/System/PlaySession.h>
#include <Engine/Application/UI/EngineUI/Context/EditorContext.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Editor/PickingPass.h>
#include <Engine/Editor/SceneSwitchOverlay.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Graphics/Camera/3d/DebugCamera.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>
#include <Engine/Objects/LightObject/DirectionalLight.h>
#include <Engine/Objects/LightObject/PointLight.h>
#include <Engine/Physics/Ray/Raycastor.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Scene/System/SceneManager.h>
#include <Engine/System/Command/Interface/ICommand.h>
#include <Engine/System/Command/Manager/CommandManager.h>

// imgui
#include <externals/imgui/ImGuiFileDialog.h>
#include <externals/imgui/imgui.h>

// c++
#include "Engine/Foundation/HotReload/LivePP/LivePPService.h"

#include <Engine/Foundation/Utility/FileSystem/FileScanner.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>
#include <Engine/System/Command/EditorCommand/BaseLevelEditorCommand.h>
#include <Engine/System/Command/Manager/CommandManager.h>
#include <externals/nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <functional>
#include <unordered_map>

using namespace EngineEdit;

namespace {

	// ライブラリに同一 GUID のオブジェクトが既に登録されているか？
	// （あまり呼ばない前提なので Contains を使う）
	bool LibraryContains(const SceneObjectLibrary*			 lib,
						 const std::shared_ptr<SceneObject>& sp) {
		if(!lib || !sp) return false;
		return lib->Contains(sp->GetGuid());
	}

	bool IsViewportSelectableObject(const SceneObject* object) {
		if(!object || object->IsTransient() || !object->IsPickable()) return false;
		if(object->GetObjectType() == ObjectType::Light) return false;
		if(object->GetObjectClassName() == "SkyBox") return false;
		if(object->GetName() == "ground") return false;
		return true;
	}

	nlohmann::json CaptureObjectJson(const std::shared_ptr<SceneObject>& object) {
		nlohmann::json j;
		if(!object) return j;

		j["type"] = object->GetObjectClassName();
		j["guid"] = object->GetGuid();
		j["name"] = object->GetName();
		j["inheritScale"] = object->GetWorldTransform().inheritScale;
		if(auto parent = object->GetParent()) {
			j["parentGuid"] = parent->GetGuid();
		}
		const std::string& configPath = object->GetConfigPath();
		if(!configPath.empty()) {
			j["configPath"] = configPath;
		}

		if(auto* cfg = dynamic_cast<const IConfigurable*>(object.get())) {
			nlohmann::json inlineConfig;
			cfg->ExtractConfigToJson(inlineConfig);
			for(auto it = inlineConfig.begin(); it != inlineConfig.end(); ++it) {
				j[it.key()] = it.value();
			}
		}
		return j;
	}

	void CaptureObjectTree(const std::shared_ptr<SceneObject>& object,
						   std::vector<nlohmann::json>& snapshots,
						   std::vector<Guid>& captured) {
		if(!object) return;
		if(std::find(captured.begin(), captured.end(), object->GetGuid()) != captured.end()) {
			return;
		}

		captured.push_back(object->GetGuid());
		snapshots.push_back(CaptureObjectJson(object));
		for(const auto& child : object->GetChildren()) {
			CaptureObjectTree(child, snapshots, captured);
		}
	}

	bool HasSelectedAncestor(const std::shared_ptr<SceneObject>& object,
							 const std::vector<Guid>& selectedGuids) {
		if(!object) return false;
		auto parent = object->GetParent();
		while(parent) {
			if(std::find(selectedGuids.begin(), selectedGuids.end(), parent->GetGuid()) != selectedGuids.end()) {
				return true;
			}
			parent = parent->GetParent();
		}
		return false;
	}

	void RegisterRestoredObject(SceneContext* ctx, const std::shared_ptr<SceneObject>& object) {
		if(!ctx || !object) return;

		if(auto dir = std::dynamic_pointer_cast<DirectionalLight>(object)) {
			ctx->GetLightLibrary()->SetDirectionalLight(dir);
		} else if(auto point = std::dynamic_pointer_cast<PointLight>(object)) {
			ctx->GetLightLibrary()->SetPointLight(point);
		} else if(auto debugCamera = std::dynamic_pointer_cast<DebugCamera>(object)) {
			ctx->GetCameraMgr()->SetDebugCamera(debugCamera);
		} else if(auto camera = std::dynamic_pointer_cast<Camera3d>(object)) {
			ctx->GetCameraMgr()->SetMainCamera(camera);
		}
	}

	class DeleteObjectsCommand final
		: public ICommand {
	public:
		using AfterApply = std::function<void()>;

		DeleteObjectsCommand(SceneContext* ctx,
							 std::vector<std::shared_ptr<SceneObject>> targets,
							 AfterApply afterApply,
							 std::string name)
			: ctx_(ctx),
			  afterApply_(std::move(afterApply)),
			  name_(std::move(name)) {
			std::vector<Guid> selectedGuids;
			selectedGuids.reserve(targets.size());
			for(const auto& target : targets) {
				if(target) selectedGuids.push_back(target->GetGuid());
			}

			std::vector<Guid> captured;
			for(auto& target : targets) {
				if(!target || HasSelectedAncestor(target, selectedGuids)) continue;
				rootGuids_.push_back(target->GetGuid());
				CaptureObjectTree(target, snapshots_, captured);
			}
		}

		void Execute() override {
			DeleteRoots();
			if(afterApply_) afterApply_();
		}

		void Undo() override {
			RestoreSnapshots();
			if(afterApply_) afterApply_();
		}

		void Redo() override {
			DeleteRoots();
			if(afterApply_) afterApply_();
		}

		const char* GetName() const override {
			return name_.c_str();
		}

	private:
		void DeleteRoots() {
			if(!ctx_ || !ctx_->GetObjectLibrary()) return;
			for(const auto& guid : rootGuids_) {
				auto object = ctx_->GetObjectLibrary()->Find(guid);
				if(object) {
					ctx_->RemoveObject(object);
				}
			}
		}

		void RestoreSnapshots() {
			if(!ctx_ || !ctx_->GetObjectLibrary()) return;

			std::unordered_map<Guid, std::shared_ptr<SceneObject>> guidMap;
			for(const auto& j : snapshots_) {
				std::string typeName = j.value("type", "");
				if(typeName.empty()) continue;

				auto object = SceneObjectRegistry::Get().Create(typeName);
				if(!object) continue;

				if(j.contains("configPath")) {
					object->SetConfigPath(j.at("configPath").get<std::string>());
				}
				if(auto* cfg = dynamic_cast<IConfigurable*>(object.get())) {
					cfg->ApplyConfigFromJson(j);
				}

				const Guid guid = j.value("guid", Guid{});
				if(guid.isValid()) {
					object->SetGuid(guid);
				}

				ctx_->AddObject(object);
				object->Initialize();
				object->SetName(j.value("name", object->GetName()), object->GetObjectType());
				RegisterRestoredObject(ctx_, object);

				guidMap[object->GetGuid()] = object;
			}

			for(const auto& j : snapshots_) {
				const Guid childGuid = j.value("guid", Guid{});
				const Guid parentGuid = j.value("parentGuid", Guid{});
				if(!childGuid.isValid() || !parentGuid.isValid()) continue;

				auto childIt = guidMap.find(childGuid);
				if(childIt == guidMap.end()) continue;

				std::shared_ptr<SceneObject> parent;
				auto parentIt = guidMap.find(parentGuid);
				if(parentIt != guidMap.end()) {
					parent = parentIt->second;
				} else {
					parent = ctx_->GetObjectLibrary()->Find(parentGuid);
				}
				if(parent) {
					childIt->second->SetParent(parent, j.value("inheritScale", true));
				}
			}
		}

		SceneContext* ctx_ = nullptr;
		std::vector<Guid> rootGuids_;
		std::vector<nlohmann::json> snapshots_;
		AfterApply afterApply_;
		std::string name_;
	};

} // namespace

namespace CalyxEngine {

	namespace {
		// PrefabSerializer 相当のスナップショット（ファイルに書かずメモリに保持）
		// - 削除対象のルートと子を再帰的に JSON 配列へ変換
		// - Undo ではこの JSON から SceneObject を再生成し GUID/親子関係を復元する
		nlohmann::json SerializeSubtree(SceneObject* root) {
			nlohmann::json jArray = nlohmann::json::array();
			std::function<void(SceneObject*)> rec;
			rec = [&](SceneObject* obj) {
				if(!obj || !obj->IsSerializable()) return;

				nlohmann::json j;
				j["type"] = obj->GetObjectClassName();
				j["guid"] = obj->GetGuid();
				j["pickingID"] = obj->GetPickingID();
				j["name"] = obj->GetName();
				j["objectType"] = static_cast<int>(obj->GetObjectType());
				if(auto parent = obj->GetParent()) {
					j["parentGuid"] = parent->GetGuid();
				} else {
					j["parentGuid"] = Guid::Empty();
				}

				if(auto* cfg = dynamic_cast<IConfigurable*>(obj)) {
					// 既存のキーと衝突しないよう一旦作ってマージ
					nlohmann::json jCfg;
					cfg->ExtractConfigToJson(jCfg);
					for(auto it = jCfg.begin(); it != jCfg.end(); ++it) {
						j[it.key()] = it.value();
					}
				}

				jArray.push_back(std::move(j));
				// FxObject は ApplyConfig 時に emitter 子を再構築するため、
				// ここで子まで保存すると Undo 時に二重生成されやすい。
				const std::string_view className = obj->GetObjectClassName();
				if(className != "FxObject") {
					for(auto& childSp : obj->GetChildren()) {
						if(childSp) rec(childSp.get());
					}
				}
			};
			rec(root);
			return jArray;
		}
	} // namespace

	class DeleteObjectCommand final : public BaseLevelEditorCommand {
	public:
		DeleteObjectCommand(SceneContext* ctx,
							const std::shared_ptr<SceneObject>& object,
							LevelEditor* editor,
							const char* label = "Delete Object")
			: BaseLevelEditorCommand(label), ctx_(ctx), editor_(editor) {
			if(object) {
				rootGuid_ = object->GetGuid();
				snapshot_ = SerializeSubtree(object.get());
			}
		}

		void Execute() override {
			if(!ctx_ || !rootGuid_.isValid()) return;
			auto* lib = ctx_->GetObjectLibrary();
			if(!lib) return;
			auto rootSp = lib->Find(rootGuid_);
			if(!rootSp) return;

			ctx_->RemoveObject(rootSp);
			if(editor_ && editor_->GetHierarchyPanel()) editor_->GetHierarchyPanel()->RefreshCache();
		}

		void Undo() override {
			if(!ctx_) return;
			auto* lib = ctx_->GetObjectLibrary();
			if(!lib) return;
			if(!snapshot_.is_array() || snapshot_.empty()) return;

			std::unordered_map<Guid, std::shared_ptr<SceneObject>> guidMap;
			guidMap.reserve(snapshot_.size());

			// 1) 生成 + 設定適用 + GUID 復元 + ライブラリ登録 + Initialize
			for(const auto& j : snapshot_) {
				const std::string typeName = j.value("type", "");
				if(typeName.empty()) continue;

				auto sp = SceneObjectRegistry::Get().Create(typeName);
				if(!sp) continue;

				if(auto* cfg = dynamic_cast<IConfigurable*>(sp.get())) {
					cfg->ApplyConfigFromJson(j);
				}
				// IConfigurable が無い型（例: SceneObject）でも最低限の情報を復元
				if(!dynamic_cast<IConfigurable*>(sp.get())) {
					const std::string name = j.value("name", "");
					const int objTypeInt = j.value("objectType", static_cast<int>(sp->GetObjectType()));
					if(!name.empty()) {
						sp->SetName(name, static_cast<ObjectType>(objTypeInt));
					}
				}

				const Guid guid = j.value("guid", Guid{});
				if(guid.isValid()) {
					sp->SetGuid(guid);
				}
				const uint32_t pickingID = j.value("pickingID", 0u);
				if(pickingID != 0u) {
					sp->SetPickingID(pickingID);
				}

				ctx_->AddObject(sp);
				sp->Initialize();

				guidMap[sp->GetGuid()] = sp;
			}

			// 2) 親子リンク復元
			for(const auto& j : snapshot_) {
				const Guid childGuid = j.value("guid", Guid{});
				const Guid parentGuid = j.value("parentGuid", Guid{});
				if(!childGuid.isValid() || !parentGuid.isValid()) continue;

				auto childIt = guidMap.find(childGuid);
				if(childIt == guidMap.end() || !childIt->second) continue;

				std::shared_ptr<SceneObject> parentSp;
				if(auto parentIt = guidMap.find(parentGuid); parentIt != guidMap.end()) {
					parentSp = parentIt->second;
				} else {
					parentSp = lib->Find(parentGuid);
				}

				if(parentSp) {
					auto& childTransform = childIt->second->GetWorldTransform();
					childIt->second->SetParent(parentSp, childTransform.inheritScale);
				}
			}

			if(editor_ && editor_->GetHierarchyPanel()) editor_->GetHierarchyPanel()->RefreshCache();
		}

	private:
		SceneContext* ctx_ = nullptr;
		LevelEditor* editor_ = nullptr;
		Guid rootGuid_{};
		nlohmann::json snapshot_;
	};

	//=============================================================================
	// Initialize
	//=============================================================================
	LevelEditor::~LevelEditor() = default;

	void LevelEditor::Initialize() {
#if defined(_DEBUG) || defined(DEVELOP)
		// 各パネルの初期化 ----------------------------------------------------
		hierarchy_			= std::make_unique<HierarchyPanel>();
		editor_				= std::make_unique<EditorPanel>();
		inspector_			= std::make_unique<InspectorPanel>();
		sceneEditor_		= std::make_unique<SceneObjectEditor>();
		placeToolPanel_		= std::make_unique<PlaceToolPanel>();
		splineEditor_		= std::make_unique<SplineEditorPanel>();
		assetPanel_			= std::make_unique<AssetPanel>();
		materialNodeEditorPanel_ = std::make_unique<MaterialNodeEditorPanel>();
		postEffectNodeEditorPanel_ = std::make_unique<PostEffectNodeEditorPanel>();
		livePPPanel_		= std::make_unique<LivePPPanel>();
		sceneSwitchOverlay_ = std::make_unique<SceneSwitchOverlay>();

		// レイアウトスイッチャーの初期化 --------------------------------------
		std::string				 layoutDir = "Resources/Assets/Configs/Editor/Layout/";
		auto					 files	   = CalyxEngine::FileScanner::ScanFiles(layoutDir, ".ini");
		std::vector<LayoutEntry> layouts;

		for(const auto& file : files) {
			layouts.push_back({CalyxEngine::FileScanner::GetFileName(file), file.generic_string()});
		}
		// ファイルが見つからなかった場合のフォールバック（念のため）
		if(layouts.empty()) {
			layouts.push_back({"Default", "Resources/Assets/Configs/Editor/Layout/default.ini"});
		}

		layoutSwitcher_ = std::make_unique<ImGuiLayoutSwitcher>(std::move(layouts), "imgui.ini");

		if(auto* db = AssetDatabase::GetInstance()) {
			assetPanel_->Initialize(db->GetRoot());
		}

		// Panel に LevelEditor 自体を渡す（コールバック通知や setter） ----------
		editor_->SetOnEditorSelected(
			[this](BaseEditor* ed) { SetSelectedEditor(ed); });

		// Hierarchy から来るコールバックは shared_ptr で受けて、
		// LevelEditor 内で weak_ptr に変換して管理する
		hierarchy_->SetOnObjectSelected(
			[this](std::shared_ptr<SceneObject> sp, bool toggle) {
				if(toggle) {
					ToggleSelectedObject(sp);
				} else {
					SetSelectedObject(sp);
				}
			});

		hierarchy_->SetOnObjectDelete(
			[this](std::shared_ptr<SceneObject> sp) { DeleteObject(std::move(sp)); });

		hierarchy_->SetOnObjectCreate(
			[this](std::shared_ptr<SceneObject> sp) { CreateObject(std::move(sp)); });

		hierarchy_->SetOnObjectRename(
			[](std::shared_ptr<SceneObject> sp, const std::string& newName) {
				if(!sp) return;
				if(auto* ctx = SceneContext::Current()) {
					if(auto* lib = ctx->GetObjectLibrary()) {
						lib->RenameObject(sp, newName);
						return;
					}
				}
				sp->SetName(newName, sp->GetObjectType());
			});

		inspector_->SetSceneObjectEditor(sceneEditor_.get());

		// ビューポートの初期化 ------------------------------------------------
		mainViewport_	 = std::make_unique<Viewport>(ViewportType::VIEWPORT_MAIN, "Game Viewport");
		debugViewport_	 = std::make_unique<Viewport>(ViewportType::VIEWPORT_DEBUG, "Debug Viewport");
		pickingViewport_ = std::make_unique<Viewport>(ViewportType::VIEWPORT_PICKING, "Picking Viewport");

		performanceOverlay_ = std::make_unique<PerformanceOverlay>();
		debugOverlay_		= std::make_unique<DebugOverlay>();

		// Manipulator をツールとして登録
		if(auto* manipulator = sceneEditor_->GetManipulator()) {
			debugViewport_->AddTool(manipulator);
			debugViewport_->AddTool(performanceOverlay_.get());
			debugViewport_->AddTool(debugOverlay_.get());
		}

		// エディターメニューの初期化 ------------------------------------------
		menu_ = std::make_unique<EditorMenu>();

		// --- Advanced Hot Reload (Object Re-instancing) ---
		if(auto* lpp = CalyxEngine::LivePPService::GetInstance()) {
			lpp->AddPrePatchListener([this]() {
				if(auto* ctx = SceneContext::Current()) {
					livePPSnapshot_ = SceneSerializer::DumpJson(*ctx);
					OutputDebugStringW(L"[LivePP] Scene snapshot taken.\n");
				}
			});

			lpp->AddPostPatchListener([this]() {
				if(auto* ctx = SceneContext::Current()) {
					if(!livePPSnapshot_.empty()) {
						ClearSelection();
						ctx->Clear();
						SceneSerializer::LoadJson(*ctx, livePPSnapshot_);
						livePPSnapshot_.clear();
						OutputDebugStringW(L"[LivePP] Scene re-instanced from snapshot.\n");
					}
				}
			});
		}

		// File: Save Scene
		menu_->Add(MenuCategory::File,
				   {"Save Scene",
					"Ctrl+S",
					[this]() {
						IGFD::FileDialogConfig config;
						config.path = "Resources/Assets/Scenes/";
						ImGuiFileDialog::Instance()->OpenDialog(
							"SceneSaveDialog",
							"save scene file",
							".scene",
							config);
					},
					true});

		// File: Open Scene
		menu_->Add(MenuCategory::File,
				   {"Open Scene",
					"Ctrl+O",
					[] {
						IGFD::FileDialogConfig config;
						config.path = "Resources/Assets/Scenes/";
						ImGuiFileDialog::Instance()->OpenDialog(
							"SceneOpenDialog",
							"open scene",
							".scene",
							config);
					},
					true});

		// View: Game Mode トグル
		if(mode_ == EditorMode::Edit) {
			menu_->Add(MenuCategory::View,
					   {"Enter Game Mode", "", [this]() { ToggleMode(); }, true});
		} else {
			menu_->Add(MenuCategory::View,
					   {"Exit Game Mode", "", [this]() { ToggleMode(); }, true});
		}

		// パネル群を登録（Editors メニューに並べる） --------------------------
		editorPanels_.push_back(hierarchy_.get());
		editorPanels_.push_back(editor_.get());
		editorPanels_.push_back(inspector_.get());
		editorPanels_.push_back(placeToolPanel_.get());
		editorPanels_.push_back(splineEditor_.get());
		editorPanels_.push_back(assetPanel_.get());
		editorPanels_.push_back(materialNodeEditorPanel_.get());
		editorPanels_.push_back(postEffectNodeEditorPanel_.get());
		editorPanels_.push_back(livePPPanel_.get());

		ApplyEditToolMode(editToolMode_, true);

		// Editors メニュー（MenuCategory::Tools）に各パネルのトグルを追加
		for(auto* p : editorPanels_) {
			// LivePPPanel は自動表示なのでメニューには出さない
			if(p == livePPPanel_.get()) continue;

			menu_->Add(MenuCategory::Tools,
					   {p->GetPanelName(),
						"",
						[p, this]() { TogglePanel(p); },
						true});
		}

		// Viewport 表示トグル
		menu_->Add(MenuCategory::View,
				   {"Main Viewport",
					"",
					[this] {
						if(mainViewport_) {
							mainViewport_->SetShow(!mainViewport_->IsShow());
						}
					},
					true});

		menu_->Add(MenuCategory::View,
				   {"Debug Viewport",
					"",
					[this] {
						if(debugViewport_) {
							debugViewport_->SetShow(!debugViewport_->IsShow());
						}
					},
					true});

		// Play / Pause / Exit（PlaySession 連携） ------------------------------
		menu_->Add(MenuCategory::Edit,
				   {"Play ",
					"(F5)",
					[this] {
						if(pPlaySesseion_ && !pPlaySesseion_->IsRuntime()) {
							pPlaySesseion_->Enter();
						}
					},
					true});

		menu_->Add(MenuCategory::Edit,
				   {"Pause ",
					"(F6)",
					[this] {
						if(pPlaySesseion_ && pPlaySesseion_->IsRuntime()) {
							pPlaySesseion_->TogglePause();
						}
					},
					true});

		menu_->Add(MenuCategory::Edit,
				   {"Exit ",
					"(Shift+F5)",
					[this] {
						if(pPlaySesseion_ && pPlaySesseion_->IsRuntime()) {
							pPlaySesseion_->Exit();
						}
					},
					true});

		// Edit: Hot Reload
		menu_->Add(MenuCategory::Edit,
				   {"Hot Reload (Live++)",
					"Ctrl+Alt+F11",
					[] {
						if(auto* service = CalyxEngine::LivePPService::GetInstance()) {
							service->TriggerReload();
						}
					},
					true});

		// Scene: Switch Scene
#endif // _DEBUG || DEVELOP
	}

	//=============================================================================
	// Update
	//=============================================================================
	void LevelEditor::Update() {
#if defined(_DEBUG) || defined(DEVELOP)
		if(layoutSwitcher_) {
			layoutSwitcher_->ApplyPending();
		}

		if(editToolMode_ == EngineEdit::EditToolMode::ParticleEffect) {
			UpdateParticlePreviewContext(ClockManager::GetInstance()->GetDeltaTime());
			if(particlePreviewContext_) {
				particlePreviewContext_->MakeCurrent();
			}
		}

		SceneContext* ctx = SceneContext::Current();

		const ImGuiIO& io			= ImGui::GetIO();
		const bool	   guizmoActive = ImGuizmo::IsOver() || ImGuizmo::IsUsing();

		// --- デバッグビューポート上にマウスがあるか？ -----------------------
		bool overDebugViewport = false;
		if(debugViewport_ && debugViewport_->IsShow()) {
			overDebugViewport = debugViewport_->IsHovered();
		}

		// ImGui がマウスを掴んでいても、ビューポート上なら許可
		const bool uiBlocksClick = io.WantCaptureMouse && !overDebugViewport;

		if(auto* debugCam = CameraManager::GetDebug()) {
			debugCam->SetInputEnabled(overDebugViewport);
		}

		if(editToolMode_ != EngineEdit::EditToolMode::ParticleEffect &&
		   debugViewport_ && debugViewport_->IsShow() && !guizmoActive && !uiBlocksClick) {
			UpdateViewportSelectionInput();
		} else if(!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			rangeSelectCandidate_ = false;
			rangeSelecting_ = false;
		}

		if(!selectedObjects_.empty() && !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
			DeleteSelectedObjects();
		}

		// ----------------------------
		// Open Scene ダイアログ処理
		// ----------------------------
		if(ImGuiFileDialog::Instance()->Display("SceneOpenDialog")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
				ClearSelection();
				SceneSerializer::Load(*ctx, filePath);
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// ----------------------------
		// Save Scene ダイアログ処理
		// ----------------------------
		if(ImGuiFileDialog::Instance()->Display("SceneSaveDialog")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
				SceneSerializer::Save(*ctx, filePath);
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// シーンコンテキスト切り替え検出
		NotifySceneContextChanged();
		prevCtx_ = SceneContext::Current();
		if(editToolMode_ == EngineEdit::EditToolMode::ParticleEffect && particlePreviewFx_) {
			if(particlePreviewContext_) {
				particlePreviewContext_->MakeCurrent();
			}
			SetSelectedObject(particlePreviewFx_);
		}

		// PlaySession 状態と EditorMode の同期 ------------------------------
		if(pPlaySesseion_) {
			const bool playing = pPlaySesseion_->IsRuntime();
			if(playing && !lastPlaying_) {
				EnterGameMode();
			} else if(!playing && lastPlaying_) {
				ExitGameMode();
			}
			lastPlaying_ = playing;
		}

		// LivePP Visibility Control
		if(livePPPanel_) {
			auto* service	 = CalyxEngine::LivePPService::GetInstance();
			bool  shouldShow = (service && service->GetStatus() != CalyxEngine::LivePPStatus::Idle);
			livePPPanel_->SetShow(shouldShow);
		}
#endif
	}

	//=============================================================================
	// Render
	//=============================================================================
	void LevelEditor::Render() {
		SceneContext* previousContext = nullptr;
		if(editToolMode_ == EngineEdit::EditToolMode::ParticleEffect && particlePreviewContext_) {
			previousContext = SceneContext::Current();
			particlePreviewContext_->MakeCurrent();
		}

		// 各パネル描画
		for(auto* p : editorPanels_) {
			if(p->IsShow()) {
				p->Render();
			}
		}

		// Play セッション用ツールバー
		if(pPlaySesseion_) {
			pPlaySesseion_->RenderToolbar();
		}

		// シーン切り替えツールバー
		if(sceneSwitchOverlay_) {
			sceneSwitchOverlay_->RenderToolbar();
		}

		// パフォーマンス表示
		if(performanceOverlay_) {
			performanceOverlay_->RenderToolbar();
		}

		// SceneObjectEditor 側の更新（マニピュレータなど）
		if(sceneEditor_) {
			sceneEditor_->Update();
		}

		if(previousContext && previousContext != particlePreviewContext_.get()) {
			previousContext->MakeCurrent();
		}
	}

	//=============================================================================
	// Menu
	//=============================================================================
	void LevelEditor::RenderMenu() {
		if(menu_) {
			menu_->Render();
		}
		if(layoutSwitcher_ && ImGui::BeginMainMenuBar()) {
			DrawEditModeCombo();
			ImGui::EndMainMenuBar();
		}
	}

	void LevelEditor::DrawEditModeCombo() {
		ImGui::Separator();
		ImGui::TextUnformatted("EditMode");
		ImGui::SetNextItemWidth(140.0f);
		if(ImGui::BeginCombo("##EditToolMode", GetEditToolModeName(editToolMode_))) {
			if(layoutSwitcher_ && ImGui::BeginMenu("Object")) {
				for(const auto& preset : layoutSwitcher_->GetPresets()) {
					const bool selected = editToolMode_ == EngineEdit::EditToolMode::Object &&
										  preset.path == layoutSwitcher_->GetCurrentPath();
					if(ImGui::MenuItem(preset.name.c_str(), nullptr, selected)) {
						ApplyEditToolMode(EngineEdit::EditToolMode::Object, false);
						layoutSwitcher_->Apply(preset.path);
					}
				}
				ImGui::EndMenu();
			} else if(!layoutSwitcher_) {
				const bool selected = editToolMode_ == EngineEdit::EditToolMode::Object;
				if(ImGui::Selectable(GetEditToolModeName(EngineEdit::EditToolMode::Object), selected)) {
					ApplyEditToolMode(EngineEdit::EditToolMode::Object, false);
				}
			}

			const EngineEdit::EditToolMode modes[] = {
				EngineEdit::EditToolMode::ParticleEffect,
				EngineEdit::EditToolMode::PostEffect,
				EngineEdit::EditToolMode::Material,
				EngineEdit::EditToolMode::Animation,
			};

			for(const auto mode : modes) {
				const bool selected = (mode == editToolMode_);
				if(ImGui::Selectable(GetEditToolModeName(mode), selected)) {
					ApplyEditToolMode(mode, true);
				}
				if(selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	const char* LevelEditor::GetEditToolModeName(EngineEdit::EditToolMode mode) const {
		switch(mode) {
		case EngineEdit::EditToolMode::Object:
			return "Object";
		case EngineEdit::EditToolMode::ParticleEffect:
			return "Particle";
		case EngineEdit::EditToolMode::PostEffect:
			return "PostEffect";
		case EngineEdit::EditToolMode::Material:
			return "Material";
		case EngineEdit::EditToolMode::Animation:
			return "Animation";
		default:
			return "Object";
		}
	}

	std::string LevelEditor::GetEditToolModeLayoutPath(EngineEdit::EditToolMode mode) const {
		const std::string layoutDir = "Resources/Assets/Configs/Editor/Layout/";

		switch(mode) {
		case EngineEdit::EditToolMode::Object:
			return layoutDir + "ObjectEdit.ini";
		case EngineEdit::EditToolMode::ParticleEffect:
			return layoutDir + "ParticleEffectEdit.ini";
		case EngineEdit::EditToolMode::PostEffect:
			return layoutDir + "PostEfectEdit.ini";
		case EngineEdit::EditToolMode::Material:
			return layoutDir + "MaterialEdit.ini";
		case EngineEdit::EditToolMode::Animation:
			return layoutDir + "AnimationEdit.ini";
		default:
			return layoutDir + "ObjectEdit.ini";
		}
	}

	std::string LevelEditor::GetEditToolModeLoadLayoutPath(EngineEdit::EditToolMode mode) const {
		const std::string modeLayoutPath = GetEditToolModeLayoutPath(mode);
		if(std::filesystem::exists(modeLayoutPath)) {
			return modeLayoutPath;
		}

		if(mode == EngineEdit::EditToolMode::Object) {
			return "Resources/Assets/Configs/Editor/Layout/gameEngineDefault.ini";
		}

		return modeLayoutPath;
	}

	void LevelEditor::EnsureParticlePreviewContext() {
		if(particlePreviewContext_) return;

		SceneContext* previous = SceneContext::Current();

		particlePreviewContext_ = std::make_unique<SceneContext>();
		particlePreviewContext_->Initialize(false);
		particlePreviewContext_->SetSceneName("ParticleEffectPreview");

		particlePreviewFx_ = particlePreviewContext_->Instantiate<CalyxEngine::FxObject>("ParticlePreview");
		particlePreviewFx_->SetTransient(true);
		particlePreviewFx_->SetEnablePicking(false);
		particlePreviewFx_->Initialize();
		particlePreviewPlayedEmitterRevision_ = particlePreviewFx_->GetEmitterRevision();

		if(auto* debugCamera = particlePreviewContext_->GetCameraMgr()->GetDebug()) {
			debugCamera->GetWorldTransform().translation = {0.0f, 4.0f, -10.0f};
			debugCamera->GetWorldTransform().Update();
		}

		if(previous) {
			previous->MakeCurrent();
		}
	}

	void LevelEditor::UpdateParticlePreviewContext(float dt) {
		if(!particlePreviewContext_) return;

		SceneContext* previous = SceneContext::Current();
		particlePreviewContext_->MakeCurrent();

		if(particlePreviewFx_) {
			const uint64_t emitterRevision = particlePreviewFx_->GetEmitterRevision();
			if(emitterRevision != particlePreviewPlayedEmitterRevision_) {
				particlePreviewFx_->PlayAll();
				particlePreviewPlayedEmitterRevision_ = emitterRevision;
			}
		}

		particlePreviewContext_->Update(dt, dt, false);

		if(previous && previous != particlePreviewContext_.get()) {
			previous->MakeCurrent();
		}
	}

	void LevelEditor::ApplyEditToolMode(EngineEdit::EditToolMode mode, bool applyLayout) {
		editToolMode_ = mode;
		if(mode != EngineEdit::EditToolMode::ParticleEffect && sceneManager_) {
			if(auto* active = sceneManager_->ActiveCtx()) {
				active->MakeCurrent();
			}
		}

		auto setShow = [](IEngineUI* panel, bool show) {
			if(panel) {
				panel->SetShow(show);
			}
		};

		if(mainViewport_) mainViewport_->SetShow(true);
		if(debugViewport_) debugViewport_->SetShow(true);
		if(debugViewport_) debugViewport_->SetOverlayToolsEnabled(true);

		switch(mode) {
		case EngineEdit::EditToolMode::Object:
			if(sceneManager_) sceneManager_->SetEditorPreviewContext(nullptr);
			setShow(hierarchy_.get(), true);
			setShow(editor_.get(), true);
			setShow(inspector_.get(), true);
			setShow(placeToolPanel_.get(), true);
			setShow(splineEditor_.get(), false);
			setShow(assetPanel_.get(), true);
			setShow(materialNodeEditorPanel_.get(), false);
			setShow(postEffectNodeEditorPanel_.get(), false);
			break;
		case EngineEdit::EditToolMode::ParticleEffect:
			EnsureParticlePreviewContext();
			if(sceneManager_) sceneManager_->SetEditorPreviewContext(particlePreviewContext_.get());
			if(particlePreviewContext_) particlePreviewContext_->MakeCurrent();
			if(particlePreviewFx_) SetSelectedObject(particlePreviewFx_);
			if(mainViewport_) mainViewport_->SetShow(false);
			if(debugViewport_) debugViewport_->SetShow(true);
			if(debugViewport_) debugViewport_->SetOverlayToolsEnabled(false);
			setShow(hierarchy_.get(), false);
			setShow(editor_.get(), false);
			setShow(inspector_.get(), true);
			setShow(placeToolPanel_.get(), false);
			setShow(splineEditor_.get(), false);
			setShow(assetPanel_.get(), true);
			setShow(materialNodeEditorPanel_.get(), false);
			setShow(postEffectNodeEditorPanel_.get(), false);
			break;
		case EngineEdit::EditToolMode::PostEffect:
			if(sceneManager_) sceneManager_->SetEditorPreviewContext(nullptr);
			setShow(hierarchy_.get(), false);
			setShow(editor_.get(), false);
			setShow(inspector_.get(), false);
			setShow(placeToolPanel_.get(), false);
			setShow(splineEditor_.get(), false);
			setShow(assetPanel_.get(), true);
			setShow(materialNodeEditorPanel_.get(), false);
			setShow(postEffectNodeEditorPanel_.get(), true);
			break;
		case EngineEdit::EditToolMode::Material:
			if(sceneManager_) sceneManager_->SetEditorPreviewContext(nullptr);
			setShow(hierarchy_.get(), false);
			setShow(editor_.get(), false);
			setShow(inspector_.get(), true);
			setShow(placeToolPanel_.get(), false);
			setShow(splineEditor_.get(), false);
			setShow(assetPanel_.get(), true);
			setShow(materialNodeEditorPanel_.get(), true);
			setShow(postEffectNodeEditorPanel_.get(), false);
			break;
		case EngineEdit::EditToolMode::Animation:
			if(sceneManager_) sceneManager_->SetEditorPreviewContext(nullptr);
			setShow(hierarchy_.get(), true);
			setShow(editor_.get(), false);
			setShow(inspector_.get(), true);
			setShow(placeToolPanel_.get(), false);
			setShow(splineEditor_.get(), true);
			setShow(assetPanel_.get(), true);
			setShow(materialNodeEditorPanel_.get(), false);
			setShow(postEffectNodeEditorPanel_.get(), false);
			break;
		default:
			break;
		}

		if(applyLayout && layoutSwitcher_) {
			layoutSwitcher_->Apply(GetEditToolModeLoadLayoutPath(mode));
		}
	}

	//=============================================================================
	// Game Mode 切り替え
	//=============================================================================
	void LevelEditor::EnterGameMode() {
		mode_ = EditorMode::Game;
	}

	void LevelEditor::ExitGameMode() {
		mode_ = EditorMode::Edit;

		if(pPlaySesseion_ && pPlaySesseion_->IsRuntime()) {
			pPlaySesseion_->Exit();
			lastPlaying_ = false;
		}
	}

	void LevelEditor::ToggleMode() {
		if(mode_ == EditorMode::Edit) {
			mode_ = EditorMode::Game;
		} else {
			mode_ = EditorMode::Edit;
		}
	}

	//=============================================================================
	// Selection API
	//=============================================================================
	void LevelEditor::SetSelectedEditor(BaseEditor* editor) {
		selectedEditor_ = editor;
		selectedObjects_.clear();

		if(inspector_) {
			inspector_->SetSelectedEditor(editor);
			// オブジェクト選択はクリア
			inspector_->SetSelectedObject(std::shared_ptr<SceneObject>{});
		}
		if(sceneEditor_) {
			sceneEditor_->ClearSelection();
		}
	}

	void LevelEditor::SetSelectedObject(const std::shared_ptr<SceneObject>& sp) {
		if(sp) {
			SetSelectedObjects({sp});
		} else {
			SetSelectedObjects({});
		}
	}

	void LevelEditor::ToggleSelectedObject(const std::shared_ptr<SceneObject>& sp) {
		if(!sp) {
			ClearSelection();
			return;
		}

		std::vector<std::shared_ptr<SceneObject>> objects;
		objects.reserve(selectedObjects_.size() + 1);

		bool removed = false;
		for(auto& weak : selectedObjects_) {
			auto current = weak.lock();
			if(!current) continue;
			if(current == sp) {
				removed = true;
				continue;
			}
			objects.push_back(current);
		}

		if(!removed) {
			objects.push_back(sp);
		}

		SetSelectedObjects(objects);
	}

	void LevelEditor::SetSelectedObjects(const std::vector<std::shared_ptr<SceneObject>>& objects) {
		selectedObjects_.clear();
		selectedEditor_ = nullptr;

		std::vector<std::shared_ptr<SceneObject>> validObjects;
		validObjects.reserve(objects.size());
		for(const auto& object : objects) {
			if(!object) continue;
			if(std::find(validObjects.begin(), validObjects.end(), object) != validObjects.end()) continue;
			validObjects.push_back(object);
			selectedObjects_.push_back(object);
		}

		// Hierarchy / Inspector / SceneObjectEditor にも通知
		if(hierarchy_) {
			hierarchy_->SetSelectedObjects(validObjects);
		}
		if(inspector_) {
			inspector_->SetSelectedEditor(nullptr);
			inspector_->SetSelectedObjects(selectedObjects_);
		}
		if(sceneEditor_) {
			std::vector<SceneObject*> rawObjects;
			rawObjects.reserve(validObjects.size());
			for(const auto& object : validObjects) {
				rawObjects.push_back(object.get());
			}
			sceneEditor_->SetTargets(rawObjects);
		}

		if(auto* ctx = SceneContext::Current()) {
			std::vector<SceneObject*> rawObjects;
			rawObjects.reserve(validObjects.size());
			for(const auto& object : validObjects) {
				rawObjects.push_back(object.get());
			}
			ctx->SetDebugSelectedObjects(rawObjects);
		}
	}

	bool LevelEditor::IsSelectedObject(const SceneObject* object) const {
		if(!object) return false;
		for(const auto& weak : selectedObjects_) {
			auto selected = weak.lock();
			if(selected.get() == object) return true;
		}
		return false;
	}

	std::shared_ptr<SceneObject> LevelEditor::GetPrimarySelectedObject() const {
		for(auto it = selectedObjects_.rbegin(); it != selectedObjects_.rend(); ++it) {
			if(auto selected = it->lock()) {
				return selected;
			}
		}
		return nullptr;
	}

	//=============================================================================
	// Create / Delete Object
	//=============================================================================
	void LevelEditor::CreateObject(const std::shared_ptr<SceneObject>& obj) {
		if(!obj) return;

		SceneContext* ctx = SceneContext::Current();
		if(!ctx) return;

		auto* lib = ctx->GetObjectLibrary();
		if(!lib) return;

		// すでに同一 GUID が登録済みなら何もしない（Prefab の重複登録防止）
		if(LibraryContains(lib, obj)) {
			return;
		}

		// SceneContext 経由で登録（内部で SceneObjectLibrary::AddObject を呼ぶ）
		ctx->AddObject(obj);

		if(hierarchy_) hierarchy_->RefreshCache();
	}

	void LevelEditor::DeleteObject(const std::shared_ptr<SceneObject>& sp) {
		if(!sp) return;

		SceneContext* ctx = SceneContext::Current();
		if(!ctx) return;

		// ── 選択状態をクリア（選択中だった場合） ─────────────────────
		if(IsSelectedObject(sp.get())) {
			ClearSelection();
		}

		CommandManager::GetInstance()->Execute(
			std::make_unique<DeleteObjectCommand>(ctx, sp, this));
	}

	void LevelEditor::DeleteSelectedObjects() {
		std::vector<std::shared_ptr<SceneObject>> targets;
		targets.reserve(selectedObjects_.size());

		for(auto& weak : selectedObjects_) {
			auto object = weak.lock();
			if(!object) continue;
			if(std::find(targets.begin(), targets.end(), object) != targets.end()) continue;
			targets.push_back(object);
		}

		if(targets.empty()) {
			ClearSelection();
			return;
		}

		SceneContext* ctx = SceneContext::Current();
		if(!ctx) return;

		auto afterApply = [this]() {
			ClearSelection();
			if(hierarchy_) hierarchy_->RefreshCache();
		};

		CommandManager::GetInstance()->Execute(
			std::make_unique<DeleteObjectsCommand>(
				ctx,
				std::move(targets),
				std::move(afterApply),
				"Delete Selected Objects"));
	}

	//=============================================================================
	// Viewport
	//=============================================================================
	void LevelEditor::RenderViewport(ViewportType type, const ImTextureID& tex) {
		if(type == ViewportType::VIEWPORT_MAIN) {
			if(mainViewport_ && mainViewport_->IsShow()) {
				mainViewport_->Render(tex);
			}
		} else if(type == ViewportType::VIEWPORT_DEBUG) {
			if(debugViewport_ && debugViewport_->IsShow()) {
				const bool focusDebugViewport = startupDebugViewportFocusFrames_ > 0;
				if(focusDebugViewport) {
					ImGui::SetNextWindowFocus();
				}

				debugViewport_->Render(tex);

				if(focusDebugViewport) {
					ImGui::SetWindowFocus("Debug Viewport");
					--startupDebugViewportFocusFrames_;
				}

				DrawViewportSelectionRect();
			}
		} //else if(type == ViewportType::VIEWPORT_PICKING) {
		// 	if(pickingViewport_ && pickingViewport_->IsShow()) {
		// 		pickingViewport_->Render(tex);
		// 	}
		// }
	}

	void LevelEditor::SetCameraForViewport(BaseCamera* mainCamera, BaseCamera* debugCamera) {
		if(mainViewport_) {
			mainViewport_->SetCamera(mainCamera);
		}
		if(debugViewport_) {
			debugViewport_->SetCamera(debugCamera);
		}
	}

	//=============================================================================
	// Picking
	//=============================================================================
	void LevelEditor::TryPickObjectFromMouse(const CalyxEngine::Vector2&	 mouse,
											 const CalyxEngine::Vector2&	 viewportSize,
											 const CalyxEngine::Matrix4x4& view,
											 const CalyxEngine::Matrix4x4& proj) {
		SceneContext* ctx = SceneContext::Current();
		if(!ctx || !debugViewport_) return;

		// ビューポート内ローカル座標へ変換
		CalyxEngine::Vector2 mouseLocal = mouse - debugViewport_->GetPosition();

		Ray ray = Raycastor::ConvertMouseToRay(mouseLocal, view, proj, viewportSize);

		// ヒット判定（raw ptr）
		if(SceneObject* raw = PickSceneObjectByRay(ray)) {
			// 対応する shared_ptr をライブラリから取得
			if(auto sp = ctx->FindSharedObject(raw)) {
				const bool toggle = ImGui::GetIO().KeyCtrl;
				if(toggle) {
					ToggleSelectedObject(sp);
				} else {
					SetSelectedObject(sp);
				}
			}
		}
	}

	SceneObject* LevelEditor::PickSceneObjectByRay(const Ray& ray) {
		const auto* lib = hierarchy_ ? hierarchy_->GetSceneObjectLibrary() : nullptr;
		if(!lib) return nullptr;

		const auto& allObjects = lib->GetAllObjectsRaw();
		std::vector<SceneObject*> pickableObjects;
		pickableObjects.reserve(allObjects.size());
		for(auto* object : allObjects) {
			if(IsViewportSelectableObject(object)) {
				pickableObjects.push_back(object);
			}
		}

		auto hit = Raycastor::Raycast(ray, pickableObjects);
		if(hit) {
			return static_cast<SceneObject*>(hit->hitObject);
		}
		return nullptr;
	}

	void LevelEditor::UpdateViewportSelectionInput() {
		if(!debugViewport_ || !debugViewport_->IsShow()) return;

		const CalyxEngine::Vector2 origin = debugViewport_->GetPosition();
		const CalyxEngine::Vector2 size = debugViewport_->GetSize();
		const ImVec2 mouse = ImGui::GetMousePos();
		const CalyxEngine::Vector2 local{mouse.x - origin.x, mouse.y - origin.y};
		const bool inViewport = local.x >= 0.0f && local.y >= 0.0f && local.x <= size.x && local.y <= size.y;

		if(ImGui::IsMouseClicked(ImGuiMouseButton_Left) && inViewport) {
			rangeSelectCandidate_ = true;
			rangeSelecting_ = false;
			rangeSelectStart_ = local;
			rangeSelectEnd_ = local;
		}

		if(rangeSelectCandidate_ && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			rangeSelectEnd_ = local;
			const CalyxEngine::Vector2 delta = rangeSelectEnd_ - rangeSelectStart_;
			if(delta.LengthSquared() > 36.0f) {
				rangeSelecting_ = true;
			}
		}

		if(rangeSelectCandidate_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			if(rangeSelecting_) {
				SelectObjectsInViewportRect(rangeSelectStart_, rangeSelectEnd_, ImGui::GetIO().KeyCtrl);
			} else if(inViewport) {
				TryPickUnderCursor();
			}
			rangeSelectCandidate_ = false;
			rangeSelecting_ = false;
		}
	}

	bool LevelEditor::ProjectObjectToViewport(SceneObject* object, CalyxEngine::Vector2& outLocal) const {
		if(!object || !debugViewport_) return false;
		auto* camera = CameraManager::GetDebug();
		if(!camera) return false;

		const CalyxEngine::Vector3 worldPos = object->GetWorldTransform().GetWorldPosition();
		const CalyxEngine::Matrix4x4 viewProj = camera->GetViewProjectionMatrix();
		const CalyxEngine::Vector4 clip = CalyxEngine::Vector4::Transform(CalyxEngine::Vector4(worldPos, 1.0f), viewProj);
		if(std::abs(clip.w) <= 0.0001f) return false;

		const float ndcX = clip.x / clip.w;
		const float ndcY = clip.y / clip.w;
		const float ndcZ = clip.z / clip.w;
		if(ndcZ < 0.0f || ndcZ > 1.0f) return false;

		const CalyxEngine::Vector2 size = debugViewport_->GetSize();
		outLocal.x = (ndcX * 0.5f + 0.5f) * size.x;
		outLocal.y = (0.5f - ndcY * 0.5f) * size.y;
		return outLocal.x >= 0.0f && outLocal.y >= 0.0f && outLocal.x <= size.x && outLocal.y <= size.y;
	}

	void LevelEditor::SelectObjectsInViewportRect(const CalyxEngine::Vector2& startLocal,
												  const CalyxEngine::Vector2& endLocal,
												  bool append) {
		SceneContext* current = SceneContext::Current();
		if(!current || !current->GetObjectLibrary()) return;

		const float minX = (std::min)(startLocal.x, endLocal.x);
		const float minY = (std::min)(startLocal.y, endLocal.y);
		const float maxX = (std::max)(startLocal.x, endLocal.x);
		const float maxY = (std::max)(startLocal.y, endLocal.y);

		std::vector<std::shared_ptr<SceneObject>> selected;
		if(append) {
			selected.reserve(selectedObjects_.size());
			for(auto& weak : selectedObjects_) {
				if(auto sp = weak.lock()) {
					selected.push_back(sp);
				}
			}
		}

		for(auto& object : current->GetObjectLibrary()->GetAllObjectsShared()) {
			if(!IsViewportSelectableObject(object.get())) continue;

			CalyxEngine::Vector2 projected;
			if(!ProjectObjectToViewport(object.get(), projected)) continue;
			if(projected.x < minX || projected.x > maxX || projected.y < minY || projected.y > maxY) continue;
			if(std::find(selected.begin(), selected.end(), object) != selected.end()) continue;
			selected.push_back(object);
		}

		SetSelectedObjects(selected);
	}

	void LevelEditor::DrawViewportSelectionRect() const {
		if(!rangeSelecting_ || !debugViewport_) return;

		const CalyxEngine::Vector2 origin = debugViewport_->GetPosition();
		const ImVec2 a{origin.x + rangeSelectStart_.x, origin.y + rangeSelectStart_.y};
		const ImVec2 b{origin.x + rangeSelectEnd_.x, origin.y + rangeSelectEnd_.y};
		auto* drawList = ImGui::GetForegroundDrawList();
		drawList->AddRectFilled(a, b, IM_COL32(255, 160, 40, 45));
		drawList->AddRect(a, b, IM_COL32(255, 160, 40, 220), 0.0f, 0, 1.5f);
	}

	void LevelEditor::TryPickUnderCursor() {
		if(!debugViewport_ || !debugViewport_->IsShow()) return;

		SceneContext* current = SceneContext::Current();
		if(!current) return;

		CalyxEngine::Vector2 origin = debugViewport_->GetPosition();
		CalyxEngine::Vector2 size	  = debugViewport_->GetSize();

		ImVec2 mouse	 = ImGui::GetMousePos();
		float  relativeX = mouse.x - origin.x;
		float  relativeY = mouse.y - origin.y;

		if(relativeX < 0 || relativeY < 0 || relativeX > size.x || relativeY > size.y) return;

		// --- Pixel Shader Picking (Priority) ---
		if(sceneManager_) {
			if(auto* pickingPass = sceneManager_->GetPickingPass()) {
				// ビューポートサイズとテクスチャサイズの比率を計算して座標をスケーリング
				float scaleX = static_cast<float>(pickingPass->GetWidth()) / size.x;
				float scaleY = static_cast<float>(pickingPass->GetHeight()) / size.y;

				int32_t px = static_cast<int32_t>(relativeX * scaleX);
				int32_t py = static_cast<int32_t>(relativeY * scaleY);

				uint32_t pickingID = pickingPass->GetObjectID(px, py);
				if(pickingID > 0) {
					if(auto sp = current->GetObjectLibrary()->FindSharedByPickingID(pickingID)) {
						if(IsViewportSelectableObject(sp.get())) {
							const bool toggle = ImGui::GetIO().KeyCtrl;
							if(toggle) {
								ToggleSelectedObject(sp);
							} else {
								SetSelectedObject(sp);
							}
							return;
						}
					}
				}
			}
		}

		// --- Raycast Picking (Fallback) ---
		CalyxEngine::Vector2	 mousePos(relativeX, relativeY);
		CalyxEngine::Matrix4x4 view = CameraManager::GetDebug()->GetViewMatrix();
		CalyxEngine::Matrix4x4 proj = CameraManager::GetDebug()->GetProjectionMatrix();

		Ray ray = Raycastor::ConvertMouseToRay(mousePos, view, proj, size);
		if(SceneObject* picked = PickSceneObjectByRay(ray)) {
			if(auto sp = current->FindSharedObject(picked)) {
				const bool toggle = ImGui::GetIO().KeyCtrl;
				if(toggle) {
					ToggleSelectedObject(sp);
				} else {
					SetSelectedObject(sp);
				}
			}
		}
	}

	//=============================================================================
	// Scene Save
	//=============================================================================
	void LevelEditor::SaveScene() {
		SceneContext* ctx = SceneContext::Current();
		if(!ctx) return;

		std::string scenePath = "Resources/Assets/Scenes/" + ctx->GetSceneName() + ".scene";
		SceneSerializer::Save(*ctx, scenePath);
	}

	//=============================================================================
	// SceneContext の変更検出
	//=============================================================================
	void LevelEditor::SetSceneManager(CalyxEngine::SceneManager* manager) {
		sceneManager_ = manager;

		if(manager) {
			auto* pickingPass = manager->GetPickingPass();
			if(mainViewport_) mainViewport_->SetPickingPass(pickingPass);
			if(debugViewport_) debugViewport_->SetPickingPass(pickingPass);
			if(pickingViewport_) pickingViewport_->SetPickingPass(pickingPass);
		}
		if(manager && editToolMode_ == EngineEdit::EditToolMode::ParticleEffect) {
			manager->SetEditorPreviewContext(particlePreviewContext_.get());
		}

		if(sceneSwitchOverlay_) {
			sceneSwitchOverlay_->SetSceneManager(manager);
		}
	}
	void LevelEditor::NotifySceneContextChanged() {
		SceneContext* current = SceneContext::Current();
		if(prevCtx_ == current) return;

		if(hierarchy_) {
			hierarchy_->SetSceneObjectLibrary(current ? current->GetObjectLibrary() : nullptr);
			hierarchy_->RefreshCache();
		}

		ClearSelection();

		if(current) {
			current->AddOnObjectRemovedListener(
				[editor = this](SceneObject* removed) {
					if(!editor) return;

					if(editor->IsSelectedObject(removed)) {
						editor->ClearSelection();
					}
				});

			if(sceneEditor_) {
				sceneEditor_->BindRemovalCallback(current);
			}
		}
	}
	void LevelEditor::ClearSelection() {
		selectedObjects_.clear();
		selectedEditor_ = nullptr;

		std::weak_ptr<SceneObject> empty;

		hierarchy_->SetSelectedObject(empty);
		inspector_->SetSelectedObject(empty);
		sceneEditor_->ClearSelection();

		if(auto* ctx = SceneContext::Current()) {
			ctx->SetDebugSelectedObjects({});
		}
	}

} // namespace CalyxEngine

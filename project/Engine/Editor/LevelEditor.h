#pragma once
// engine
#include <Engine/Application/UI/EngineUI/DebugOverlay.h>
#include <Engine/Application/UI/EngineUI/EditorMenu.h>
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Application/UI/EngineUI/Manipulator.h>
#include <Engine/Application/UI/EngineUI/PerformanceOverlay.h>
#include <Engine/Application/UI/EngineUI/Viewport.h>
#include <Engine/Application/UI/Panels/AssetPanel.h>
#include <Engine/Application/UI/Panels/EditorPanel.h>
#include <Engine/Application/UI/Panels/HierarchyPanel.h>
#include <Engine/Application/UI/Panels/InspectorPanel.h>
#include <Engine/Application/UI/Panels/LivePPPanel.h>
#include <Engine/Application/UI/Panels/MaterialNodeEditorPanel.h>
#include <Engine/Application/UI/Panels/PlaceToolPanel.h>
#include <Engine/Application/UI/Panels/PostEffectNodeEditorPanel.h>
#include <Engine/Application/UI/Panels/SplineEditorPanel.h>
#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Editor/EditorSelectionCoordinator.h>
#include <Engine/Editor/ImGuiLayoutSwitcher.h>
#include <Engine/Editor/SceneObjectEditor.h>
#include <externals/nlohmann/json.hpp>

// c++
#include <memory>
#include <string>
#include <vector>

namespace EngineEdit {
	enum class EditorMode {
		Edit,
		Game
	};

	enum class EditToolMode {
		Object,
		Prefab,
		ParticleEffect,
		PostEffect,
		Material,
		Animation
	};
} // namespace EngineEdit

class BaseEditor;
class SceneContext;
class SceneObject;
class BaseCamera;
class WorldTransform;
struct CalyxEngine::Vector2;
struct CalyxEngine::Matrix4x4;
struct Ray;

namespace CalyxEngine {
	class SceneManager;
}

namespace CalyxEngine {

	class PlaySession;
	class SceneSwitchOverlay;
	class ImGuiLayoutSwitcher;

	/*-----------------------------------------------------------------------------------------
	 * LevelEditor
	 * - レベルエディタクラス
	 * - シーンオブジェクトの選択・配置・削除、ビューポート操作、シーン保存を統合管理
	 *---------------------------------------------------------------------------------------*/
	class LevelEditor {
	public:
		void Initialize();
		void Update();
		void Render();
		void RenderMenu();
		void RenderSettingsWindow();
		void RenderRuntimeFullscreenViewport(const ImTextureID& tex);
		void ClearSelection();
		~LevelEditor();
		CalyxEngine::SceneManager* GetSceneManager() const { return sceneManager_; }
		void					  SetSceneManager(CalyxEngine::SceneManager* sceneManager);

		// 編集対象 ----------------------------------------------------------------
		void SetSelectedEditor(BaseEditor* editor);
		/// SceneObject の選択（shared_ptr で受けて内部では weak_ptr で保持）
		void SetSelectedObject(const std::shared_ptr<SceneObject>& sp);
		void ToggleSelectedObject(const std::shared_ptr<SceneObject>& sp);
		void SetSelectedObjects(const std::vector<std::shared_ptr<SceneObject>>& objects);
		bool IsSelectedObject(const SceneObject* object) const;
		std::shared_ptr<SceneObject> GetPrimarySelectedObject() const;
		std::vector<std::shared_ptr<SceneObject>> GetSelectedObjects() const;

		/// シーンへのオブジェクト追加（Prefab / PlaceTool などから呼ばれる）
		void CreateObject(const std::shared_ptr<SceneObject>& obj);
		/// シーンからオブジェクト削除（階層パネルなどから呼ばれる）
		void DeleteObject(const std::shared_ptr<SceneObject>& sp);
		void DeleteSelectedObjects();
		std::vector<WorldTransform*> DuplicateSelectedObjects();

		// ビューポート関連 --------------------------------------------------------
		void RenderViewport(ViewportType type, const ImTextureID& tex);
		void SetCameraForViewport(BaseCamera* mainCamera, BaseCamera* debugCamera);

		// パネル取得 --------------------------------------------------------------
		HierarchyPanel* GetHierarchyPanel() const { return hierarchy_.get(); }
		EditorPanel*	GetEditorPanel() const { return editor_.get(); }
		PlaceToolPanel* GetPlaceToolPanel() const { return placeToolPanel_.get(); }

		EngineEdit::EditorMode GetMode() const { return mode_; }
		void				   SetPlaySession(PlaySession* session) { pPlaySesseion_ = session; }
		bool				   ShouldRenderRuntimeFullscreen() const;
		bool				   ShouldHideEditorUiInGameMode() const;

	private:
		// マウスピッキング関連 ----------------------------------------------------
		void		 TryPickUnderCursor();
		void		 UpdateViewportSelectionInput();
		void		 SelectObjectsInViewportRect(const CalyxEngine::Vector2& startLocal,
												 const CalyxEngine::Vector2& endLocal,
												 bool append);
		bool		 ProjectObjectToViewport(SceneObject* object, CalyxEngine::Vector2& outLocal) const;
		void		 DrawViewportSelectionRect() const;
		void		 TryPickObjectFromMouse(const CalyxEngine::Vector2&	mouse,
											const CalyxEngine::Vector2&	viewportSize,
											const CalyxEngine::Matrix4x4& view,
											const CalyxEngine::Matrix4x4& proj);
		SceneObject* PickSceneObjectByRay(const Ray& ray);

		// シーン管理 --------------------------------------------------------------
		void SaveScene();
		void NotifySceneContextChanged();
		void DrawSceneSelector();

		// モード切り替え ----------------------------------------------------------
		void EnterGameMode();
		void ExitGameMode();
		void ToggleMode();
		void DrawEditModeCombo();
		void ApplyEditToolMode(EngineEdit::EditToolMode mode, bool applyLayout);
		const char* GetEditToolModeName(EngineEdit::EditToolMode mode) const;
		std::string GetEditToolModeLayoutPath(EngineEdit::EditToolMode mode) const;
		std::string GetEditToolModeLoadLayoutPath(EngineEdit::EditToolMode mode) const;
		void EnsureParticlePreviewContext();
		void UpdateParticlePreviewContext(float dt);
		void EnsurePrefabEditContext();
		void NewPrefabEditContext(const std::string& rootTypeName = {});
		void OpenPrefabForEdit(const std::string& path);
		void SavePrefabEdit();
		void SavePrefabEditAs(const std::string& path);
		std::vector<SceneObject*> GetPrefabEditRoots() const;
		void NormalizePrefabEditRoots();
		void MarkPrefabEditorUtilityObjects();
		void ApplyPrefabOverridesFromInstance(const std::shared_ptr<SceneObject>& object);
		void UpdatePrefabEditContext(float dt);
		void SyncPrefabInstancesInCurrentScene(const Guid& prefabAssetGuid,
											   const std::string& prefabPath);

		void TogglePanel(IEngineUI* p) {
			if(p) p->SetShow(!p->IsShow());
		}

	private:
		// 管理UI
		std::unique_ptr<HierarchyPanel>		 hierarchy_;
		std::unique_ptr<EditorPanel>		 editor_;
		std::unique_ptr<InspectorPanel>		 inspector_;
		std::unique_ptr<SceneObjectEditor>	 sceneEditor_;
		std::unique_ptr<PlaceToolPanel>		 placeToolPanel_;
		std::unique_ptr<SplineEditorPanel>	 splineEditor_;
		std::unique_ptr<AssetPanel>			 assetPanel_;
		std::unique_ptr<MaterialNodeEditorPanel> materialNodeEditorPanel_;
		std::unique_ptr<PostEffectNodeEditorPanel> postEffectNodeEditorPanel_;
		std::unique_ptr<LivePPPanel>		 livePPPanel_;
		std::unique_ptr<SceneSwitchOverlay>	 sceneSwitchOverlay_;
		std::unique_ptr<ImGuiLayoutSwitcher> layoutSwitcher_;
		PlaySession*						 pPlaySesseion_ = nullptr;
		CalyxEngine::SceneManager*			 sceneManager_	= nullptr;

		// メニュー
		std::unique_ptr<EditorMenu> menu_; //< エディターメニュー
		EngineEdit::EditorMode		mode_ = EngineEdit::EditorMode::Edit;
		EngineEdit::EditToolMode	editToolMode_ = EngineEdit::EditToolMode::Object;

		// ビューポート
		std::unique_ptr<Viewport>			mainViewport_;		 //< メインビューポート
		std::unique_ptr<Viewport>			debugViewport_;		 //< デバッグビューポート
		std::unique_ptr<Viewport>			pickingViewport_;	 //< ピッキングビューポート
		int									startupDebugViewportFocusFrames_ = 3;
		std::unique_ptr<PerformanceOverlay> performanceOverlay_; //< パフォーマンスオーバーレイ
		std::unique_ptr<DebugOverlay>		debugOverlay_;		 //< デバッグオーバーレイ
		std::unique_ptr<SceneContext>		particlePreviewContext_;
		std::shared_ptr<CalyxEngine::FxObject> particlePreviewFx_;
		uint64_t							particlePreviewPlayedEmitterRevision_ = 0;
		std::unique_ptr<SceneContext>		prefabEditContext_;
		std::string							prefabEditPath_;
		bool								prefabEditDirty_ = false;

		// 状態
		bool		  lastPlaying_	  = false;
		SceneContext* prevCtx_		  = nullptr;
		EditorSelectionCoordinator selection_;
		nlohmann::json			   livePPSnapshot_;
		bool					   rangeSelectCandidate_ = false;
		bool					   rangeSelecting_ = false;
		CalyxEngine::Vector2	   rangeSelectStart_{};
		CalyxEngine::Vector2	   rangeSelectEnd_{};

		// Editors メニューに並べるパネル群
		std::vector<IEngineUI*> editorPanels_;
	};

} // namespace CalyxEngine

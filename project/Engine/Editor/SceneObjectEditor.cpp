#include "SceneObjectEditor.h"
/* ========================================================================
/*		include space
/* ===================================================================== */
// engine
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/graphics/Camera/Manager/CameraManager.h>

// externals
#include "externals/imgui/ImGuizmo.h"
#include <externals/imgui/imgui.h>


namespace CalyxEngine {
	SceneObjectEditor::SceneObjectEditor(const std::string& name) : BaseEditor(name) {
		manipulator_ = std::make_unique<Manipulator>();
	}

	SceneObjectEditor::SceneObjectEditor() : BaseEditor("sceneObjectEditor") {
		manipulator_ = std::make_unique<Manipulator>();
	}

	void SceneObjectEditor::SetTarget(SceneObject* object) {
		multiSelection_ = false;
		sceneObject_ = object;
		if(object) {
			manipulator_->SetTarget(&sceneObject_->GetWorldTransform());
		} else {
			manipulator_->SetTarget(nullptr);
		}
	}

	void SceneObjectEditor::SetTargets(const std::vector<SceneObject*>& objects) {
		std::vector<WorldTransform*> targets;
		targets.reserve(objects.size());
		for(auto* object : objects) {
			if(!object) continue;
			targets.push_back(&object->GetWorldTransform());
		}

		sceneObject_ = objects.empty() ? nullptr : objects.back();
		multiSelection_ = objects.size() > 1;
		manipulator_->SetTargets(targets);
	}

	void SceneObjectEditor::Update() {
		if(!sceneObject_) return;
	}

	void SceneObjectEditor::ShowImGuiInterface() {
		if(!sceneObject_) return;
		sceneObject_->ShowGui();
		// マニピュレーターの更新
		if(!multiSelection_) {
			manipulator_->SetTarget(&sceneObject_->GetWorldTransform());
		}
	}

	//====================================================================//
	//  SceneObjectEditor::ShowGuizmo
	//====================================================================//
	void SceneObjectEditor::BindRemovalCallback(SceneContext* ctx) {
		ctx->SetOnEditorObjectRemoved([this](SceneObject* removed) {
			if(sceneObject_ == removed) {
				ClearSelection(); // 明示的に無効化
			}
		});
	}

	void SceneObjectEditor::ClearSelection() {
		sceneObject_ = nullptr;
		multiSelection_ = false;
		manipulator_->SetTarget(nullptr);
	}
} // namespace CalyxEngine

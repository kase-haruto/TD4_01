#pragma once

#include <memory>
#include <vector>

class SceneObject;
class SceneContext;

namespace CalyxEngine {

	class BaseEditor;
	class HierarchyPanel;
	class InspectorPanel;
	class SceneObjectEditor;

	class EditorSelectionCoordinator {
	public:
		void Bind(HierarchyPanel* hierarchy,
				  InspectorPanel* inspector,
				  SceneObjectEditor* sceneEditor);

		void SetSelectedEditor(BaseEditor* editor);
		void SetSelectedObject(const std::shared_ptr<SceneObject>& object);
		void ToggleSelectedObject(const std::shared_ptr<SceneObject>& object);
		void SetSelectedObjects(const std::vector<std::shared_ptr<SceneObject>>& objects);
		void Clear();

		bool HasSelection() const;
		bool IsSelected(const SceneObject* object) const;
		std::shared_ptr<SceneObject> GetPrimarySelectedObject() const;
		std::vector<std::shared_ptr<SceneObject>> GetSelectedObjects() const;

	private:
		void SyncPanels(const std::vector<std::shared_ptr<SceneObject>>& validObjects);
		void SyncSceneContext(const std::vector<std::shared_ptr<SceneObject>>& validObjects);

		HierarchyPanel* hierarchy_ = nullptr;
		InspectorPanel* inspector_ = nullptr;
		SceneObjectEditor* sceneEditor_ = nullptr;
		BaseEditor* selectedEditor_ = nullptr;
		std::vector<std::weak_ptr<SceneObject>> selectedObjects_;
	};

} // namespace CalyxEngine

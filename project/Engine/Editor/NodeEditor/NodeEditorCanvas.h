#pragma once

#include <Engine\Editor\NodeEditor\NodeGraph.h>
#include <externals\imgui\imgui_node_editor.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

namespace CalyxEngine {
	class NodeEditorCanvas {
	public:
		using DrawNodeBody = std::function<bool(Node&)>;

		explicit NodeEditorCanvas(std::string id);
		~NodeEditorCanvas();

		NodeEditorCanvas(const NodeEditorCanvas&) = delete;
		NodeEditorCanvas& operator=(const NodeEditorCanvas&) = delete;

		bool Draw(NodeGraph& graph, const DrawNodeBody& drawBody = {});

	private:
		bool CanCreateLink(const NodeGraph& graph, int32_t a, int32_t b, int32_t& from, int32_t& to) const;
		void DrawPin(const NodePin& pin);

	private:
		std::string id_;
		ax::NodeEditor::EditorContext* context_ = nullptr;
		const NodeGraph* lastGraph_ = nullptr;
		std::unordered_set<int32_t> positionedNodes_;
	};
}

#include "NodeEditorCanvas.h"

#include <externals\imgui\imgui.h>

#include <algorithm>

namespace ed = ax::NodeEditor;

namespace CalyxEngine {
	NodeEditorCanvas::NodeEditorCanvas(std::string id) : id_(std::move(id)) {
		ed::Config config;
		config.SettingsFile = nullptr;
		context_ = ed::CreateEditor(&config);
	}

	NodeEditorCanvas::~NodeEditorCanvas() {
		if(context_) ed::DestroyEditor(context_);
	}

	void NodeEditorCanvas::DrawPin(const NodePin& pin) {
		const auto kind = pin.kind == NodePinKind::Input ? ed::PinKind::Input : ed::PinKind::Output;
		ed::BeginPin(ed::PinId(pin.id), kind);
		ImGui::TextUnformatted(pin.name.c_str());
		ed::EndPin();
	}

	bool NodeEditorCanvas::CanCreateLink(const NodeGraph& graph, int32_t a, int32_t b, int32_t& from, int32_t& to) const {
		const Node* nodeA = nullptr;
		const Node* nodeB = nullptr;
		const NodePin* pinA = graph.FindPin(a, &nodeA);
		const NodePin* pinB = graph.FindPin(b, &nodeB);
		if(!pinA || !pinB || !nodeA || !nodeB || nodeA == nodeB) return false;
		if(pinA->kind == pinB->kind || pinA->valueType != pinB->valueType) return false;
		from = pinA->kind == NodePinKind::Output ? pinA->id : pinB->id;
		to = pinA->kind == NodePinKind::Input ? pinA->id : pinB->id;
		return true;
	}

	bool NodeEditorCanvas::Draw(NodeGraph& graph, const DrawNodeBody& drawBody) {
		bool changed = false;
		if(lastGraph_ != &graph) {
			lastGraph_ = &graph;
			positionedNodes_.clear();
		}
		ed::SetCurrentEditor(context_);
		ed::Begin(id_.c_str());

		for(auto& node : graph.nodes) {
			ed::BeginNode(ed::NodeId(node.id));
			ImGui::TextUnformatted(node.title.c_str());
			ImGui::Separator();
			for(const auto& pin : node.inputs) DrawPin(pin);
			if(drawBody) changed |= drawBody(node);
			for(const auto& pin : node.outputs) DrawPin(pin);
			ed::EndNode();

			if(!positionedNodes_.contains(node.id)) {
				ed::SetNodePosition(ed::NodeId(node.id), ImVec2(node.position.x, node.position.y));
				positionedNodes_.insert(node.id);
			}
			ImVec2 pos = ed::GetNodePosition(ed::NodeId(node.id));
			if(pos.x != node.position.x || pos.y != node.position.y) {
				node.position = {pos.x, pos.y};
				changed = true;
			}
		}

		for(const auto& link : graph.links) {
			ed::Link(ed::LinkId(link.id), ed::PinId(link.fromPinId), ed::PinId(link.toPinId));
		}

		if(ed::BeginCreate()) {
			ed::PinId a, b;
			if(ed::QueryNewLink(&a, &b)) {
				int32_t from = 0;
				int32_t to = 0;
				if(CanCreateLink(graph, static_cast<int32_t>(a.Get()), static_cast<int32_t>(b.Get()), from, to)) {
					if(ed::AcceptNewItem()) {
						graph.links.push_back({graph.AllocateId(), from, to});
						changed = true;
					}
				} else {
					ed::RejectNewItem();
				}
			}
		}
		ed::EndCreate();

		if(ed::BeginDelete()) {
			ed::LinkId deletedLink;
			while(ed::QueryDeletedLink(&deletedLink)) {
				if(ed::AcceptDeletedItem()) {
					const int32_t id = static_cast<int32_t>(deletedLink.Get());
					std::erase_if(graph.links, [id](const NodeLink& l) { return l.id == id; });
					changed = true;
				}
			}
		}
		ed::EndDelete();

		ed::End();
		ed::SetCurrentEditor(nullptr);
		return changed;
	}
}

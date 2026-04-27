#include "NodeEditorCanvas.h"

#include <externals\imgui\imgui.h>

#include <algorithm>

namespace ed = ax::NodeEditor;

namespace CalyxEngine {
	namespace {
		constexpr float kNodeWidth = 190.0f;
		constexpr float kHeaderHeight = 26.0f;
		constexpr float kPinRadius = 4.5f;

		ImVec4 NodeAccentColor(const Node& node) {
			if(node.type == "Output") return ImVec4(0.18f, 0.29f, 0.39f, 1.0f);
			if(node.type == "Color" || node.type == "MultiplyColor") return ImVec4(0.30f, 0.34f, 0.22f, 1.0f);
			if(node.type == "LightingMode") return ImVec4(0.22f, 0.31f, 0.42f, 1.0f);
			if(node.type == "Reflect") return ImVec4(0.20f, 0.33f, 0.36f, 1.0f);
			return ImVec4(0.20f, 0.25f, 0.30f, 1.0f);
		}

		ImU32 ToU32(const ImVec4& color) {
			return ImGui::ColorConvertFloat4ToU32(color);
		}
	}

	NodeEditorCanvas::NodeEditorCanvas(std::string id) : id_(std::move(id)) {
		ed::Config config;
		config.SettingsFile = nullptr;
		config.DragButtonIndex = 0;
		config.SelectButtonIndex = 0;
		config.NavigateButtonIndex = 2;
		config.ContextMenuButtonIndex = 1;
		context_ = ed::CreateEditor(&config);
	}

	NodeEditorCanvas::~NodeEditorCanvas() {
		if(context_) ed::DestroyEditor(context_);
	}

	void NodeEditorCanvas::DrawPin(const NodePin& pin) {
		const auto kind = pin.kind == NodePinKind::Input ? ed::PinKind::Input : ed::PinKind::Output;
		const ImVec4 pinColor = GetPinColor(pin.valueType);
		ed::BeginPin(ed::PinId(pin.id), kind);
		ImGui::PushID(pin.id);
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if(pin.kind == NodePinKind::Input) {
			ImVec2 cursor = ImGui::GetCursorScreenPos();
			ImVec2 center(cursor.x + kPinRadius, cursor.y + kPinRadius + 4.0f);
			ImGui::InvisibleButton("pin", ImVec2(kPinRadius * 2.0f, kPinRadius * 2.0f + 4.0f));
			drawList->AddCircleFilled(center, kPinRadius, ToU32(pinColor), 16);
			drawList->AddCircle(center, kPinRadius, ToU32(ImVec4(0.03f, 0.03f, 0.035f, 1.0f)), 16, 1.0f);
			ImGui::SameLine(0.0f, 7.0f);
			ImGui::TextUnformatted(pin.name.c_str());
		} else {
			const float labelWidth = ImGui::CalcTextSize(pin.name.c_str()).x;
			ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), kNodeWidth - labelWidth - 23.0f));
			ImGui::TextUnformatted(pin.name.c_str());
			ImGui::SameLine(0.0f, 7.0f);
			ImVec2 cursor = ImGui::GetCursorScreenPos();
			ImVec2 center(cursor.x + kPinRadius, cursor.y + kPinRadius + 4.0f);
			ImGui::InvisibleButton("pin", ImVec2(kPinRadius * 2.0f, kPinRadius * 2.0f + 4.0f));
			drawList->AddCircleFilled(center, kPinRadius, ToU32(pinColor), 16);
			drawList->AddCircle(center, kPinRadius, ToU32(ImVec4(0.03f, 0.03f, 0.035f, 1.0f)), 16, 1.0f);
		}
		ImGui::PopID();
		ed::EndPin();
	}

	void NodeEditorCanvas::DrawNodeHeader(const Node& node) {
		const ImVec2 min = ImGui::GetCursorScreenPos();
		const ImVec2 size(kNodeWidth, kHeaderHeight);
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(min, ImVec2(min.x + size.x, min.y + size.y), ToU32(NodeAccentColor(node)), 6.0f, ImDrawFlags_RoundCornersTop);
		drawList->AddRectFilled(ImVec2(min.x, min.y + size.y - 1.0f), ImVec2(min.x + size.x, min.y + size.y), ToU32(ImVec4(1, 1, 1, 0.10f)));
		drawList->AddCircleFilled(ImVec2(min.x + 12.0f, min.y + 13.0f), 3.0f, ToU32(ImVec4(0.70f, 0.82f, 0.92f, 1.0f)), 12);
		drawList->AddText(ImVec2(min.x + 22.0f, min.y + 5.0f), ToU32(ImVec4(0.88f, 0.91f, 0.94f, 1.0f)), node.title.c_str());
		ImGui::Dummy(size);
		ImGui::Dummy(ImVec2(kNodeWidth, 4.0f));
	}

	ImVec4 NodeEditorCanvas::GetPinColor(NodeValueType type) const {
		switch(type) {
		case NodeValueType::Float:
			return ImVec4(0.25f, 0.58f, 1.0f, 1.0f);
		case NodeValueType::Color:
			return ImVec4(0.95f, 0.28f, 0.24f, 1.0f);
		case NodeValueType::Bool:
			return ImVec4(0.88f, 0.20f, 0.18f, 1.0f);
		case NodeValueType::Int:
			return ImVec4(0.34f, 0.86f, 0.42f, 1.0f);
		default:
			return ImVec4(0.82f, 0.86f, 0.90f, 1.0f);
		}
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

	bool NodeEditorCanvas::Draw(NodeGraph& graph, const DrawNodeBody& drawBody, const DrawContextMenu& drawContextMenu) {
		bool changed = false;
		backgroundContextRequested_ = false;
		nodeContextRequested_ = false;
		if(lastGraph_ != &graph) {
			lastGraph_ = &graph;
			positionedNodes_.clear();
		}
		ed::SetCurrentEditor(context_);
		ed::PushStyleColor(ed::StyleColor_Bg, ImVec4(0.095f, 0.100f, 0.108f, 1.0f));
		ed::PushStyleColor(ed::StyleColor_Grid, ImVec4(0.42f, 0.45f, 0.48f, 0.11f));
		ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(0.135f, 0.145f, 0.155f, 0.98f));
		ed::PushStyleColor(ed::StyleColor_NodeBorder, ImVec4(0.35f, 0.39f, 0.43f, 0.95f));
		ed::PushStyleColor(ed::StyleColor_HovNodeBorder, ImVec4(0.47f, 0.64f, 0.80f, 1.0f));
		ed::PushStyleColor(ed::StyleColor_SelNodeBorder, ImVec4(0.96f, 0.64f, 0.26f, 1.0f));
		ed::PushStyleColor(ed::StyleColor_LinkSelRectBorder, ImVec4(0.30f, 0.52f, 0.78f, 1.0f));
		ed::PushStyleColor(ed::StyleColor_PinRect, ImVec4(0.25f, 0.58f, 1.0f, 0.25f));
		ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(0.0f, 0.0f, 0.0f, 8.0f));
		ed::PushStyleVar(ed::StyleVar_NodeRounding, 5.0f);
		ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 1.0f);
		ed::PushStyleVar(ed::StyleVar_LinkStrength, 80.0f);
		ed::Begin(id_.c_str());

		for(auto& node : graph.nodes) {
			ed::BeginNode(ed::NodeId(node.id));
			DrawNodeHeader(node);
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
			ed::NodeId deletedNode;
			while(ed::QueryDeletedNode(&deletedNode)) {
				if(ed::AcceptDeletedItem()) {
					const int32_t nodeId = static_cast<int32_t>(deletedNode.Get());
					std::erase_if(graph.nodes, [nodeId](const Node& n) { return n.id == nodeId; });
					std::erase_if(graph.links, [&graph](const NodeLink& l) {
						return graph.FindPin(l.fromPinId) == nullptr || graph.FindPin(l.toPinId) == nullptr;
					});
					positionedNodes_.erase(nodeId);
					changed = true;
				}
			}

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

		ed::NodeId hoveredNode;
		if(ed::ShowNodeContextMenu(&hoveredNode)) {
			nodeContextRequested_ = true;
			contextNodeId_ = static_cast<int32_t>(hoveredNode.Get());
			activeContextMenu_ = {ContextMenuType::Node, {}, contextNodeId_};
			hasActiveContextMenu_ = true;
		}
		if(ed::ShowBackgroundContextMenu()) {
			backgroundContextRequested_ = true;
			ImVec2 pos = ed::ScreenToCanvas(ImGui::GetMousePos());
			contextCanvasPos_ = {pos.x, pos.y};
			activeContextMenu_ = {ContextMenuType::Background, contextCanvasPos_, 0};
			hasActiveContextMenu_ = true;
		}

		ed::End();

		if(drawContextMenu) {
			if(hasActiveContextMenu_) {
				ImGui::OpenPopup(activeContextMenu_.type == ContextMenuType::Background ? "NodeEditorBackgroundContextMenu" : "NodeEditorNodeContextMenu");
			}
			if(ImGui::BeginPopup("NodeEditorBackgroundContextMenu")) {
				activeContextMenu_.type = ContextMenuType::Background;
				changed |= drawContextMenu(activeContextMenu_);
				ImGui::EndPopup();
			}
			if(ImGui::BeginPopup("NodeEditorNodeContextMenu")) {
				activeContextMenu_.type = ContextMenuType::Node;
				changed |= drawContextMenu(activeContextMenu_);
				ImGui::EndPopup();
			}
			if(!ImGui::IsPopupOpen("NodeEditorBackgroundContextMenu") && !ImGui::IsPopupOpen("NodeEditorNodeContextMenu")) {
				hasActiveContextMenu_ = false;
			}
		}

		ed::PopStyleVar(4);
		ed::PopStyleColor(8);
		ed::SetCurrentEditor(nullptr);
		return changed;
	}

	bool NodeEditorCanvas::ConsumeBackgroundContextRequest(Vector2& outCanvasPos) {
		if(!backgroundContextRequested_) return false;
		backgroundContextRequested_ = false;
		outCanvasPos = contextCanvasPos_;
		return true;
	}

	bool NodeEditorCanvas::ConsumeNodeContextRequest(int32_t& outNodeId) {
		if(!nodeContextRequested_) return false;
		nodeContextRequested_ = false;
		outNodeId = contextNodeId_;
		return true;
	}
}

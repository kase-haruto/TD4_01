#include "DebugTextOverlay.h"
#include "DebugTextManager.h"

#include <algorithm>

namespace CalyxEngine {
//////////////////////////////////////////////////////////////////////////
///		コンストラクタ/デストラクタ
//////////////////////////////////////////////////////////////////////////
DebugTextOverlay::DebugTextOverlay() {
	BaseOnViewportTool::SetOverlayAlign(OverlayAlign::TopLeft);
	BaseOnViewportTool::SetOverlayOffset(ImVec2(0.0f, 0.0f));
}
DebugTextOverlay::~DebugTextOverlay() = default;

//////////////////////////////////////////////////////////////////////////
///		表示
//////////////////////////////////////////////////////////////////////////
void DebugTextOverlay::RenderOverlay(const ImVec2& basePos) {
	const auto& popups = DebugTextManager::GetPopupTexts();
	if(popups.empty()) return;

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if(!drawList) return;

	for(const auto& popup : popups) {
		const float lifetime = std::max(popup.lifetime, 0.001f);
		const float t		  = std::clamp(popup.age / lifetime, 0.0f, 1.0f);
		ImVec4	   color	  = popup.color;
		color.w *= (1.0f - t);

		const ImVec2 pos(
			basePos.x + popup.position.x,
			basePos.y + popup.position.y - popup.rise * t);

		drawList->AddText(pos, ImGui::ColorConvertFloat4ToU32(color), popup.text.c_str());
	}
}
} // namespace CalyxEngine

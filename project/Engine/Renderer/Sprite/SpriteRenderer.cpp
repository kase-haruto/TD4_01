#include "SpriteRenderer.h"
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/Graphics/Pipeline/Presets/PipelinePresets.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>

void SpriteRenderer::Register(Sprite* sprite){
	if (sprite){
		sprites_.push_back(sprite);
	}
}

void SpriteRenderer::Draw(ID3D12GraphicsCommandList* cmdList,
						  PipelineService* psoService,
						  RenderTargetType renderTarget,
						  bool clearAfterDraw){
	if (sprites_.empty()) return;

	bool hasTargetSprite = false;
	for(Sprite* sprite : sprites_) {
		if(sprite && sprite->GetTargetRt() == renderTarget) {
			hasTargetSprite = true;
			break;
		}
	}

	if (!hasTargetSprite){
		if(clearAfterDraw) Clear();
		return;
	}

	auto desc = PipelinePresets::MakeObject2D();
	psoService->SetCommand(desc, cmdList);

	for (Sprite* sprite : sprites_) {
		if(sprite && sprite->GetTargetRt() == renderTarget) {
			sprite->Draw(cmdList);
		}
	}

	if(clearAfterDraw) Clear();
}


void SpriteRenderer::Clear(){
	sprites_.clear();
}

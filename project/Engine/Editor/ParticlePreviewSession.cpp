#include "ParticlePreviewSession.h"

#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Graphics/Camera/3d/DebugCamera.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Scene/Context/SceneContext.h>

namespace CalyxEngine {

	void ParticlePreviewSession::Ensure() {
		if(context_) return;

		SceneContext* previous = SceneContext::Current();

		context_ = std::make_unique<SceneContext>();
		context_->Initialize(false);
		context_->SetSceneName("ParticleEffectPreview");

		fx_ = context_->Instantiate<CalyxEngine::FxObject>("ParticlePreview");
		fx_->SetTransient(true);
		fx_->SetEnablePicking(false);
		fx_->Initialize();
		playedEmitterRevision_ = fx_->GetEmitterRevision();

		if(auto* debugCamera = context_->GetCameraMgr()->GetDebug()) {
			debugCamera->GetWorldTransform().translation = {0.0f, 4.0f, -10.0f};
			debugCamera->GetWorldTransform().Update();
		}

		if(previous) {
			previous->MakeCurrent();
		}
	}

	void ParticlePreviewSession::Update(float dt) {
		if(!context_) return;

		SceneContext* previous = SceneContext::Current();
		context_->MakeCurrent();

		if(fx_) {
			const uint64_t emitterRevision = fx_->GetEmitterRevision();
			if(emitterRevision != playedEmitterRevision_) {
				fx_->PlayAll();
				playedEmitterRevision_ = emitterRevision;
			}
		}

		context_->Update(dt, dt, false);

		if(previous && previous != context_.get()) {
			previous->MakeCurrent();
		}
	}

} // namespace CalyxEngine

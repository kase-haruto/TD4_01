#pragma once

#include <cstdint>
#include <memory>

class SceneContext;

namespace CalyxEngine {

	class FxObject;

	class ParticlePreviewSession {
	public:
		void Ensure();
		void Update(float dt);

		SceneContext* Context() const { return context_.get(); }
		std::shared_ptr<FxObject> Object() const { return fx_; }

	private:
		std::unique_ptr<SceneContext> context_;
		std::shared_ptr<FxObject> fx_;
		uint64_t playedEmitterRevision_ = 0;
	};

} // namespace CalyxEngine

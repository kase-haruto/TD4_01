#pragma once

#include <Engine\Foundation\Reflection\CalyxReflection.h>
#include <Engine\Objects\Event\BaseEventObject.h>

CALYX_OBJECT(Category = Event, DisplayName = "PostEffectEvent")
class PostEffectEvent : public BaseEventObject {
public:
	PostEffectEvent();
	PostEffectEvent(const std::string& name);
	~PostEffectEvent() override = default;

	void Initialize() override;
	void AlwaysUpdate(float dt) override;

	std::string_view GetObjectClassName() const override {
		return "PostEffectEvent";
	}

protected:
	void DerivativeGui() override;
	void ApplyDerivedConfigFromJson(const nlohmann::json& root, const nlohmann::json* derived) override;
	void ExtractDerivedConfigToJson(nlohmann::json& root, nlohmann::json& derived) const override;

private:
	void ApplyPresetIfNeeded(bool force);
	std::string PostEffectLabel() const;

private:
	Guid postEffectGuid_;
	bool enabled_ = true;
	bool loaded_ = false;
	Guid loadedGuid_;
};

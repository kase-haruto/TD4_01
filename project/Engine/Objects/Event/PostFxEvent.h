#pragma once

#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <Engine/Objects/Event/BaseEventObject.h>

CALYX_OBJECT(Category = Event, DisplayName = "Post Fx Event")
class PostFxEvent : public BaseEventObject {
public:
	PostFxEvent();
	explicit PostFxEvent(const std::string& name);
	~PostFxEvent() override = default;

	void AlwaysUpdate(float dt) override;
	void DerivativeGui() override;
	void ApplyDerivedConfigFromJson(const nlohmann::json& root, const nlohmann::json* derived) override;
	void ExtractDerivedConfigToJson(nlohmann::json& root, nlohmann::json& derived) const override;
	std::string_view GetObjectClassName() const override { return "PostFxEvent"; }
	std::string GetObjectTypeName() const override { return name_; }

private:
	Guid postEffectGuid_{Guid::Empty()};
	bool enabled_ = true;
	int priority_ = 0;
};

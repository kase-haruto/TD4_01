#pragma once
#include "Engine/Objects/3D/Actor/Actor.h"
#include "Engine\Foundation\Serialization\SerializableObject.h"

class DemoHammer : public Actor {
public:
	DemoHammer();
	DemoHammer(const std::string&		  modelName,
			   std::optional<std::string> objectName);
	~DemoHammer() override = default;

public:
	void Initialize() override;
	void Update(float dt) override;

	//--------- ui/gui --------------------------------------------------

	void DerivativeGui() override;

	//--------- Collision -----------------------------------------------

	//--------- config ------------------------------------------------

	//--------- accessor ------------------------------------------------
	std::string_view GetObjectClassName() const override { return "DemoHammer"; }



private:
	struct HammerParameter : public CalyxEngine::SerializableObject {
		float moveSpeed = 2.0f;

		HammerParameter() {
			AddField("Move Speed", moveSpeed).Category("Move Param");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "DemoHammer", "Hammer"};
		}
	};

	HammerParameter param_;


};

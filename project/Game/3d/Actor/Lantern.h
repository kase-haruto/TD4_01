#pragma once

// engine
#include "Engine/Foundation/Reflection/CalyxReflection.h"
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Objects/LightObject/PointLightActor.h>

/*-----------------------------------------------------------------------------------------
 * Lantern
 * - 提灯
 * - 点灯させる
 *---------------------------------------------------------------------------------------*/
CALYX_OBJECT(Category = GameObject, DisplayName = "Lantern", Icon = "UI/Tool/Hierarchy/lightIcon.dds")
class Lantern
	: public PointLightActor{
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	Lantern();
	~Lantern() override;
	
	void DerivativeGui() override;
	void AlwaysUpdate(float dt) override;
	std::string_view GetObjectClassName() const override { return "Lantern"; }

private:
	//===================================================================*/
	//                    private methods / members
	//===================================================================*/

	struct LanternParam:CalyxEngine::SerializableObject {
		LanternParam() {
			AddField("Blink Interval", blinkInterval_).Range(0.1f, 5.0f).Tooltip("点灯のインターバル");
			AddField("Min Intensity", minIntensity_).Range(0.0f, 10.0f).Tooltip("点灯の最小光度");
			AddField("Max Intensity", maxIntensity_).Range(0.0f, 10.0f).Tooltip("点灯の最大光度");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return { CalyxEngine::ParamDomain::Game,
				"Lantern",
				"Actor/Background/Lantern" };
		}

		float blinkInterval_= 0.5f;//< インターバル
		float minIntensity_ = 0.0f; //< 最小光度
		float maxIntensity_ = 1.0f; //< 最大光度
	} param_;
	
	float blinkTimer_ = 0.0f;	//< 点灯タイマー
	bool isIncreasing_ = true;
};

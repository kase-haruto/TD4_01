#include "Lantern.h"
#include <Engine/Objects/LightObject/PointLight.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(Lantern);

Lantern::Lantern():PointLightActor("debugCube","Lantern") {
	param_.LoadParams();
}

Lantern::~Lantern() {
}
void Lantern::DerivativeGui() {
	PointLightActor::DerivativeGui();
	param_.ShowGui();
}
///////////////////////////////////////////////////////t//////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void Lantern::AlwaysUpdate(float dt) {

	// 点灯させる
	blinkTimer_ += dt;
	if(param_.blinkInterval_ > 0.0f && blinkTimer_ >= param_.blinkInterval_) {
		blinkTimer_ -= param_.blinkInterval_;
		isIncreasing_ = !isIncreasing_;
	}
	
	if(GetPointLight() != nullptr && param_.blinkInterval_ > 0.0f) {
		float t = blinkTimer_ / param_.blinkInterval_;
		if(!isIncreasing_) {
			t = 1.0f - t;
		}
		float intensity = param_.minIntensity_ + (param_.maxIntensity_ - param_.minIntensity_) * t;
		SetLightIntensity(intensity);
	}
	
	PointLightActor::AlwaysUpdate(dt);
}

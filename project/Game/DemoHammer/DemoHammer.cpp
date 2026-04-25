#include "DemoHammer.h"
#include "Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h"

REGISTER_SCENE_OBJECT(DemoHammer)

DemoHammer::DemoHammer() : Actor() {
}

DemoHammer::DemoHammer(const std::string& modelName, std::optional<std::string> objectName) :
Actor::Actor(modelName, objectName) {
	param_.LoadParams();
}

void DemoHammer::Initialize() {
	param_.LoadParams();
}

void DemoHammer::Update(float dt) {
	float t = dt;
	t		= 1.0f;
}

void DemoHammer::DerivativeGui() {
	param_.ShowGui();
}

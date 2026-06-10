#include "DemoHammer.h"
#include "Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h"

REGISTER_SCENE_OBJECT(DemoHammer)

DemoHammer::DemoHammer() : Actor() {
}

DemoHammer::DemoHammer(const std::string& modelName, std::optional<std::string> objectName) :
Actor::Actor(modelName, objectName) {
	param_.LoadParams();
	worldTransform_.inheritRotate = false;
}

void DemoHammer::Initialize() {
	param_.LoadParams();
	swingAxis_	= {1.0f, 0.0f, 0.0f};
	swingAngle_ = 0.0f;

}

void DemoHammer::Update([[maybe_unused]] float dt) {
	using CalyxEngine::Quaternion;

	// inheritRotate=false で親の回転継承を切ったので、親モデル(DemoPlayer)の回転を手動で合成する。
	// 合成順: base(初期姿勢) → ownerRotation_(親モデルの回転) → swing(スイング・最外)
	// inheritRotate=false なので worldRot = local がそのままワールド回転になる
	Quaternion swing		 = Quaternion::MakeRotateAxisQuaternion(swingAxis_, swingAngle_);
	worldTransform_.rotation = swing * ownerRotation_ * baseRotation_;
}

void DemoHammer::DerivativeGui() {
	param_.ShowGui();
}

#pragma once
#include "Engine\Objects\3D\Actor\Actor.h"
#include "Engine\Foundation\Serialization\SerializableObject.h"

class DemoPlayer :
public Actor {
public:
	DemoPlayer();
	DemoPlayer(const std::string&		  modelName,
			   std::optional<std::string> objectName);
	~DemoPlayer() override;

public:
	void Initialize() override;
	void Update(float dt) override;

	//--------- ui/gui --------------------------------------------------

	void DerivativeGui() override;

	//--------- Collision -----------------------------------------------

	//--------- config ------------------------------------------------

	//--------- accessor ------------------------------------------------
	std::string_view GetObjectClassName() const override { return "DemoPlayer"; }
private:
	void Move(float dt);
	void ApplyGravity(float dt);
	void UpdatePopScale(float dt);

private:

	struct PlayerParameter : public CalyxEngine::SerializableObject {
		float moveSpeed = 2.0f;
		float jumpForce = 15.0f;
		float diveForce = -30.0f;
		float gravity	= 40.0f;

		float stiffness = 400.0f;
		float damping	= 20.0f;

		float diveRotationTime = 0.5f;

		CalyxEngine::Vector3 jumpScale = {0.8f, 1.3f, 0.8f};
		CalyxEngine::Vector3 diveScale = {0.9f, 1.2f, 0.9f};
		CalyxEngine::Vector3 landScale = {1.4f, 0.5f, 1.4f};

		PlayerParameter() {
			AddField("Move Speed", moveSpeed).Category("Move Param");
			AddField("Jump Force", jumpForce).Category("Move Param");
			AddField("Dive Force", diveForce).Category("Move Param");
			AddField("Gravity", gravity).Category("Move Param");
			AddField("Stiffness", stiffness).Category("Pop Scale");
			AddField("Damping", damping).Category("Pop Scale");
			AddField("Dive Rotation Time", diveRotationTime).Category("Move Param");
			AddField("Jump Scale", jumpScale).Category("Pop Scale");
			AddField("Dive Scale", diveScale).Category("Pop Scale");
			AddField("Land Scale", landScale).Category("Pop Scale");
		}
		
		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "DemoPlayer", "Player"};
		}
	};

	PlayerParameter param_;
	bool  isJumping_	 = false;
	bool  isDiving_		 = false;

	// 回転補間用
	float rotationSpeed_ = 10.0f;
	CalyxEngine::Quaternion baseRotation_ = CalyxEngine::Quaternion::MakeIdentity();

	// ジャンプ回転用
	float jumpRotation_			= 0.0f; // 現在の回転角
	float jumpRotationSpeed_	= 0.0f; // 現在の回転速度
	float jumpRotationRemaining_ = 0.0f; // 残りの回転量

	// Pop Scale
	CalyxEngine::Vector3 targetScale_ = {1.0f, 1.0f, 1.0f};
	CalyxEngine::Vector3 scaleVelocity_ = {0.0f, 0.0f, 0.0f};
};
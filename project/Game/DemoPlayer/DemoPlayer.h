#pragma once
#include "Engine\Objects\3D\Actor\Actor.h"

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

	//--------- Collision -----------------------------------------------

	//--------- config ------------------------------------------------

	//--------- accessor ------------------------------------------------
	std::string_view GetObjectClassName() const override { return "DemoPlayer"; }
private:
	void Move(float dt);
	void ApplyGravity(float dt);

private:
	float jumpForce_	 = 20.0f;
	float diveForce_	 = -40.0f;
	float gravity_		 = 40.0f;
	bool  isJumping_	 = false;
	bool  isDiving_		 = false;

	// 回転補間用
	float rotationSpeed_ = 10.0f;
	CalyxEngine::Quaternion baseRotation_ = CalyxEngine::Quaternion::MakeIdentity();

	// ジャンプ回転用
	float jumpRotation_			= 0.0f; // 現在の回転角
	float jumpRotationSpeed_	= 0.0f; // 現在の回転速度
	float jumpRotationRemaining_ = 0.0f; // 残りの回転量
};
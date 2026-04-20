#pragma once
#include "Engine\Objects\3D\Actor\Actor.h"

class DemoPlayer : public Actor {
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

private:
	void Move(float dt);
	void ApplyGravity(float dt);

private:
	float jumpForce_ = 20.0f;
	float diveForce_ = -40.0f;
	float gravity_	 = 40.0f;
	bool  isJumping_ = false;
};
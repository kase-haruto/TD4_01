#pragma once


class Stage {
public:
	Stage() = default;
	~Stage() = default;

	void Initialize(float hp,float limitTime);

	void Update(float dt);

	void TakeDamage(float damage);

private:

private:

	float stageHP_ = 0.0f;
	float timer_   = 0.0f;

};

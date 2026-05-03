#pragma once


class Stage {
public:
	Stage() = default;
	~Stage() = default;

	void Update();

	void TakeDamage(float damage);

private:

private:

	float stageHP_ = 0.0f;
	float timer_   = 0.0f;

};

#pragma once
#include "Engine/Objects/3D/Actor/Actor.h"
#include "Engine\Foundation\Serialization\SerializableObject.h"
#include <vector>

class Shockwave : public Actor {
public:
	Shockwave();
	Shockwave(const std::string&		 modelName,
			  std::optional<std::string> objectName);
	~Shockwave() override = default;

public:
	void Initialize() override;
	void Update(float dt) override;

	/// <summary> 衝撃波を発生させる </summary>
	/// <param name="pos"> 位置 </param>
	/// <param name="scaleMultiplier"> 拡大の倍率 </param>
	void Activate(const CalyxEngine::Vector3& pos, float scaleMultiplier, bool strong);

	/// <summary> 停止処理 </summary>
	void Deactivate();

	//--------- ui/gui --------------------------------------------------

	void DerivativeGui() override;

	//--------- Collision -----------------------------------------------

	void OnCollisionEnter(Collider* other) override;

	//--------- accessor ------------------------------------------------
	std::string_view GetObjectClassName() const override { return "Shockwave"; }
	float			 GetPushForce() const { return param_.pushForce * scaleMultiplier_; }
	bool			 IsActive() const { return isActive_; }
	bool			 IsStrong() const { return isStrong_; }

	// 再利用可能か（非アクティブ かつ 未発行のダメージが残っていない）
	bool			 IsReusable() const {
		return !isActive_ && pendingDamages_.empty() && readyStageDamage_ == 0;
	}

	//確定したステージダメージを取り出して消費する
	int				 ConsumeStageDamage();

private:
	struct ShockwaveParameter : public CalyxEngine::SerializableObject {
		float lifeTime	 = 0.5f;
		float startScale = 1.0f;
		float endScale	 = 4.0f;
		float pushForce	 = 20.0f; // 跳ね返す力

		ShockwaveParameter() {
			AddField("Life Time", lifeTime);
			AddField("Start Scale", startScale);
			AddField("End Scale", endScale);
			AddField("Push Force", pushForce);
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "Shockwave", "Shockwave"};
		}
	};

	// 確定待ちのダメージ（ヒットごとに1件積む。timer_が0以下で確定）
	struct PendingDamage {
		float timer	 = 0.0f; // 確定までの残り時間
		int	  amount = 1;	 // ダメージ量
	};

	ShockwaveParameter		   param_;
	float					   scaleMultiplier_	 = 1.0f;
	float					   timer_			 = 0.0f;
	float					   currentMaxScale_	 = 5.0f;
	std::vector<PendingDamage> pendingDamages_;
	int						   readyStageDamage_ = 0;
	bool					   isActive_		 = false;
	bool					   isStrong_		 = false;
};

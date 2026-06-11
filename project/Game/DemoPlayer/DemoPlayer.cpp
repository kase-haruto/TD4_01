#include "DemoPlayer.h"

#include "Engine/Assets/Manager/AssetManager.h"
#include "Engine/Foundation/Input/Input.h"
#include "Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h"
#include "Engine/Scene/Utility/SceneUtility.h"
#include "Engine/Objects/Collider/BoxCollider.h"
#include "Engine/PostProcess/Manager/PostEffectManager.h"

#include <Game/DemoShockwave/ShockwaveManager.h>
#include <Game/StageGimmick/Tutorial/TutorialEvent.h>
#include <algorithm>
#include <cmath>
#include <numbers>

REGISTER_SCENE_OBJECT(DemoPlayer)

DemoPlayer::DemoPlayer() : Actor() {
	param_.LoadParams();
	moveSpeed_ = param_.moveSpeed;
}

DemoPlayer::DemoPlayer(const std::string& modelName, std::optional<std::string> objectName) : Actor::Actor(modelName, objectName) {
	param_.LoadParams();
	moveSpeed_ = param_.moveSpeed;
}

DemoPlayer::~DemoPlayer() = default;

void DemoPlayer::Initialize() {
	param_.LoadParams();
	moveSpeed_					= param_.moveSpeed;
	life_						= param_.playerHP;
	worldTransform_.translation = {0.0f, 0.0f, 0.0f};
	worldTransform_.scale		= {1.0f, 1.0f, 1.0f};
	velocity_					= {0.0f, 0.0f, 0.0f};
	isJumping_					= false;
	isDiving_					= false;
	isDivingInvincible_			= false;
	jumpInvincibleTimer_		= 0.0f;
	activeTutorial_				= nullptr;
	isRecovering_				= false;
	recoveryTimer_				= 0.0f;
	isTutorialClear_			= false;

	damageFlashTimer_ = 0.0f;
	isInvincible_	  = false;
	SetDrawEnable(true);

	baseRotation_				= CalyxEngine::Quaternion::MakeIdentity();
	jumpRotation_				= 0.0f;
	jumpRotationSpeed_			= 0.0f;
	jumpRotationRemaining_		= 0.0f;
	pitfallRecoveryState_		= PitfallRecoveryState::None;
	pitfallReturnY_				= 0.0f;

	// Pop Scale
	targetScale_   = {1.0f, 1.0f, 1.0f};
	scaleVelocity_ = {0.0f, 0.0f, 0.0f};

	hammerTargetScale_	 = {1.0f, 1.0f, 1.0f};
	hammerScaleVelocity_ = {0.0f, 0.0f, 0.0f};

	InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		// 敵、イベントオブジェクト、ステージギミックを対象にする
		collider_->SetTargetType(ColliderType::Type_Enemy | ColliderType::Type_EnemyAttack |
								 ColliderType::Type_EventObject | ColliderType::Type_StageGimmick | ColliderType::Type_Impediment);
		collider_->SetOffset(param_.colliderOffset);
		if(auto* radius = dynamic_cast<BoxCollider*>(collider_.get())) {
			radius->SetSize(param_.colliderSize);
		}
	}

	// 衝撃波マネージャーの初期化（プール作成）
	ShockwaveManager::GetInstance()->Initialize(10);

	effectData_.Load("PlayerShockwave");
	effectStrongData_.Load("PlayerShockwaveStrong");
	rollingEffect_.Load("PlayerRolling");
	jumpEffect_.Load("PlayerJumpEffect");

	AudioAPI::Load(hitAudio_, "hit");
	AudioAPI::Load(shockwaveAudio_, "shockwave");
	AudioAPI::Load(damageAudio_, "damage");
}

void DemoPlayer::Update(float dt) {
	if(firstSetting_) {
		if(collider_) {
			if(!collider_->GetOwner()) {
				collider_->SetOwner(this);
			}
		}
		// 子オブジェクトからハンマーを探す
		hammer_ = nullptr;
		for(auto& child : children_) {
			if(auto h = std::dynamic_pointer_cast<DemoHammer>(child)) {
				hammer_ = h;
				break;
			}
		}

		if(hammer_) {
			// 既存の親子付け（通常の親子関係）を解除してからボーンにペアレントする
			hammer_->SetParent(nullptr);

			// ハンマーのトランスフォームをボーンへ追従させる
			this->SetBoneParent(
				hammer_->GetWorldTransform(),//< このトランスフォーム
				"Bone.011",					 //< ボーン名
				false						 //<親のスケールを継承するか
			);

			// 本来のサイズを保持
			hammerInitialScale_ = hammer_->GetWorldTransform().scale;
			hammerTargetScale_	= hammerInitialScale_;

			hammer_->SetTranslate({-0.24f, 0.23f, 0.0f});
			// 初期姿勢はスイングのベースとして渡す（DemoHammer 側で毎フレーム合成される）
			// hammer_->SetBaseRotation(CalyxEngine::Quaternion::EulerToQuaternion({-0.6f, 0.1f, -1.6f}));
			hammer_->SetBaseRotation(CalyxEngine::Quaternion::EulerToQuaternion({0.278f, 0.1f, 0.119f}));
			firstSetting_ = false;
			hammer_->GetWorldTransform().inheritRotate = false;
		}
	}

	DamageFlash(dt);

	if(jumpInvincibleTimer_ > 0.0f) {
		jumpInvincibleTimer_ -= dt;
		if(jumpInvincibleTimer_ < 0.0f) {
			jumpInvincibleTimer_ = 0.0f;
		}
	}

	moveSpeed_ = param_.moveSpeed;

	if(pitfallRecoveryState_ != PitfallRecoveryState::None) {
		UpdatePitfallRecovery(dt);
		UpdatePopScale(dt);
		HammerControl(dt);
		if(collider_) {
			collider_->Update(worldTransform_.translation, worldTransform_.rotation);
		}
		return;
	}

	Move(dt);
	LandingEvents landingEvents = ApplyGravity(dt);
	if(landingEvents.hardLanded) {
		//強い衝撃時の処理
		EffectAPI::Play(effectStrongData_, worldTransform_.GetWorldPosition());
		EffectAPI::Play(effectData_, worldTransform_.GetWorldPosition());
		ShockwaveManager::GetInstance()->Emit(worldTransform_.GetWorldPosition(), param_.strongShockScale, true);
		//カメラシェイク
		CameraManager::GetMain3d()->StartShake(param_.shakeParm.duration, param_.shakeParm.intensity);
		PostEffectManager::Get()->PlayTriggeredEffect("PlayerShock");
		//EffectAPI::PlayFromName("shockwave", worldTransform_.GetWorldPosition());
		AudioAPI::Play(shockwaveAudio_, false, 0.5f);
	}
	UpdatePopScale(dt);
	HammerControl(dt);

	if(collider_) {
		collider_->Update(worldTransform_.translation, worldTransform_.rotation);
	}
	
}

void DemoPlayer::TakeDamage(int32_t damage) {
	if(isInvincible_) {
		return;
	}
	if(isDivingInvincible_) {
		return;
	}
	if(jumpInvincibleTimer_ > 0.0f) {
		return;
	}

	life_ -= damage;
	if(life_ <= 0) {
		life_	 = 0;
		isAlive_ = false;
	}

	damageFlashTimer_ = param_.damageFlashDuration;
	isInvincible_	  = true;
	AudioAPI::Play(damageAudio_, false, 0.5f);
}

void DemoPlayer::DerivativeGui() {
	param_.ShowGui();
}

void DemoPlayer::OnCollisionEnter(Collider* other) {
	if(other->GetType() == ColliderType::Type_Enemy || other->GetType() == ColliderType::Type_EnemyAttack || other->GetType() == ColliderType::Type_Impediment) {
		TakeDamage(1);
	}

	BaseGameObject* otherObj = other->GetOwner();
	if(!otherObj) {
		return;
	}
}

void DemoPlayer::StartPitfallRecovery() {
	if(pitfallRecoveryState_ != PitfallRecoveryState::None) {
		return;
	}

	pitfallRecoveryState_  = PitfallRecoveryState::Falling;
	pitfallReturnY_		   = worldTransform_.translation.y;
	velocity_			   = {0.0f, -param_.pitfallFallSpeed, 0.0f};
	isJumping_			   = false;
	isDiving_			   = false;
	isDivingInvincible_	   = false;
	jumpInvincibleTimer_   = 0.0f;
	isRecovering_		   = false;
	jumpRotation_		   = 0.0f;
	jumpRotationSpeed_	   = 0.0f;
	jumpRotationRemaining_ = 0.0f;
	worldTransform_.scale  = param_.diveScale;
	scaleVelocity_		   = {0.0f, 0.0f, 0.0f};
}

void DemoPlayer::UpdatePitfallRecovery(float dt) {
	if(pitfallRecoveryState_ == PitfallRecoveryState::Falling) {
		worldTransform_.translation.y -= param_.pitfallFallSpeed * dt;
		if(worldTransform_.translation.y <= param_.pitfallBottomY) {
			worldTransform_.translation.y = param_.pitfallBottomY;
			pitfallRecoveryState_		  = PitfallRecoveryState::JumpingBack;
			// 元のY座標より少し上を頂点にする初速を計算し、重力で自然な放物線を描いて戻す。
			float apexY			  = pitfallReturnY_ + (std::max)(0.0f, param_.pitfallJumpHeight);
			float jumpHeight	  = (std::max)(0.0f, apexY - worldTransform_.translation.y);
			velocity_			  = {0.0f, std::sqrt(2.0f * param_.gravity * jumpHeight), 0.0f};
			isJumping_			  = true;
			worldTransform_.scale = param_.jumpScale;
			scaleVelocity_		  = {0.0f, 0.0f, 0.0f};
		}
		return;
	}

	if(pitfallRecoveryState_ == PitfallRecoveryState::JumpingBack) {
		worldTransform_.translation.y += velocity_.y * dt;
		velocity_.y -= param_.gravity * dt;
		if(velocity_.y <= 0.0f && worldTransform_.translation.y <= pitfallReturnY_) {
			worldTransform_.translation.y = pitfallReturnY_;
			velocity_					  = {0.0f, 0.0f, 0.0f};
			pitfallRecoveryState_		  = PitfallRecoveryState::None;
			isJumping_					  = false;
			worldTransform_.scale		  = param_.landScale;
			scaleVelocity_				  = {0.0f, 0.0f, 0.0f};
		}
	}
}

void DemoPlayer::Move(float dt) {
	// 水平移動の入力
	CalyxEngine::Vector3 horizonVelocity = {0.0f, 0.0f, 0.0f};

	// キーボード入力
	if(CalyxFoundation::Input::PushKey(DIK_A)) {horizonVelocity.x -= 1.0f;}
	if(CalyxFoundation::Input::PushKey(DIK_D)) {horizonVelocity.x += 1.0f;}

	// ゲームパッド：スティック入力
	CalyxEngine::Vector2 leftStick = CalyxFoundation::Input::GetInstance()->GetLeftStick();
	horizonVelocity.x += leftStick.x;

	// ゲームパッド：十字ボタン入力
	if(CalyxFoundation::Input::PushGamepadButton(CalyxFoundation::PadButton::DPAD_LEFT)) {horizonVelocity.x -= 1.0f;}
	if(CalyxFoundation::Input::PushGamepadButton(CalyxFoundation::PadButton::DPAD_RIGHT)) {horizonVelocity.x += 1.0f;}

	if(isJumping_) {
		horizonVelocity.x = 0.0f;
	}

	// 斜め移動などで速くならないように最大1.0に制限
	if(horizonVelocity.Length() > 1.0f) {
		horizonVelocity.Normalize();
	}

	CalyxEngine::Vector3 horizonRotateV = horizonVelocity;
	horizonRotateV.z += 1.0f;
	if(horizonRotateV.Length() > 0.0f) {
		horizonRotateV.Normalize();

		// 回転（移動方向を向く目標）
		CalyxEngine::Vector3	from		   = CalyxEngine::Vector3::Forward();
		CalyxEngine::Vector3	to			   = horizonRotateV;
		CalyxEngine::Quaternion targetRotation = CalyxEngine::Quaternion::FromToQuaternion(from, to);

		// 線形補間(SLERP)による滑らかな回転
		baseRotation_ = CalyxEngine::Quaternion::Slerp(baseRotation_, targetRotation, rotationSpeed_ * dt);
		if(!isJumping_) {
			preRotate_ = baseRotation_;
		} else {
			baseRotation_ = preRotate_;
		}
	}

	JumpEvents jumpEvents = HandleJump(dt);
	if(jumpEvents.jumpStart) {
		// ジャンプ時の処理
		EffectAPI::Play(effectData_, worldTransform_.GetWorldPosition());
		auto offset = CalyxEngine::Vector3{0.0f, 0.5f, 1.0f};
		EffectAPI::Play(jumpEffect_, worldTransform_.GetWorldPosition() + offset);
		ShockwaveManager::GetInstance()->Emit(worldTransform_.GetWorldPosition(), param_.defaultShockScale);
		AudioAPI::Play(hitAudio_, false, 0.5f);
	}

	// 速度の更新（水平成分のみ上書き、垂直成分は維持）
	velocity_.x = horizonVelocity.x * moveSpeed_;
	velocity_.z = horizonVelocity.z * moveSpeed_;


	// 最終的な回転を適用 (向き + ジャンプ回転 + 傾き)
	CalyxEngine::Quaternion flipRotation = CalyxEngine::Quaternion::MakeRotateX(jumpRotation_);
	worldTransform_.rotation			 = baseRotation_ * flipRotation;

	worldTransform_.translation += velocity_ * dt;
	if(worldTransform_.translation.x < -param_.limit) {
		worldTransform_.translation.x = -param_.limit;
	} else if(worldTransform_.translation.x > param_.limit) {
		worldTransform_.translation.x = param_.limit;
	}
}

DemoPlayer::JumpEvents DemoPlayer::HandleJump(float dt) {
	JumpEvents events{};
	// ジャンプ入力
	bool jumpTrigger = CalyxFoundation::Input::TriggerKey(DIK_SPACE) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::A);

	// チュートリアルの連打対策中はジャンプ入力を無視する
	// （Diveの2回目停止時もTutorialEvent側でブロックされるため自動で対応）
	if(activeTutorial_ && activeTutorial_->IsInputBlocking()) {
		jumpTrigger = false;
	}

	float pi = std::numbers::pi_v<float>;
	if(jumpTrigger && !isRecovering_) {
		if(!isJumping_) {
			// ジャンプ開始
			velocity_.y	  = param_.jumpForce;
			isJumping_	  = true;
			isDiving_	  = false;
			isDivingInvincible_ = false;
			jumpInvincibleTimer_ = param_.jumpInvincibleTime;
			jumpRotation_ = 0.0f;
			// 接地状態からジャンプして着地するまでの時間を計算
			float airTime		   = 2.0f * param_.jumpForce / param_.gravity;
			jumpRotationSpeed_	   = (2.0f * pi) / airTime;
			jumpRotationRemaining_ = 2.0f * pi;

			events.jumpStart = true;

			// ジャンプ時に縦に伸びる
			worldTransform_.scale = param_.jumpScale;
			scaleVelocity_		  = {0.0f, 0.0f, 0.0f};

			// ハンマーのサイズ（降り始めに最大化）
			if(hammer_) {
				hammer_->GetWorldTransform().scale.x = hammerInitialScale_.x * param_.hammerSwingScale.x;
				hammer_->GetWorldTransform().scale.y = hammerInitialScale_.y * param_.hammerSwingScale.y;
				hammer_->GetWorldTransform().scale.z = hammerInitialScale_.z * param_.hammerSwingScale.z;
				hammerScaleVelocity_				 = {0.0f, 0.0f, 0.0f};
			}
		} else if(!isDiving_ && velocity_.y >= -10.0f) {
			velocity_.y			= param_.jumpForce;
			isDiving_			= true;
			auto offset			= CalyxEngine::Vector3{0.0f, 0.5f, 1.0f};
			rollingFxHandle_	= EffectAPI::Play(rollingEffect_, worldTransform_.GetWorldPosition() + offset);
			isDivingInvincible_ = true;
			// 2回追加で回る
			jumpRotationSpeed_	   = (4.0f * pi) / param_.diveRotationTime;
			jumpRotationRemaining_ = 4.0f * pi;

			events.diveStart = true;
		}
	}

	// ジャンプ回転の更新
	if(isJumping_) {
		if(!isDiving_) {
			// 通常ジャンプの場合、残りの滞空時間に合わせて回転速度をに調整
			bool isJumpPush =
				CalyxFoundation::Input::PushKey(DIK_SPACE) || CalyxFoundation::Input::PushGamepadButton(CalyxFoundation::PadButton::A);
			float currentGravity = param_.gravity;
			if(!isJumpPush && velocity_.y > 0.0f) {
				currentGravity *= param_.variableJumpMultiplier;
			}

			float remainingTime = 0.0f;
			if(velocity_.y > 0.0f) {
				// 頂点までの時間 + 頂点から地面までの時間
				float tToApex	 = velocity_.y / currentGravity;
				float apexHeight = worldTransform_.translation.y + (velocity_.y * velocity_.y) / (2.0f * currentGravity);
				float tFromApex	 = std::sqrt(2.0f * (std::max)(0.0f, apexHeight) / param_.gravity);
				remainingTime	 = tToApex + tFromApex;
			} else {
				// 地面までの落下時間
				float v	  = -velocity_.y; // 落下速度
				float y	  = (std::max)(0.0f, worldTransform_.translation.y);
				float g	  = param_.gravity;
				float det = v * v + 2.0f * g * y;
				if(det > 0.0f) {
					remainingTime = (-v + std::sqrt(det)) / g;
				}
			}

			if(remainingTime > 0.001f) {
				jumpRotationSpeed_ = jumpRotationRemaining_ / remainingTime;
			}
		}

		if(jumpRotationRemaining_ > 0.0f) {
			float rotateAmount = jumpRotationSpeed_ * dt;
			if(rotateAmount > jumpRotationRemaining_) {
				rotateAmount = jumpRotationRemaining_;
			}
			jumpRotation_ += rotateAmount;
			jumpRotationRemaining_ -= rotateAmount;

			// 回りきったら急降下
			if(isDiving_ && jumpRotationRemaining_ <= 0.0f) {
				velocity_.y	  = param_.diveForce;
				jumpRotation_ = 0.0f; // 回転をデフォルトに戻す
				auto offset	  = CalyxEngine::Vector3{0.0f, 0.5f, 1.0f};
				EffectAPI::Stop(rollingFxHandle_);
				rollingFxHandle_ = {};

				// 急降下開始時に少し縦に伸ばす
				worldTransform_.scale = param_.diveScale;
			}
		}
	}

	return events;
}

DemoPlayer::LandingEvents DemoPlayer::ApplyGravity(float dt) {
	LandingEvents events{};
	// 可変ジャンプ：上昇中にボタンを離していたら重力を強くする
	float gravity = param_.gravity;
	bool  isJumpPush =
		CalyxFoundation::Input::PushKey(DIK_SPACE) || CalyxFoundation::Input::PushGamepadButton(CalyxFoundation::PadButton::A);
	if(isJumping_ && !isDiving_ && !isJumpPush && velocity_.y > 0.0f) {
		gravity *= param_.variableJumpMultiplier;
	}

	// 重力加算
	velocity_.y -= gravity * dt;

	// 接地判定（Y=0を床とする）
	// 上昇中（velocity_.y > 0）は着地扱いしない。
	// TimeStep=0（dt=0）でジャンプした瞬間は位置が動かず y=0 のままになるため、
	// この条件が無いとジャンプ開始と同フレームで着地判定が成立し、ジャンプ速度が消えてしまう。
	if(worldTransform_.translation.y <= 0.0f && velocity_.y <= 0.0f) {
		// 着地した瞬間
		if(isJumping_) {
			events.landed = true;
			// 着地衝撃波（急降下中なら大きく、通常なら少し大きめ）
			if(isDiving_) {
				events.hardLanded = true;
				// リカバリー開始
				isRecovering_  = true;
				recoveryTimer_ = 0.0f;
			}

			worldTransform_.scale = param_.landScale;
			scaleVelocity_		  = {0.0f, 0.0f, 0.0f};
		}

		worldTransform_.translation.y = 0.0f;
		velocity_.y					  = 0.0f;
		isJumping_				  = false;
		isDiving_				  = false;
		jumpRotation_			  = 0.0f;
		jumpRotationRemaining_  = 0.0f;
	}

	return events;
}

void DemoPlayer::UpdatePopScale(float dt) {

	CalyxEngine::Vector3 diff		  = worldTransform_.scale - targetScale_;
	CalyxEngine::Vector3 acceleration = (diff * -param_.stiffness) - (scaleVelocity_ * param_.damping);

	scaleVelocity_ += acceleration * dt;
	worldTransform_.scale += scaleVelocity_ * dt;
}

void DemoPlayer::HammerControl(float dt) {
	if(hammer_) {
		float hammerAngle = 0.0f;
		float pi		  = std::numbers::pi_v<float>;

		if(isRecovering_) {
			// 着地後のリカバリー
			recoveryTimer_ += dt;
			float progress = std::clamp(recoveryTimer_ / recoveryDuration_, 0.0f, 1.0f);
			// 90度(pi/2)から0度へ
			hammerAngle = std::lerp(pi * 0.5f, 0.0f, progress);

			if(progress >= 0.5f && isDivingInvincible_) {
				isDivingInvincible_ = false;
			}
			if(progress >= 1.0f) {
				isRecovering_ = false;
			}
			// リカバリー中サイズ
			CalyxEngine::Vector3 scaleMultiplier = CalyxEngine::Vector3::Lerp(param_.hammerSwingScale, {1.0f, 1.0f, 1.0f}, progress);

			hammer_->GetWorldTransform().scale.x = hammerInitialScale_.x * scaleMultiplier.x;
			hammer_->GetWorldTransform().scale.y = hammerInitialScale_.y * scaleMultiplier.y;
			hammer_->GetWorldTransform().scale.z = hammerInitialScale_.z * scaleMultiplier.z;

		} else if(isJumping_) {
			if(isDiving_) {
				// 急降下中は90度
				hammerAngle = pi * 0.5f;
				// 急降下中サイズ
				hammer_->GetWorldTransform().scale.x = hammerInitialScale_.x * param_.hammerSwingScale.x;
				hammer_->GetWorldTransform().scale.y = hammerInitialScale_.y * param_.hammerSwingScale.y;
				hammer_->GetWorldTransform().scale.z = hammerInitialScale_.z * param_.hammerSwingScale.z;
			} else {
				// スイング中
				if(jumpRotation_ <= pi / 6.0f) {
					float t		= jumpRotation_ / (pi / 6.0f);
					hammerAngle = std::lerp(0.0f, pi * 0.5f, t);
				} else if(jumpRotation_ <= pi) {
					hammerAngle = pi * 0.5f;
				} else {
					float t		= (jumpRotation_ - pi) / pi;
					hammerAngle = std::lerp(pi * 0.5f, 0.0f, t);

					// ジャンプの回転（pi〜2*pi）に合わせてサイズを戻す
					// 0~piの時に最大（param_.hammerSwingScale）、2*piの時に元（1.0）
					float				 scaleT			 = std::clamp((jumpRotation_ - pi) / (pi), 0.0f, 1.0f);
					CalyxEngine::Vector3 scaleMultiplier = CalyxEngine::Vector3::Lerp(param_.hammerSwingScale, {1.0f, 1.0f, 1.0f}, scaleT);

					hammer_->GetWorldTransform().scale.x = hammerInitialScale_.x * scaleMultiplier.x;
					hammer_->GetWorldTransform().scale.y = hammerInitialScale_.y * scaleMultiplier.y;
					hammer_->GetWorldTransform().scale.z = hammerInitialScale_.z * scaleMultiplier.z;
				}
			}
		} else {
			// 地面にいる時は初期サイズ
			hammer_->GetWorldTransform().scale = hammerInitialScale_;
		}


		// 親モデル(自分)のワールド回転をハンマーへ渡す（inheritRotate=false の分を手動で合成させる）
		hammer_->SetOwnerRotation(worldTransform_.rotation);

		CalyxEngine::Vector3 swingAxis = CalyxEngine::Quaternion::RotateVector({1.0f, 0.0f, 0.0f}, worldTransform_.rotation);
		hammer_->SetSwingAngle(swingAxis, -hammerAngle);
	}
}

void DemoPlayer::DamageFlash(float dt) {
	// ダメージ点滅
	if(damageFlashTimer_ > 0.0f) {
		damageFlashTimer_ -= dt;
		// 0.1秒おきに表示/非表示を切り替える
		bool isVisible = (static_cast<int>(damageFlashTimer_ * 10) % 2 == 0);
		SetDrawEnable(isVisible);
		if(hammer_) {
			hammer_->SetDrawEnable(isVisible);
		}

		if(damageFlashTimer_ <= 0.0f) {
			damageFlashTimer_ = 0.0f;
			isInvincible_	  = false;
			SetDrawEnable(true);
			if(hammer_) {
				hammer_->SetDrawEnable(true);
			}
		}
	}
}
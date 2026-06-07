#pragma once

#include <Engine/Application/Effects/EffectAsset.h>
#include <Engine/Assets/Manager/AssetManager.h>
#include <Engine/Foundation/Debug/CxAssert.h>
#include <Engine/Application/Effects/EffectPlayer.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <filesystem>
#include <memory>

namespace SceneAPI{
	template<class T, class... Args>
	std::shared_ptr<T> Instantiate(Args&&... args){
		auto ctx = SceneContext::Current();
		CX_CHECK(ctx && "No active SceneContext!", "Assertion failed");
		return ctx->Instantiate<T>(std::forward<Args>(args)...);
	}
}

namespace AudioAPI {
	inline ::Audio* Manager() {
		auto* manager = CalyxEngine::AssetManager::GetInstance()->GetAudioManager();
		CX_CHECK(manager && "AudioManager is not initialized!", "Assertion failed");
		return manager;
	}

	inline bool Load(CalyxEngine::Audio& audio, const std::filesystem::path& path) {
		if(!audio.Load(path)) return false;
		Manager()->Load(audio.GetFilename());
		return true;
	}

	inline void Play(const CalyxEngine::Audio& audio, bool loop = false, float volume = 1.0f) {
		CX_CHECK(audio.IsValid(), "Assertion failed");
		auto* manager = Manager();
		if(!manager->IsLoadedAudio(audio.GetFilename())) {
			manager->Load(audio.GetFilename());
		}
		manager->Play(audio.GetFilename(), loop, volume);
	}

	inline void PlayFromName(const std::filesystem::path& path, bool loop = false, float volume = 1.0f) {
		CalyxEngine::Audio audio;
		if(!Load(audio, path)) return;
		Manager()->Play(audio.GetFilename(), loop, volume);
	}

	inline void Stop(const CalyxEngine::Audio& audio) {
		CX_CHECK(audio.IsValid(), "Assertion failed");
		Manager()->EndAudio(audio.GetFilename());
	}

	inline void Pause(const CalyxEngine::Audio& audio) {
		CX_CHECK(audio.IsValid(), "Assertion failed");
		Manager()->PauseAudio(audio.GetFilename());
	}

	inline void Restart(const CalyxEngine::Audio& audio) {
		CX_CHECK(audio.IsValid(), "Assertion failed");
		Manager()->RestertAudio(audio.GetFilename());
	}

	inline void SetVolume(const CalyxEngine::Audio& audio, float volume) {
		CX_CHECK(audio.IsValid(), "Assertion failed");
		Manager()->SetAudioVolume(audio.GetFilename(), volume);
	}

	inline bool IsPlaying(const CalyxEngine::Audio& audio) {
		CX_CHECK(audio.IsValid(), "Assertion failed");
		return Manager()->IsPlayingAudio(audio.GetFilename());
	}

	inline void Unload(const CalyxEngine::Audio& audio) {
		CX_CHECK(audio.IsValid(), "Assertion failed");
		Manager()->UnloadAudio(audio.GetFilename());
	}
}

namespace EffectAPI {
	inline CalyxEngine::EffectPlayer* Player() {
		auto ctx = SceneContext::Current();
		CX_CHECK(ctx && "No active SceneContext!", "Assertion failed");
		return ctx->GetEffectPlayer();
	}

	inline CalyxEngine::EffectHandle Play(const CalyxEngine::EffectAsset& asset,
										  const CalyxEngine::Vector3& position,
										  const CalyxEngine::Quaternion& rotation = CalyxEngine::Quaternion::MakeIdentity(),
										  const CalyxEngine::Vector3& scale = {1.0f, 1.0f, 1.0f}) {
		return Player()->Play(asset, position, rotation, scale);
	}

	inline CalyxEngine::EffectHandle Play(const CalyxEngine::EffectAssetData& data,
										  const CalyxEngine::Vector3& position,
										  const CalyxEngine::Quaternion& rotation = CalyxEngine::Quaternion::MakeIdentity(),
										  const CalyxEngine::Vector3& scale = {1.0f, 1.0f, 1.0f}) {
		return Player()->Play(data, position, rotation, scale);
	}

	inline CalyxEngine::EffectHandle PlayFromName(const std::string& name,
												  const CalyxEngine::Vector3& position,
												  const CalyxEngine::Quaternion& rotation = CalyxEngine::Quaternion::MakeIdentity(),
												  const CalyxEngine::Vector3& scale = {1.0f, 1.0f, 1.0f}) {
		return Player()->PlayFromName(name, position, rotation, scale);
	}

	inline void Stop(CalyxEngine::EffectHandle handle) {
		Player()->Stop(handle);
	}
}

namespace CalyxEngine {
	// scene識別id
	using SceneId = uint8_t;
}
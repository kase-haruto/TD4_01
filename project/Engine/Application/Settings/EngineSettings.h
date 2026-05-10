#pragma once

#include <externals/nlohmann/json.hpp>

namespace CalyxEngine {

	struct RuntimeSettings {
		bool fullscreenGameViewOnPlay = false;
	};

	struct EngineSettingsData {
		RuntimeSettings runtime;
	};

	class EngineSettings {
	public:
		enum class Category {
			Runtime,
		};

		static EngineSettings& GetInstance();

		void Initialize();
		bool Load();
		bool Save() const;

		void OpenSettingsWindow();
		void RenderSettingsWindow();

		const EngineSettingsData& GetData() const { return data_; }

	private:
		EngineSettings() = default;

		static constexpr const char* kSettingsPath = "generated/Settings/EngineSettings.json";

		nlohmann::json ToJson() const;
		void		   ApplyJson(const nlohmann::json& json);

		EngineSettingsData data_{};
		EngineSettingsData editingData_{};
		Category		   selectedCategory_ = Category::Runtime;
		bool			   showSettingsWindow_ = false;
		bool			   editingInitialized_ = false;
	};

} // namespace CalyxEngine

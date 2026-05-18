#pragma once

#include <Engine/Application/UI/EngineUI/DebugTextManager.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <externals/imgui/imgui.h>

#include <Windows.h>
#include <string>

namespace CalyxEngine::Debug {

	inline void ReportAssertFailure(const char* expression,
									const char* message,
									const char* file,
									int			line,
									const Vector2& position,
									const ImVec4&  color) {
		std::string text = "Assert failed: ";
		text += expression ? expression : "";
		if(message && message[0] != '\0') {
			text += " | ";
			text += message;
		}

		DebugTextManager::AddPopupText(position, color, text, 4.0f, 32.0f);

		std::string debug = text;
		debug += " (";
		debug += file ? file : "";
		debug += ":";
		debug += std::to_string(line);
		debug += ")\n";
		OutputDebugStringA(debug.c_str());
	}

} // namespace CalyxEngine::Debug

#if defined(_DEBUG) || defined(DEVELOP)
#define CX_ENSURE_AT(expr, position, color, text) \
	((expr) ? true : (::CalyxEngine::Debug::ReportAssertFailure(#expr, text, __FILE__, __LINE__, position, color), false))

#define CX_CHECK_AT(expr, position, color, text) \
	do { \
		if(!(expr)) { \
			::CalyxEngine::Debug::ReportAssertFailure(#expr, text, __FILE__, __LINE__, position, color); \
			__debugbreak(); \
		} \
	} while(false)
#else
#define CX_ENSURE_AT(expr, position, color, text) (!!(expr))
#define CX_CHECK_AT(expr, position, color, text) ((void)sizeof(expr))
#endif

#define CX_ENSURE(expr, text) \
	CX_ENSURE_AT(expr, ::CalyxEngine::Vector2(24.0f, 72.0f), ImVec4(1.0f, 0.82f, 0.18f, 1.0f), text)

#define CX_CHECK(expr, text) \
	CX_CHECK_AT(expr, ::CalyxEngine::Vector2(24.0f, 72.0f), ImVec4(1.0f, 0.18f, 0.18f, 1.0f), text)

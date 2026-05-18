#pragma once

#include <Engine/Application/UI/EngineUI/DebugTextManager.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <externals/imgui/imgui.h>

#include <Windows.h>
#include <string>

namespace CalyxEngine::Debug {

	inline std::string MakeAssertText(const char* expression, const char* message) {
		std::string text = "Assert failed: ";
		text += expression ? expression : "";
		if(message && message[0] != '\0') {
			text += " | ";
			text += message;
		}
		return text;
	}

	inline void OutputAssertDebugString(const std::string& text, const char* file, int line) {
		std::string debug = text;
		debug += " (";
		debug += file ? file : "";
		debug += ":";
		debug += std::to_string(line);
		debug += ")\n";
		OutputDebugStringA(debug.c_str());
	}

	inline void ReportAssertWarning(const char* expression,
									const char* message,
									const char* file,
									int			line,
									const Vector2& position,
									const ImVec4&  color) {
		const std::string text = MakeAssertText(expression, message);

		DebugTextManager::AddPopupText(position, color, text, 4.0f, 32.0f);
		OutputAssertDebugString(text, file, line);
	}

	inline void ReportFatalAssert(const char* expression,
								  const char* message,
								  const char* file,
								  int		  line,
								  const ImVec4& color) {
		const std::string text = MakeAssertText(expression, message);
		DebugTextManager::SetFatalAssert(
			DebugTextManager::FatalAssert{
				expression ? expression : "",
				message ? message : "",
				file ? file : "",
				line,
				color});
		OutputAssertDebugString(text, file, line);
	}

} // namespace CalyxEngine::Debug

#if defined(_DEBUG) || defined(DEVELOP)
#define CX_ENSURE_AT(expr, position, color, text) \
	((expr) ? true : (::CalyxEngine::Debug::ReportAssertWarning(#expr, text, __FILE__, __LINE__, position, color), false))

#define CX_CHECK_AT(expr, position, color, text) \
	do { \
		if(!(expr)) { \
			::CalyxEngine::Debug::ReportFatalAssert(#expr, text, __FILE__, __LINE__, color); \
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

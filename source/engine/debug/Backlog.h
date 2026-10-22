#pragma once
#include "core/CommonInclude.h"
#include "render/GraphicsAPI/GraphicsDevice.h"
#include "render/Canvas.h"
#include "math/Color.h"
#include "core/Helper.h"

#include <string>
#include <cassert>

#define wilog_level(str,level,...) {char text[1024]; snprintf(text, sizeof(text), str, ## __VA_ARGS__); Protean::Backlog::post(text, level);}
#define wilog_messagebox(str,...) {char text[1024]; snprintf(text, sizeof(text), str, ## __VA_ARGS__); Protean::Backlog::post(text, Protean::Backlog::LogLevel::Error); Protean::Helper::messageBox(text, "Error!");}
#define wilog_warning(str,...) {wilog_level(str, Protean::Backlog::LogLevel::Warning, ## __VA_ARGS__);}
#define wilog_error(str,...) {wilog_level(str, Protean::Backlog::LogLevel::Error, ## __VA_ARGS__);}
#define wilog_success(str,...) {wilog_level(str, Protean::Backlog::LogLevel::Success, ## __VA_ARGS__);}
#define wilog(str,...) {wilog_level(str, Protean::Backlog::LogLevel::Default, ## __VA_ARGS__);}
#define wilog_assert(cond,str,...) {if(!(cond)){wilog_error(str, ## __VA_ARGS__); assert(cond);}}

#ifdef _DEBUG
#define wilog_debug(str,...) {wilog_level(str, Protean::Backlog::LogLevel::Default, ## __VA_ARGS__);}
#else
#define wilog_debug(str,...)
#endif // DEBUG

namespace Protean::Backlog
{
	// Keep it in sync with LogLevel LUA binding
	enum class LogLevel
	{
		None,
		Default,
		Success,
		Warning,
		Error,
	};
	void Toggle();
	void Scroll(float direction);
	void Update(const Protean::Canvas& canvas, float dt = 1.0f / 60.0f);
	void Draw(
		const Protean::Canvas& canvas,
		Protean::Graphics::CommandList cmd,
		Protean::Graphics::ColorSpace colorspace = Protean::Graphics::ColorSpace::SRGB
	);
	void DrawOutputText(
		const Protean::Canvas& canvas,
		Protean::Graphics::CommandList cmd,
		Protean::Graphics::ColorSpace colorspace = Protean::Graphics::ColorSpace::SRGB
	);

	std::string getText();
	void clear();
	void post(const char* input, LogLevel level = LogLevel::Default);
	void post(const std::string& input, LogLevel level = LogLevel::Default);

	void historyPrev();
	void historyNext();

	bool isActive();

	void setBackground(Protean::Graphics::Texture* texture);
	void setBackgroundColor(Protean::Color color);
	void setFontSize(int value);
	void setFontRowspacing(float value);
	void setFontColor(Protean::Color color);

	void Lock();
	void Unlock();

	void BlockLuaExecution();
	void UnblockLuaExecution();

	void SetLogLevel(LogLevel newLevel);

	LogLevel GetUnseenLogLevelMax();

	void SetLogFile(const std::string& path);
	void GetLogFile(std::string& path);
	std::string GetLogFile();

	// Flush pending log writes to file
	void Flush();
	// Set the interval for automatic periodic flushes (in milliseconds, 0 = disabled, default = 1000ms)
	void SetAutoFlushInterval(uint32_t milliseconds);

	struct LogEntry
	{
		std::string text;
		LogLevel level = LogLevel::Default;
	};
	// this function is intended to be used only in crash handlers to write the current
	// backlog, but without locking or any mallocs. It might iterate over complete garbage,
	// so it should only be used as a last resort.
	// It's also not considered part of the API and can change at any time.
	//
	// Use getText() instead, unless absolutely necessary.
	void _forEachLogEntry_unsafe(const std::function<void(const LogEntry&)>& cb);
};

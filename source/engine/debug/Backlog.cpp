#include "debug/Backlog.h"
#include "math/Math.h"
#include "resource/ResourceManager.h"
#include "render/TextureHelper.h"
#include "render/Font.h"
#include "render/SpriteFont.h"
#include "render/Image.h"
#include "script/Lua.h"
#include "input/Input.h"
#include "platform/Platform.h"
#include "core/Helper.h"
#include "gui/GUI.h"
#include "core/Timer.h"

#include <mutex>
#include <deque>
#include <limits>
#include <iostream>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <fstream>
#include <chrono>

using namespace Protean::Graphics;

namespace Protean::Backlog
{
	bool enabled = false;
	bool was_ever_enabled = enabled;
	const float speed = 4000.0f;
	const size_t deleteFromLine = 500; // Only limits in-memory display buffer, not file
	float pos = 5;
	int historyPos = 0;
	Protean::Font::Params font_params;
	Protean::Color backgroundColor = Protean::Color(17, 30, 43, 255);
	Texture backgroundTex;
	bool refitscroll = false;
	Protean::Gui::TextInputField inputField;
	Protean::Gui::Button toggleButton;
	Protean::Gui::GUI GUI;
	Protean::Gui::ScrollBar scrollbar;
	float autoscroll = 0; // automatic scroll behaviour while backlog is not really updated with a proper scrollbar (init screen, shader compile, etc) the text should still scroll

	bool locked = false;
	bool blockLuaExec = false;
	LogLevel logLevel = LogLevel::Default;
	LogLevel unseen = LogLevel::None;

	std::deque<LogEntry> history;
	std::mutex historyLock;
	std::string logfile_path = "";

	struct AsyncWriter
	{
		std::thread writerThread;
		std::mutex queueMutex;
		std::condition_variable queueEntriesCondition;
		std::condition_variable queueEmptyCondition;
		std::deque<std::string> writeQueue;
		std::atomic<bool> running{ false };
		std::atomic<bool> initialized{ false };
		std::atomic<uint32_t> autoFlushInterval{ 1000 };
		Protean::Timer lastFlushTimer;
		std::ofstream logFileStream;
		std::string currentLogFilePath;
		std::mutex flushMutex; // Synchronous flush requests

		void Start(const std::string& filepath)
		{
			if (initialized.exchange(true))
			{
				return;
			}

			currentLogFilePath = filepath;
			running = true;
			lastFlushTimer.record();

			logFileStream.open(filepath, std::ios::binary | std::ios::trunc);
			if (!logFileStream.is_open())
			{
				Protean::Helper::DebugOut("Failed to open log file: " + filepath + "\n", Protean::Helper::DebugLevel::Error);
			}

			writerThread = std::thread([this]() {
				WriterLoop();
			});
		}

		void Stop()
		{
			if (!initialized.load())
			{
				return;
			}

			running = false;
			queueEntriesCondition.notify_all();

			if (writerThread.joinable())
			{
				writerThread.join();
			}

			// Final flush of any remaining entries
			FlushQueue();

			if (logFileStream.is_open())
			{
				logFileStream.flush();
				logFileStream.close();
			}

			initialized = false;
		}

		void WriterLoop()
		{
			while (running.load())
			{
				std::unique_lock lock(queueMutex);

				// Wait for new entries or timeout for periodic flush
				auto timeout = std::chrono::milliseconds(100);
				queueEntriesCondition.wait_for(lock, timeout, [this]() {
					return !writeQueue.empty() || !running.load();
				});

				if (!running.load() && writeQueue.empty())
					break;

				// Process all queued entries
				static std::deque<std::string> localQueue;
				std::swap(localQueue, writeQueue);
				lock.unlock();

				// Notify waiters that queue is now empty after swap
				queueEmptyCondition.notify_all();

				WriteEntries(localQueue);
				localQueue.clear();

				// Check for auto-flush interval
				const auto interval = autoFlushInterval.load();
				if (interval > 0 && lastFlushTimer.elapsed_milliseconds() >= interval)
				{
					if (logFileStream.is_open())
					{
						logFileStream.flush();
					}
					lastFlushTimer.record();
				}
			}
		}

		void WriteEntries(const std::deque<std::string>& entries)
		{
			if (!logFileStream.is_open())
			{
				return;
			}

			for (const auto& entry : entries)
			{
				logFileStream.write(entry.c_str(), entry.length());
			}
		}

		void FlushQueue()
		{
			std::unique_lock lock(queueMutex);
			static thread_local std::deque<std::string> localQueue;
			std::swap(localQueue, writeQueue);
			lock.unlock();

			WriteEntries(localQueue);
			localQueue.clear();

			if (logFileStream.is_open())
			{
				logFileStream.flush();
			}
			lastFlushTimer.record();
		}

		void Enqueue(const std::string& text)
		{
			{
				std::scoped_lock lock(queueMutex);
				writeQueue.push_back(text);
			}
			queueEntriesCondition.notify_one();
		}

		void FlushSync()
		{
			std::scoped_lock lock(flushMutex);
			if (!initialized.load())
			{
				return;
			}

			// Signal and wait for queue to be processed
			std::unique_lock queueLock(queueMutex);
			if (!writeQueue.empty())
			{
				queueEntriesCondition.notify_one();
				queueEmptyCondition.wait(queueLock, [this] { return writeQueue.empty(); });
			}
			queueLock.unlock();

			// Force file flush
			if (logFileStream.is_open())
			{
				logFileStream.flush();
			}
		}

		void SetAutoFlushInterval(uint32_t ms)
		{
			autoFlushInterval = ms;
		}

		void UpdateLogFilePath(const std::string& newPath)
		{
			std::scoped_lock lock(flushMutex);

			// Flush and close current file
			FlushQueue();
			if (logFileStream.is_open())
			{
				logFileStream.close();
			}

			// Open new file
			currentLogFilePath = newPath;
			logFileStream.open(newPath, std::ios::binary | std::ios::trunc);
		}
	};

	static AsyncWriter asyncWriter;

	struct InternalState
	{
		// These must have common lifetime and destruction order, so keep them together in a struct:
		std::deque<LogEntry> entries;
		std::mutex entriesLock;

		InternalState() = default;
		~InternalState()
		{
			asyncWriter.Stop();
		}

		InternalState(const InternalState&) = delete;
		InternalState& operator=(const InternalState&) = delete;
		InternalState(InternalState&&) = delete;
		InternalState& operator=(InternalState&&) = delete;

		std::string getText()
		{
			std::scoped_lock lck(entriesLock);
			std::string retval;
			_forEachLogEntry_unsafe([&](auto&& entry) {retval += entry.text;});
			return retval;
		}
		void _forEachLogEntry_unsafe(const std::function<void(const LogEntry&)>& cb) const
		{
			for (auto& entry : entries)
			{
				cb(entry);
			}
		}
	} internal_state;

	void Flush()
	{
		asyncWriter.FlushSync();
	}

	void SetAutoFlushInterval(uint32_t milliseconds)
	{
		asyncWriter.SetAutoFlushInterval(milliseconds);
	}

	void Toggle()
	{
		enabled = !enabled;
		was_ever_enabled = true;
	}
	void Scroll(float direction)
	{
		scrollbar.Scroll(direction);
		autoscroll += direction;
	}
	void Update(const Protean::Canvas& canvas, float dt)
	{
		if (!locked)
		{
			if (Protean::Input::Press(Protean::Input::KEYBOARD_BUTTON_HOME))
			{
				Toggle();
			}

			if (isActive())
			{

				if (Protean::Input::Press(Protean::Input::KEYBOARD_BUTTON_UP))
				{
					historyPrev();
				}
				if (Protean::Input::Press(Protean::Input::KEYBOARD_BUTTON_DOWN))
				{
					historyNext();
				}
				if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_PAGEUP))
				{
					Scroll(1000.0f * dt);
				}
				if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_PAGEDOWN))
				{
					Scroll(-1000.0f * dt);
				}

				Scroll(Protean::Input::GetPointer().z * 20);

				static bool created = false;
				if (!created)
				{
					created = true;
					inputField.Create("");
					inputField.SetCancelInputEnabled(false);
					inputField.OnInputAccepted([](const Protean::Gui::EventArgs& args) {
						historyPos = 0;
						post(args.sValue);
						LogEntry entry;
						entry.text = args.sValue;
						entry.level = LogLevel::Default;
						{
							std::scoped_lock lock(historyLock);
							history.push_back(entry);
							if (history.size() > deleteFromLine)
							{
								history.pop_front();
							}
						}
						if (!blockLuaExec)
						{
							Protean::Lua::RunText(args.sValue);
						}
						else
						{
							post("Lua execution is disabled", LogLevel::Error);
						}
						inputField.SetText("");
					});
					const Protean::Color theme_color_idle = Protean::Color(30, 40, 60, 200);
					const Protean::Color theme_color_focus = Protean::Color(70, 150, 170, 220);
					const Protean::Color theme_color_active = Protean::Color::White();
					const Protean::Color theme_color_deactivating = Protean::Color::lerp(theme_color_focus, Protean::Color::White(), 0.5f);
					const Protean::Color shadow_color = Protean::Color(80, 140, 180, 100);
					const Protean::Color font_color = Protean::Color(160, 240, 250, 255);
					inputField.SetColor(theme_color_idle); // all states the same, it's gonna be always active anyway
					inputField.font.params.color = Protean::Color(160, 240, 250, 255);
					inputField.font.params.shadowColor = Protean::Color::Transparent();

					toggleButton.Create("V");
					toggleButton.OnClick([](const Protean::Gui::EventArgs& args) {
						Toggle();
						});
					toggleButton.SetColor(theme_color_idle, Protean::Gui::IDLE);
					toggleButton.SetColor(theme_color_focus, Protean::Gui::FOCUS);
					toggleButton.SetColor(theme_color_active, Protean::Gui::ACTIVE);
					toggleButton.SetColor(theme_color_deactivating, Protean::Gui::DEACTIVATING);
					toggleButton.SetShadowRadius(5);
					toggleButton.SetShadowColor(shadow_color);
					toggleButton.font.params.color = font_color;
					toggleButton.font.params.rotation = XM_PI;
					toggleButton.font.params.size = 24;
					toggleButton.font.params.scaling = 3;
					toggleButton.font.params.shadowColor = Protean::Color::Transparent();
					for (int i = 0; i < arraysize(toggleButton.sprites); ++i)
					{
						toggleButton.sprites[i].params.enableCornerRounding();
						toggleButton.sprites[i].params.corners_rounding[2].radius = 50;
					}

					scrollbar.SetVertical(true);
					scrollbar.SetColor(theme_color_idle, Protean::Gui::IDLE);
					scrollbar.SetColor(theme_color_focus, Protean::Gui::FOCUS);
					scrollbar.SetColor(theme_color_active, Protean::Gui::ACTIVE);
					scrollbar.SetColor(theme_color_deactivating, Protean::Gui::DEACTIVATING);
					scrollbar.SetColor(theme_color_focus, Protean::Gui::WIDGET_ID_SCROLLBAR_KNOB_INACTIVE);
					scrollbar.SetColor(font_color, Protean::Gui::WIDGET_ID_SCROLLBAR_KNOB_HOVER);
					scrollbar.SetColor(theme_color_active, Protean::Gui::WIDGET_ID_SCROLLBAR_KNOB_GRABBED);
					scrollbar.SetColor(theme_color_active, Protean::Gui::WIDGET_ID_SCROLLBAR_KNOB_GRABBED);
					scrollbar.SetShadowRadius(5);
					scrollbar.SetShadowColor(shadow_color);
					for (int i = 0; i < arraysize(scrollbar.sprites); ++i)
					{
						scrollbar.sprites[i].params.enableCornerRounding();
						scrollbar.sprites[i].params.corners_rounding[0].radius = 8;
						scrollbar.sprites[i].params.corners_rounding[1].radius = 8;
						scrollbar.sprites[i].params.corners_rounding[2].radius = 8;
						scrollbar.sprites[i].params.corners_rounding[3].radius = 8;
					}
					for (int i = 0; i < arraysize(scrollbar.sprites_knob); ++i)
					{
						scrollbar.sprites_knob[i].params.enableCornerRounding();
						scrollbar.sprites_knob[i].params.corners_rounding[0].radius = 8;
						scrollbar.sprites_knob[i].params.corners_rounding[1].radius = 8;
						scrollbar.sprites_knob[i].params.corners_rounding[2].radius = 8;
						scrollbar.sprites_knob[i].params.corners_rounding[3].radius = 8;
					}
				}
				if (inputField.GetState() != Protean::Gui::ACTIVE)
				{
					inputField.SetAsActive();
				}

				const float textheight = Protean::Font::TextHeight(getText(), font_params); // only call when it's active, this allocates
				scrollbar.SetListLength(std::max(textheight, canvas.GetLogicalHeight()));
				autoscroll = 0; // when scrollbar active then no autoscroll
			}
			else
			{
				inputField.Deactivate();
			}
		}

		if (enabled)
		{
			pos += speed * dt;
		}
		else
		{
			pos -= speed * dt;
		}
		pos = Protean::Math::Clamp(pos, -canvas.GetLogicalHeight(), 0);

		inputField.SetSize(XMFLOAT2(canvas.GetLogicalWidth() - 40, 20));
		inputField.SetPos(XMFLOAT2(20, canvas.GetLogicalHeight() - 40 + pos));
		inputField.Update(canvas, dt);

		toggleButton.SetSize(XMFLOAT2(100, 100));
		toggleButton.SetPos(XMFLOAT2(canvas.GetLogicalWidth() - toggleButton.GetSize().x - 20, 20 + pos));
		toggleButton.Update(canvas, dt);

		scrollbar.SetSize(XMFLOAT2(20, canvas.GetLogicalHeight() - 20 - toggleButton.GetSize().y - toggleButton.GetShadowRadius() * 2 - 70));
		scrollbar.SetPos(XMFLOAT2(canvas.GetLogicalWidth() - scrollbar.GetSize().x - 20, toggleButton.GetSize().y + toggleButton.GetShadowRadius() * 2 + 30 + pos));
		scrollbar.Update(canvas, dt);
	}
	void Draw(
		const Protean::Canvas& canvas,
		CommandList cmd,
		ColorSpace colorspace
	)
	{
		if (!was_ever_enabled || pos <= -canvas.GetLogicalHeight())
		{
			return;
		}

		GraphicsDevice* device = GetDevice();
		device->EventBegin("Backlog", cmd);

		Protean::Image::Params fx = Protean::Image::Params((float)canvas.GetLogicalWidth(), (float)canvas.GetLogicalHeight());
		fx.pos = XMFLOAT3(0, pos, 0);
		fx.opacity = Protean::Math::Lerp(0.9f, 0, saturate(-pos / canvas.GetLogicalHeight()));
		if (colorspace != ColorSpace::SRGB)
		{
			fx.enableLinearOutputMapping(9);
		}
		fx.color = backgroundColor;
		Protean::Image::Draw(backgroundTex.IsValid() ? &backgroundTex : nullptr, fx, cmd);

		Protean::Image::Params inputbg;
		inputbg.color = Protean::Color(80, 140, 180, 200);
		inputbg.pos = inputField.translation;
		inputbg.pos.x -= 8;
		inputbg.pos.y -= 8;
		inputbg.siz = inputField.GetSize();
		inputbg.siz.x += 16;
		inputbg.siz.y += 16;
		inputbg.enableCornerRounding();
		inputbg.corners_rounding[0].radius = 10;
		inputbg.corners_rounding[1].radius = 10;
		inputbg.corners_rounding[2].radius = 10;
		inputbg.corners_rounding[3].radius = 10;
		if (colorspace != ColorSpace::SRGB)
		{
			inputbg.enableLinearOutputMapping(9);
		}
		Protean::Image::Draw(nullptr, inputbg, cmd);

		if (colorspace != ColorSpace::SRGB)
		{
			inputField.sprites[inputField.GetState()].params.enableLinearOutputMapping(9);
			inputField.font.params.enableLinearOutputMapping(9);
			toggleButton.sprites[toggleButton.GetState()].params.enableLinearOutputMapping(9);
			toggleButton.font.params.enableLinearOutputMapping(9);
			scrollbar.sprites[scrollbar.GetState()].params.enableLinearOutputMapping(9);
		}
		inputField.Render(canvas, cmd);

		Protean::Graphics::Rect rect;
		rect.left = 0;
		rect.right = (int32_t)canvas.GetPhysicalWidth();
		rect.top = 0;
		rect.bottom = (int32_t)canvas.GetPhysicalHeight();
		device->BindScissorRects(1, &rect, cmd);

		scrollbar.Render(canvas, cmd);

		toggleButton.Render(canvas, cmd);

		rect.bottom = int32_t(canvas.LogicalToPhysical(inputField.GetPos().y - 15));
		device->BindScissorRects(1, &rect, cmd);

		DrawOutputText(canvas, cmd, colorspace);

		rect.left = 0;
		rect.right = std::numeric_limits<int>::max();
		rect.top = 0;
		rect.bottom = std::numeric_limits<int>::max();
		device->BindScissorRects(1, &rect, cmd);
		device->EventEnd(cmd);
	}

	void DrawOutputText(
		const Protean::Canvas& canvas,
		CommandList cmd,
		ColorSpace colorspace
	)
	{
		Protean::Font::SetCanvas(canvas); // always set here as it can be called from outside...
		Protean::Font::Params params = font_params;
		params.cursor = {};
		if (refitscroll)
		{
			const float textheight = Protean::Font::TextHeight(getText(), font_params);
			const float limit = canvas.GetLogicalHeight() - 50;
			if (scrollbar.GetOffset() + textheight > limit)
			{
				scrollbar.SetOffset(limit - scrollbar.GetOffset() + textheight);
			}
			if (autoscroll + textheight > limit)
			{
				autoscroll = limit - textheight;
			}
			refitscroll = false;
		}
		params.posX = 5;
		params.posY = pos + scrollbar.GetOffset() + autoscroll;
		params.h_wrap = canvas.GetLogicalWidth() - params.posX;
		if (colorspace != ColorSpace::SRGB)
		{
			params.enableLinearOutputMapping(9);
		}

		static std::deque<LogEntry> entriesCopy;

		internal_state.entriesLock.lock();
		// Force copy because drawing text while locking is not safe because an error inside might try to lock again!
		entriesCopy = internal_state.entries;
		internal_state.entriesLock.unlock();

		for (auto& x : entriesCopy)
		{
			switch (x.level)
			{
			case LogLevel::Warning:
				params.color = Protean::Color::Warning();
				break;
			case LogLevel::Error:
				params.color = Protean::Color::Error();
				break;
			case LogLevel::Success:
				params.color = Protean::Color::Success();
				break;
			default:
				params.color = font_params.color;
				break;
			}
			params.cursor = Protean::Font::Draw(x.text, params, cmd);
		}

		unseen = LogLevel::None;
	}

	std::string getText()
	{
		return internal_state.getText();
	}
	// You generally don't want to use this. See notes in header
	void _forEachLogEntry_unsafe(const std::function<void(const LogEntry&)>& cb)
	{
		internal_state._forEachLogEntry_unsafe(cb);
	}
	void clear()
	{
		std::scoped_lock lck(internal_state.entriesLock);
		internal_state.entries.clear();
		scrollbar.SetOffset(0);
	}
	void post(const char* input, LogLevel level)
	{
		if (logLevel > level)
		{
			return;
		}

		// Initialize async writer on first log
		if (!asyncWriter.initialized.load())
		{
			asyncWriter.Start(GetLogFile());
		}

		std::string str;
		switch (level)
		{
		default:
		case LogLevel::Default:
			str = "";
			break;
		case LogLevel::Warning:
			str = "[Warning] ";
			break;
		case LogLevel::Error:
			str = "[Error] ";
			break;
		}
		str += input;
		str += '\n';
		LogEntry entry;
		entry.text = str;
		entry.level = level;

		// Add to in-memory display buffer
		internal_state.entriesLock.lock();
		internal_state.entries.push_back(entry);
		if (internal_state.entries.size() > deleteFromLine)
		{
			internal_state.entries.pop_front();
		}
		internal_state.entriesLock.unlock();

		// Enqueue for async file writing
		asyncWriter.Enqueue(str);

		refitscroll = true;

		switch (level)
		{
		default:
		case LogLevel::Default:
			Protean::Helper::DebugOut(str, Protean::Helper::DebugLevel::Normal);
			break;
		case LogLevel::Warning:
			Protean::Helper::DebugOut(str, Protean::Helper::DebugLevel::Warning);
			break;
		case LogLevel::Error:
			Protean::Helper::DebugOut(str, Protean::Helper::DebugLevel::Error);
			break;
		}

		unseen = std::max(unseen, level);

		// Force an immediate flush on errors to prevent potential data loss
		//	in case the application is about to crash
		if (level >= LogLevel::Error)
		{
			asyncWriter.FlushSync();
		}
	}

	void post(const std::string& input, LogLevel level)
	{
		post(input.c_str(), level);
	}

	void historyPrev()
	{
		std::scoped_lock lock(historyLock);
		if (!history.empty())
		{
			inputField.SetText(history[history.size() - 1 - historyPos].text);
			inputField.SetAsActive();
			if ((size_t)historyPos < history.size() - 1)
			{
				historyPos++;
			}
		}
	}
	void historyNext()
	{
		std::scoped_lock lock(historyLock);
		if (!history.empty())
		{
			if (historyPos > 0)
			{
				historyPos--;
			}
			inputField.SetText(history[history.size() - 1 - historyPos].text);
			inputField.SetAsActive();
		}
	}

	void setBackground(Texture* texture)
	{
		backgroundTex = *texture;
	}
	void setBackgroundColor(Protean::Color color)
	{
		backgroundColor = color;
	}
	void setFontSize(int value)
	{
		font_params.size = value;
	}
	void setFontRowspacing(float value)
	{
		font_params.spacingY = value;
	}
	void setFontColor(Protean::Color color)
	{
		font_params.color = color;
	}

	bool isActive() { return enabled; }

	void Lock()
	{
		locked = true;
		enabled = false;
	}
	void Unlock()
	{
		locked = false;
	}

	void BlockLuaExecution()
	{
		blockLuaExec = true;
	}
	void UnblockLuaExecution()
	{
		blockLuaExec = false;
	}

	void SetLogLevel(LogLevel newLevel)
	{
		logLevel = newLevel;
	}

	LogLevel GetUnseenLogLevelMax()
	{
		return unseen;
	}

	void SetLogFile(const std::string& path)
	{
		logfile_path = path;
		if (asyncWriter.initialized.load())
		{
			asyncWriter.UpdateLogFilePath(GetLogFile());
		}
	}

	void GetLogFile(std::string& path)
	{
		if (!logfile_path.empty())
		{
			path = logfile_path;
		}
	}

	std::string GetLogFile()
	{
		if (logfile_path.empty() || !Protean::Helper::DirectoryExists(Protean::Helper::GetDirectoryFromPath(logfile_path)))
		{
			return Protean::Helper::GetCurrentPath() + "/log.txt";
		}
		return logfile_path;
	}
}

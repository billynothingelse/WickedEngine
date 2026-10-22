#pragma once
#include "core/common_include.h"

#include <string>

namespace Protean
{
	// Loads a shared library (.dll / .so / .dylib) at runtime and resolves symbols from it
	struct SharedLibrary
	{
		SharedLibrary() = default;
		SharedLibrary(const SharedLibrary&) = delete;
		SharedLibrary& operator=(const SharedLibrary&) = delete;
		~SharedLibrary() { Unload(); }

		// Returns false and keeps the error message (see GetError()) if the library can not be loaded
		bool Load(const std::string& path);
		void Unload();
		bool IsLoaded() const { return handle != nullptr; }

		void* GetSymbol(const char* name) const;
		template<typename T>
		T GetFunction(const char* name) const { return (T)GetSymbol(name); }

		const std::string& GetPath() const { return path; }
		const std::string& GetError() const { return error; }

		// Platform specific file name: "Name.dll", "libName.so", "libName.dylib"
		static std::string GetPlatformFileName(const std::string& name);

	private:
		void* handle = nullptr;
		std::string path;
		std::string error;
	};
}

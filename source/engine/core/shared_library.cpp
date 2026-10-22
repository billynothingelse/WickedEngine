#include "core/shared_library.h"
#include "core/helper.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace Protean
{
	bool SharedLibrary::Load(const std::string& libraryPath)
	{
		Unload();
		path = libraryPath;
		error.clear();
#if defined(_WIN32)
		std::wstring wpath;
		Helper::StringConvert(libraryPath, wpath);
		handle = (void*)LoadLibraryW(wpath.c_str());
		if (handle == nullptr)
		{
			error = "LoadLibrary failed with error " + std::to_string(GetLastError());
		}
#else
		handle = dlopen(libraryPath.c_str(), RTLD_NOW | RTLD_LOCAL);
		if (handle == nullptr)
		{
			const char* message = dlerror();
			error = message != nullptr ? message : "dlopen failed";
		}
#endif
		return handle != nullptr;
	}

	void SharedLibrary::Unload()
	{
		if (handle == nullptr)
			return;
#if defined(_WIN32)
		FreeLibrary((HMODULE)handle);
#else
		dlclose(handle);
#endif
		handle = nullptr;
	}

	void* SharedLibrary::GetSymbol(const char* name) const
	{
		if (handle == nullptr)
			return nullptr;
#if defined(_WIN32)
		return (void*)GetProcAddress((HMODULE)handle, name);
#else
		return dlsym(handle, name);
#endif
	}

	std::string SharedLibrary::GetPlatformFileName(const std::string& name)
	{
#if defined(_WIN32)
		return name + ".dll";
#elif defined(__APPLE__)
		return "lib" + name + ".dylib";
#else
		return "lib" + name + ".so";
#endif
	}
}

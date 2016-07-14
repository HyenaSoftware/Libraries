#include "stdafx.h"
#include "module_cross_singleton.hpp"


#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <Psapi.h>

using namespace std;
using namespace utility;

storage _module_local_storage;
HMODULE _singleton_owner_module { NULL };
extern "C" LPVOID (__stdcall *ptr_common_underlying_storage)() { NULL };

std::vector<HMODULE> get_modules()
{
	std::vector<HMODULE> modules;
	modules.resize(100);

	DWORD necessaryArraySize = 0;
	DWORD availableArraySize = modules.size() * sizeof(HMODULE);

	// try first
	auto current_proc = GetCurrentProcess();
	EnumProcessModules(current_proc, modules.data(), availableArraySize, &necessaryArraySize);

	// it grows or shrink the vector to the expected size
	modules.resize(necessaryArraySize / sizeof(HMODULE));

	if(availableArraySize < necessaryArraySize)
	{
		// try again
		EnumProcessModules(current_proc, modules.data(), availableArraySize, &necessaryArraySize);
	}

	return modules; // NRVO
}

storage& utility::gl_storage()
{
	if (_singleton_owner_module == NULL || ptr_common_underlying_storage == NULL)
	{
		auto modules = get_modules();

		if(modules.empty())
		{
			throw runtime_error { "Process unable to detect any modules of itself." };
		}

		for (auto module : modules)
		{
			auto ptr_underlying_storage = GetProcAddress(module, "_underlying_storage@0");

			if (ptr_underlying_storage)
			{
				_singleton_owner_module = module;
				ptr_common_underlying_storage = reinterpret_cast<LPVOID(__stdcall*)()>(ptr_underlying_storage);
				break;
			}
		}
	}

	auto ptr_raw_storage = ptr_common_underlying_storage();

	return *static_cast<storage*>(ptr_raw_storage);
}

extern "C" LPVOID __declspec(dllexport) __stdcall underlying_storage()
{
	return &_module_local_storage;
}


singleton& storage::get_singleton_of(std::type_index key_, std::function<std::unique_ptr<singleton>()> allocator_)
{
	auto& ptr = _instances[key_];
	if(ptr == nullptr)
	{
		ptr = allocator_();
	}

	return *ptr;
}

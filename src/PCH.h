#pragma once

#include "RE/Skyrim.h"
#include "REX/REX/Singleton.h"
#include "SKSE/SKSE.h"
#include <Windows.h>
#include <detours/detours.h>

#include <ClibUtil/simpleINI.hpp>
#include <ClibUtil/distribution.hpp>
#include <spdlog/sinks/basic_file_sink.h>

#define DLLEXPORT __declspec(dllexport)
#define FUNCTYPE_DETOUR static inline constinit decltype(thunk)*

namespace logger = SKSE::log;
namespace string = clib_util::string;
namespace ini = clib_util::ini;
namespace dist = clib_util::distribution;

using namespace std::literals;

namespace stl
{
	using namespace SKSE::stl;

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		auto& trampoline = SKSE::GetTrampoline();
		SKSE::AllocTrampoline(14);

		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}

	template <class T>
	void write_detour(std::uintptr_t a_src)
	{
		if (!a_src) {
			SKSE::stl::report_and_fail(fmt::format("Invalid target address for detour."));
		}

		using FnPtr = decltype(T::func);
		FnPtr orig = reinterpret_cast<FnPtr>(a_src);

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		auto attachResult = DetourAttach(reinterpret_cast<PVOID*>(&orig), T::thunk);
		if (attachResult != NO_ERROR) {
			DetourTransactionAbort();
			SKSE::stl::report_and_fail(fmt::format("Detour attach failed. Address: 0x{:X} - Error: {}", a_src, attachResult));
		}

		auto commitResult = DetourTransactionCommit();
		if (commitResult != NO_ERROR) {
			SKSE::stl::report_and_fail(fmt::format("Detour commit failed. Address: 0x{:X} - Error: {}", a_src, commitResult));
		}

		T::func = orig;
	}
}

#include "Version.h"

#ifdef SKYRIM_AE
#	define OFFSET(se, ae) ae
#	define OFFSET_3(se, ae, vr) ae
#elif SKYRIMVR
#	define OFFSET(se, ae) se
#	define OFFSET_3(se, ae, vr) vr
#else
#	define OFFSET(se, ae) se
#	define OFFSET_3(se, ae, vr) se
#endif

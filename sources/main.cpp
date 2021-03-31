#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#include <psapi.h>

#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <cctype>

#include <stdexcept>
#include <string>

struct Immovable
{
	Immovable() = default;
	Immovable(const Immovable &) = delete;
	Immovable(Immovable &&) = delete;
	Immovable &operator = (const Immovable &) = delete;
	Immovable &operator = (Immovable &&) = delete;
};

struct AutoHandle : Immovable
{
	HANDLE handle = 0;

	void close()
	{
		if (handle)
		{
			::CloseHandle(handle);
			handle = 0;
		}
	}

	~AutoHandle()
	{
		close();
	}
};

std::uint64_t convert(const FILETIME &ft)
{
	LARGE_INTEGER li = {};
	li.HighPart = ft.dwHighDateTime;
	li.LowPart = ft.dwLowDateTime;
	return li.QuadPart / 10;
}

DWORD run(const int argc, const char *args[])
{
	if (argc < 2)
		throw std::runtime_error("No program name.");

	const char *cmd = GetCommandLine();
	cmd = strchr(cmd + strlen(args[0]), ' '); // skip own executable path
	while (isspace(*cmd))
		cmd++;
	printf("Command: %s\n", cmd);
	//printf("-----------------------------------------\n");
	fflush(stdout);

	STARTUPINFO startupInfo = {};
	startupInfo.cb = sizeof(startupInfo);
	PROCESS_INFORMATION procInfo = {};

	if (!CreateProcess(nullptr, (char *)cmd, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startupInfo, &procInfo))
	{
		printf("Error code: %d\n", GetLastError());
		throw std::runtime_error("CreateProcess failed");
	}

	AutoHandle hp;
	hp.handle = procInfo.hProcess;
	AutoHandle ht;
	ht.handle = procInfo.hThread;

	if (WaitForSingleObject(procInfo.hProcess, INFINITE) != WAIT_OBJECT_0)
		throw std::runtime_error("WaitForSingleObject failed");

	{
		FILETIME c, e, k, u;
		if (GetProcessTimes(procInfo.hProcess, &c, &e, &k, &u) == 0)
		{
			printf("Error code: %d\n", GetLastError());
			throw std::runtime_error("GetProcessTimes failed");
		}
		const std::uint64_t duration = convert(e) - convert(c);
		const std::uint64_t hrs = duration / 1000000 / 60 / 60;
		const std::uint64_t min = (duration / 1000000 / 60) % 60;
		const std::uint64_t sec = (duration / 1000000) % 60;
		//printf("-----------------------------------------\n");
		printf("User time (microseconds): %lld\n", convert(u));
		printf("User time (seconds): %lld\n", convert(u) / 1000000);
		printf("System time (microseconds): %lld\n", convert(k));
		printf("System time (seconds): %lld\n", convert(k) / 1000000);
		printf("Wall time (microseconds): %lld\n", duration);
		printf("Wall time (seconds): %lld\n", duration / 1000000);
		printf("Wall time (H:MM:SS): %lld:%02lld:%02lld\n", hrs, min, sec);
		fflush(stdout);
	}

	{
		PROCESS_MEMORY_COUNTERS_EX pmc = {};
		if (GetProcessMemoryInfo(procInfo.hProcess, (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc)) == 0)
		{
			printf("Error code: %d\n", GetLastError());
			throw std::runtime_error("GetProcessMemoryInfo failed");
		}

		//printf("-----------------------------------------\n");
		printf("Peak working set size (kbytes): %lld\n", pmc.PeakWorkingSetSize / 1000);
		printf("Peak page file usage (kbytes): %lld\n", pmc.PeakPagefileUsage / 1000);
		printf("Page file usage (kbytes): %lld\n", pmc.PagefileUsage / 1000);
		printf("Private usage (kbytes): %lld\n", pmc.PrivateUsage / 1000);
		printf("Quota peak paged pool (kbytes): %lld\n", pmc.QuotaPeakPagedPoolUsage / 1000);
		printf("Quota peak nonpaged pool (kbytes): %lld\n", pmc.QuotaPeakNonPagedPoolUsage / 1000);
		printf("Page fault count: %d\n", pmc.PageFaultCount);
		fflush(stdout);
	}

	{
		DWORD code = 0;
		if (GetExitCodeProcess(procInfo.hProcess, &code) == 0)
		{
			printf("Error code: %d\n", GetLastError());
			throw std::runtime_error("GetExitCodeProcess failed");
		}
		//printf("-----------------------------------------\n");
		printf("Process exit code: %d\n", code);
		fflush(stdout);
		return code;
	}
}

int main(const int argc, const char *args[])
{
	try
	{
		return run(argc, args);
	}
	catch (const std::exception &e)
	{
		printf("Exception: %s\n", e.what());
		return 1;
	}
	catch (const char *e)
	{
		printf("Exception: %s\n", e);
		return 1;
	}
	catch (...)
	{
		printf("Unknown exception\n");
		return 1;
	}
}

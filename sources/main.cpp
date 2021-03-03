#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#include <psapi.h>

#include <cstdlib>
#include <cstdio>
#include <cerrno>

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

struct RealTimer : Immovable
{
	LARGE_INTEGER begin = {}, end = {};

	void start()
	{
		if (QueryPerformanceCounter(&begin) == 0)
		{
			printf("Error code: %d\n", GetLastError());
			throw std::runtime_error("QueryPerformanceCounter failed");
		}
	}

	void stop()
	{
		if (QueryPerformanceCounter(&end) == 0)
		{
			printf("Error code: %d\n", GetLastError());
			throw std::runtime_error("QueryPerformanceCounter failed");
		}
	}

	std::uint64_t duration() const
	{
		LARGE_INTEGER freq = {};
		if (QueryPerformanceFrequency(&freq) == 0)
		{
			printf("Error code: %d\n", GetLastError());
			throw std::runtime_error("QueryPerformanceFrequency failed");
		}
		return std::uint64_t(1000000) * (end.QuadPart - begin.QuadPart) / freq.QuadPart;
	}
} realTimer;

DWORD run(const int argc, const char *args[])
{
	if (argc < 2)
		throw std::runtime_error("No program name.");

	const char *cmd = GetCommandLine();
	cmd = strchr(cmd + strlen(args[0]), ' '); // skip own executable path
	while (*cmd == ' ')
		cmd++;
	printf("Command: %s\n", cmd);

	STARTUPINFO startupInfo = {};
	startupInfo.cb = sizeof(startupInfo);
	PROCESS_INFORMATION procInfo = {};

	realTimer.start();

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

	realTimer.stop();

	{
		const std::uint64_t duration = realTimer.duration();
		printf("Wall time (microseconds): %lld\n", duration);
		std::uint64_t sec = duration / 1000000;
		std::uint64_t min = sec / 60;
		std::uint64_t hrs = min / 60;
		printf("Wall time (seconds): %lld\n", sec);
		sec %= 60;
		min %= 60;
		printf("Wall time (H:MM:SS): %lld:%02lld:%02lld\n", hrs, min, sec);
	}

	{
		PROCESS_MEMORY_COUNTERS_EX pmc = {};
		if (GetProcessMemoryInfo(procInfo.hProcess, (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc)) == 0)
		{
			printf("Error code: %d\n", GetLastError());
			throw std::runtime_error("GetProcessMemoryInfo failed");
		}

		printf("Peak working set (kbytes): %lld\n", pmc.PeakWorkingSetSize / 1000);
		printf("Peak page file usage (kbytes): %lld\n", pmc.PeakPagefileUsage / 1000);
		printf("Private usage (kbytes): %lld\n", pmc.PrivateUsage / 1000);
		printf("Page faults count: %d\n", pmc.PageFaultCount);
	}

	{
		DWORD code = 0;
		if (GetExitCodeProcess(procInfo.hProcess, &code) == 0)
		{
			printf("Error code: %d\n", GetLastError());
			throw std::runtime_error("GetExitCodeProcess failed");
		}
		printf("Process exit code: %d\n", code);
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

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>

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

void printStats(HANDLE handle)
{
	{
		DWORD code = 0;
		if (GetExitCodeProcess(handle, &code) == 0)
		{
			printf("Error code: %d\n", GetLastError());
			throw std::runtime_error("GetExitCodeProcess failed");
		}
		printf("Process exit code: %d\n", code);
	}

	// todo
}

void run(const int argc, const char *args[])
{
	if (argc == 0)
		throw std::runtime_error("No program name.");

	std::string cmd = args[0];
	for (int i = 1; i < argc; i++)
	{
		cmd += " ";
		cmd += args[i];
	}

	STARTUPINFO startupInfo;
	memset(&startupInfo, 0, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);

	PROCESS_INFORMATION procInfo;
	memset(&procInfo, 0, sizeof(procInfo));

	if (!CreateProcess(nullptr, (char *)cmd.c_str(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startupInfo, &procInfo))
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

	printStats(procInfo.hProcess);
}

int main(const int argc, const char *args[])
{
	try
	{
		run(argc - 1, args + 1);
		return 0;
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

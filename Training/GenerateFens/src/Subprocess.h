#pragma once
#include "Types.h"

#ifdef STORM_PLATFORM_WINDOWS
#include <Windows.h>
#else
#include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>
#include <unistd.h>
#endif

namespace Storm
{

#ifdef STORM_PLATFORM_WINDOWS

	class STORM_API Subprocess
	{
	private:
		HANDLE m_ChildStdinRead;
		HANDLE m_ChildStdinWrite;
		HANDLE m_ChildStdoutRead;
		HANDLE m_ChildStdoutWrite;
		HANDLE m_Process;
		HANDLE m_Thread;

	public:
		Subprocess(const std::string& commandLine);
		Subprocess(const Subprocess& other) = delete;
		Subprocess& operator=(const Subprocess& other) = delete;
		Subprocess(Subprocess&& other);
		Subprocess& operator=(Subprocess&& other);
		~Subprocess();

		void Terminate();
		size_t ReadStdout(void* buffer, size_t size);
		bool WriteStdin(const void* buffer, size_t size);
	};

#else

	class STORM_API Subprocess
	{
	private:
		int m_ChildStdin[2];
		int m_ChildStdout[2];
		pid_t m_Process;

	public:
		Subprocess(const std::string& commandLine);
		Subprocess(const Subprocess& other) = delete;
		Subprocess& operator=(const Subprocess& other) = delete;
		Subprocess(Subprocess&& other);
		Subprocess& operator=(Subprocess&& other);
		~Subprocess();

		void Terminate();
		size_t ReadStdout(void* buffer, size_t size);
		bool WriteStdin(const void* buffer, size_t size);
		
	private:
		std::vector<std::string> ParseCommandLine(const std::string& cmdLine) const;
	};

#endif

}

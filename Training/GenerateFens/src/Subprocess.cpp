#include "Subprocess.h"

#include <sstream>
#include <iostream>

namespace Storm
{

#ifdef STORM_PLATFORM_WINDOWS

    Subprocess::Subprocess(const std::string& commandLine)
        : m_ChildStdinRead(NULL), m_ChildStdinWrite(NULL), m_ChildStdoutRead(NULL), m_ChildStdoutWrite(NULL), m_Process(NULL), m_Thread(NULL)
    {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = true;
        sa.lpSecurityDescriptor = NULL;

        if (!CreatePipe(&m_ChildStdoutRead, &m_ChildStdoutWrite, &sa, 0))
        {
            STORM_ASSERT(false, "Failed to create stdout pipe");
            return;
        }

        if (!SetHandleInformation(m_ChildStdoutRead, HANDLE_FLAG_INHERIT, 0))
        {
            STORM_ASSERT(false, "Failed to set handle information");
            return;
        }

        if (!CreatePipe(&m_ChildStdinRead, &m_ChildStdinWrite, &sa, 0))
        {
            STORM_ASSERT(false, "Failed to create stdin pipe");
            return;
        }

        if (!SetHandleInformation(m_ChildStdinWrite, HANDLE_FLAG_INHERIT, 0))
        {
            STORM_ASSERT(false, "Failed to set handle information");
            return;
        }

        std::wstring wcmdLine = std::wstring{ commandLine.begin(), commandLine.end() };
        PROCESS_INFORMATION pInfo;
        STARTUPINFO startup;
        ZeroMemory(&pInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&startup, sizeof(STARTUPINFO));

        startup.cb = sizeof(STARTUPINFO);
        startup.hStdError = m_ChildStdoutWrite;
        startup.hStdOutput = m_ChildStdoutWrite;
        startup.hStdInput = m_ChildStdinRead;
        startup.dwFlags |= STARTF_USESTDHANDLES;

        TCHAR* wCmdLineCopy = new TCHAR[wcmdLine.size() + 1];
        memcpy(wCmdLineCopy, wcmdLine.data(), wcmdLine.size() * sizeof(TCHAR));
        wCmdLineCopy[wcmdLine.size()] = '\0';

        bool success = CreateProcess(NULL, wCmdLineCopy, NULL, NULL, true, 0, NULL, NULL, &startup, &pInfo);
        delete[] wCmdLineCopy;

        if (!success)
        {
            STORM_ASSERT(false, "Failed to create process");
            return;
        }

        m_Process = pInfo.hProcess;
        m_Thread = pInfo.hThread;

        CloseHandle(m_ChildStdoutWrite);
        CloseHandle(m_ChildStdinRead);
    }

    Subprocess::Subprocess(Subprocess&& other)
        : m_ChildStdinRead(other.m_ChildStdinRead), m_ChildStdinWrite(other.m_ChildStdinWrite), m_ChildStdoutRead(other.m_ChildStdoutRead), m_ChildStdoutWrite(other.m_ChildStdoutWrite), m_Process(other.m_Process), m_Thread(other.m_Thread)
    {
        other.m_Process = NULL;
        other.m_Thread = NULL;
    }

    Subprocess& Subprocess::operator=(Subprocess&& other)
    {
        std::swap(m_ChildStdinRead, other.m_ChildStdinRead);
        std::swap(m_ChildStdinWrite, other.m_ChildStdinWrite);
        std::swap(m_ChildStdoutRead, other.m_ChildStdoutRead);
        std::swap(m_ChildStdoutWrite, other.m_ChildStdoutWrite);
        std::swap(m_Process, other.m_Process);
        std::swap(m_Thread, other.m_Thread);
        return *this;
    }

    Subprocess::~Subprocess()
    {
        if (m_Process != NULL)
        {
            Terminate();
        }
    }

    void Subprocess::Terminate()
    {
        if (m_Process != NULL)
        {
            TerminateProcess(m_Process, 0);
            CloseHandle(m_Process);
            CloseHandle(m_Thread);
        }
    }

    size_t Subprocess::ReadStdout(void* buffer, size_t size)
    {
        if (m_Process != NULL)
        {
            DWORD bytesRead;
            bool success = ReadFile(m_ChildStdoutRead, buffer, size, &bytesRead, NULL);
            return success ? bytesRead : 0;
        }
        return 0;
    }

    bool Subprocess::WriteStdin(const void* buffer, size_t size)
    {
        if (m_Process != NULL)
        {
            DWORD bytesWritten;
            bool success = WriteFile(m_ChildStdinWrite, buffer, size, &bytesWritten, NULL);
            return success;
        }
        return false;
    }

#else

    Subprocess::Subprocess(const std::string& commandLine)
        : m_ChildStdin(), m_ChildStdout(), m_Process(0)
    {
        int success = pipe(m_ChildStdin);
        STORM_ASSERT(success == 0, "Failed to create pipe");
        success = pipe(m_ChildStdout);
        STORM_ASSERT(success == 0, "Failed to create pipe");
        m_Process = fork();

        if (m_Process == 0)
        {
            // Child process
            dup2(m_ChildStdin[0], STDIN_FILENO);
            dup2(m_ChildStdout[1], STDOUT_FILENO);

            std::vector<std::string> parsedCmdLine = ParseCommandLine(commandLine);
            char** args = new char*[parsedCmdLine.size() + 1];
            for (int i = 0; i < parsedCmdLine.size(); i++)
            {
                args[i] = new char[parsedCmdLine[i].size() + 1];
                std::strcpy(args[i], parsedCmdLine[i].c_str());
            }
            args[parsedCmdLine.size()] = nullptr;

            execv(parsedCmdLine[0].c_str(), args);

            for (int i = 0; i < parsedCmdLine.size(); i++)
                delete[] args[i];
            delete[] args;
            
            STORM_ASSERT(false, "Failed to exec");
            std::exit(1);
        }
        else if (m_Process == -1)
        {
            STORM_ASSERT(false, "Failed to fork");
        }

        close(m_ChildStdin[0]);
        close(m_ChildStdout[1]);
    }

    Subprocess::Subprocess(Subprocess&& other)
        : m_ChildStdin{ other.m_ChildStdin[0], other.m_ChildStdin[1] }, m_ChildStdout{ other.m_ChildStdout[0], other.m_ChildStdout[1] }, m_Process(other.m_Process)
    {
        other.m_Process = 0;
    }

    Subprocess& Subprocess::operator=(Subprocess&& other)
    {
        std::swap(m_ChildStdin[0], other.m_ChildStdin[0]);
        std::swap(m_ChildStdin[1], other.m_ChildStdin[1]);
        std::swap(m_ChildStdout[0], other.m_ChildStdout[0]);
        std::swap(m_ChildStdout[1], other.m_ChildStdout[1]);
        std::swap(m_Process, other.m_Process);
        return *this;
    }

    Subprocess::~Subprocess()
    {
        if (m_Process != 0)
        {
            Terminate();
        }
    }

    void Subprocess::Terminate()
    {
        if (m_Process != 0)
        {
            int status;
            kill(m_Process, SIGKILL);
            waitpid(m_Process, &status, 0);
        }
    }

    size_t Subprocess::ReadStdout(void* buffer, size_t size)
    {
        if (m_Process != 0)
        {
            size_t bytesRead = read(m_ChildStdout[0], (char*)buffer, size);
            return bytesRead;
        }
        return 0;
    }

    bool Subprocess::WriteStdin(const void* buffer, size_t size)
    {
        if (m_Process != 0)
        {
            size_t bytesWritten = write(m_ChildStdin[1], (const char*)buffer, size);
            return bytesWritten == size;
        }
        return false;
    }

    std::vector<std::string> Subprocess::ParseCommandLine(const std::string& cmdLine) const
    {
        std::vector<std::string> result;
        std::stringstream ss;

        for (size_t i = 0; i < cmdLine.size(); i++)
        {
            char c = cmdLine[i];
            if (c == ' ')
            {
                if (!ss.str().empty())
                    result.push_back(ss.str());
                ss = std::stringstream();
            }
            else if (c == '"')
            {
                i++;
                while (i < cmdLine.size() && cmdLine[i] != '"')
                {
                    ss << cmdLine[i];
                    i++;
                }
            }
            else
            {
                ss << cmdLine[i];
            }
        }

        if (!ss.str().empty())
            result.push_back(ss.str());

        return result;
    }

#endif

}

#pragma once
#include "Logging.h"

#include <fstream>
#include <mutex>

namespace Storm
{

	class STORM_API ThreadSafeFileWriter
	{
	private:
		std::string m_Path;
		std::ofstream m_File;
		std::mutex m_Mutex;

	public:
		ThreadSafeFileWriter(const std::string& path);

		void WriteString(const std::string& data);
		void Complete();
	};

}

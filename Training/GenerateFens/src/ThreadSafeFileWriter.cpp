#include "ThreadSafeFileWriter.h"

namespace Storm
{

	ThreadSafeFileWriter::ThreadSafeFileWriter(const std::string& path)
		: m_Path(path), m_File(m_Path), m_Mutex()
	{
	}

	void ThreadSafeFileWriter::WriteString(const std::string& data)
	{
		std::scoped_lock<std::mutex> lock(m_Mutex);
		m_File.write(data.c_str(), data.size());
	}

	void ThreadSafeFileWriter::Complete()
	{
		std::scoped_lock<std::mutex> lock(m_Mutex);
		m_File.close();
	}

}

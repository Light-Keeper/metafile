/* Copyright (C) 2015, Vadym Ianushkevych ( vadik.ya@gmail.com )
 * All Rights Reserved.
 * You may use, distribute and modify this code under the terms
 * of the Simplified BSD License. You should have received a copy
 * of the Simplified BSD License with this file. If not, see
 * http://opensource.org/licenses/BSD-2-Clause
*/

#include "defaultfileaccess.h"
#include <io.h>

namespace metafile
{
	DefaultFileAccess::DefaultFileAccess()
	{
		m_file = nullptr;
	}


	DefaultFileAccess::~DefaultFileAccess()
	{
		if (m_file) fclose(m_file);
	}


	void DefaultFileAccess::UseFile(const std::string &name) 
	{
		if (m_file)
		{
			fclose(m_file);
			m_file = nullptr;
		}

		m_error.clear();
		auto f = fopen(name.c_str(), "rb+");

		if (f == nullptr)
		{
			f = fopen(name.c_str(), "ab+");
			if (f) fclose(f);

			f = fopen(name.c_str(), "rb+");
		}

		m_file = f;
		if (!m_file) m_error = std::string("Can not open file") + name;
	}

	bool DefaultFileAccess::IsValid() 
	{
		return m_file != nullptr;
	}

	std::string DefaultFileAccess::GetLastError() 
	{
		return m_error;
	}

	void DefaultFileAccess::SetPointerTo(uint64_t offset) 
	{
		if (!m_file) return;

		bool res = 0 == _fseeki64(m_file, offset, SEEK_SET);
		if (!res)
		{
			m_error = "_fseeki64 error " ;
			m_error += strerror(errno);
			fclose(m_file);
			m_file = nullptr;
		}
	}

	void DefaultFileAccess::SetFileSize(uint64_t size)
	{
		if (!m_file) return;
		int filedes = _fileno(m_file);
		_chsize_s(filedes, size);
	}

	uint32_t DefaultFileAccess::Read(void *buffer, uint32_t bufferSize)
	{
		if (!m_file) return 0;
		return fread(buffer, 1, bufferSize, m_file);
	}

	uint32_t DefaultFileAccess::Write(void *buffer, uint32_t bufferSize)
	{
		if (!m_file) return 0;
		uint32_t res = fwrite(buffer, 1, bufferSize, m_file);
	
		if (res != bufferSize)
		{
			m_error = "Write error";
			fclose(m_file);
			m_file = nullptr;
			return 0;
		}

		return res;
	}

	void DefaultFileAccess::Flush()
	{
		if (m_file) fflush(m_file);
	}

} // namespace

/* Copyright (C) 2015, Vadym Ianushkevych ( vadik.ya@gmail.com )
 * All Rights Reserved.
 * You may use, distribute and modify this code under the terms
 * of the Simplified BSD License. You should have received a copy
 * of the Simplified BSD License with this file. If not, see
 * http://opensource.org/licenses/BSD-2-Clause
*/

#include "filethread.h"
#include "metafileimpl.h"

namespace metafile
{
	FileThread::~FileThread()
	{
	}

	std::string FileThread::GetName()
	{
		return m_impl->FileThreadGetName(m_index);
	}

	uint64_t FileThread::GetSize()
	{
		return m_impl->FileThreadGetSize(m_index);
	}

	void FileThread::SetSize(uint64_t newFileSize)
	{
		m_impl->FileThreadSetSize(m_index, newFileSize);
	}

	uint32_t FileThread::Write(void *data, uint32_t size)
	{
		return m_impl->FileThreadWrite(m_index, data, size);
	}

	uint32_t FileThread::Read(void *data, uint32_t size)
	{
		return m_impl->FileThreadRead(m_index, data, size);
	}

	void FileThread::SetPointerTo(uint64_t pos)
	{
		return m_impl->FileThreadSetPointerTo(m_index, pos);
	}

} // namespace

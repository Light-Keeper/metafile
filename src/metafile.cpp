/* Copyright (C) 2015, Vadym Ianushkevych ( vadik.ya@gmail.com )
 * All Rights Reserved.
 * You may use, distribute and modify this code under the terms
 * of the Simplified BSD License. You should have received a copy
 * of the Simplified BSD License with this file. If not, see
 * http://opensource.org/licenses/BSD-2-Clause
*/

#include <memory>
#include "metafile.h"
#include "metafileimpl.h"
#include "filethread.h"

namespace metafile
{
	Metafile::Metafile()
	{
		m_impl = std::make_shared<MetafileImpl>();
	}

	Metafile::~Metafile()
	{
	}

	bool Metafile::IsValid()
	{
		return m_impl->IsValid();
	}

	std::string Metafile::GetLastError()
	{
		return m_impl->GetLastError();
	}

	std::vector< FileThread* > Metafile::GetAllFileThreads()
	{
		auto threads = m_impl->GetRefToAllThreads();
		std::vector<FileThread *> result;

		for (auto &item : threads)
		{
			result.push_back(&*item);
		}

		return result;
	}

	FileThread* Metafile::GetFileThread(const std::string &name)
	{
		auto &thread = m_impl->GetRefToAllThreads();
		for (auto &item : thread)
		{
			if (item->GetName() == name) return &*item;
		}

		return nullptr;
	}

	void Metafile::SetFileAccessInterface(const std::shared_ptr<FileAccessInterface> &file)
	{
		m_impl->SetFileAccessInterface(file);
	}

	void Metafile::Init()
	{
		m_impl->Init();
	}

	void Metafile::InitEmpty(const std::vector<std::string> &threadNames)
	{
		m_impl->InitEmpty(threadNames);
	}

} // namespace


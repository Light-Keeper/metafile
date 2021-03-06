/* Copyright (C) 2015, Vadym Ianushkevych ( vadik.ya@gmail.com )
 * All Rights Reserved.
 * You may use, distribute and modify this code under the terms
 * of the Simplified BSD License. You should have received a copy
 * of the Simplified BSD License with this file. If not, see
 * http://opensource.org/licenses/BSD-2-Clause
*/

#pragma once
#include <memory>
#include <string>
#include <vector>
#include "fileaccessinterface.h"

namespace metafile {

	class FileThread;
	class MetafileImpl;

	class Metafile
	{
	public:
		Metafile();
		~Metafile();

		// if file is invalid, it's impossible to work with it
		bool IsValid();

		// "File not found", etc.
		std::string GetLastError();

		//FileThread* is valid as long as Metafile is valid
		std::vector<FileThread *> GetAllFileThreads();
		FileThread* GetFileThread(const std::string &name);
		void Flush();

	private:
		std::shared_ptr<MetafileImpl> m_impl;

		void SetFileAccessInterface(const std::shared_ptr<FileAccessInterface> &file);
		void Init();
		void InitEmpty(const std::vector<std::string> &threadNames);

		friend class MetafileLib;
	};

} // namespace

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
#include <stdint.h>

namespace metafile {

	class FileAccessInterface
	{
	public:
		virtual ~FileAccessInterface(){};

		virtual void UseFile(const std::string &name) = 0;
		virtual bool IsValid() = 0;
		virtual std::string GetLastError() = 0;
		virtual void SetPointerTo(uint64_t offset) = 0;
		virtual void SetFileSize(uint64_t) = 0;
		virtual uint32_t Read(void *buffer, uint32_t bufferSize) = 0;
		virtual uint32_t Write(void *buffer, uint32_t bufferSize) = 0;
	};


	class FileAccessInterfaceAbstractFactory
	{
	public:
		virtual std::shared_ptr<FileAccessInterface> CreateFile() = 0;
	};

} // namespace
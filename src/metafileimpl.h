/* Copyright (C) 2015, Vadym Ianushkevych ( vadik.ya@gmail.com )
 * All Rights Reserved.
 * You may use, distribute and modify this code under the terms
 * of the Simplified BSD License. You should have received a copy
 * of the Simplified BSD License with this file. If not, see
 * http://opensource.org/licenses/BSD-2-Clause
*/

#pragma once
#include <memory>
#include <vector>
#include "fileaccessinterface.h"
#include "filethread.h"
#include "layout.h"

namespace metafile {

	class MetafileImpl
	{
	public:
		MetafileImpl();
		~MetafileImpl();

		void SetFileAccessInterface(const  std::shared_ptr<FileAccessInterface> &fileAccess);
		void Init();
		void InitEmpty(const std::vector<std::string> &threadNames);

		bool IsValid();
		std::string GetLastError();
		std::vector< FileThread *> GetRefToAllThreads();
		void FlushToDisk();

		// threads

		std::string FileThreadGetName(uint32_t index);
		uint64_t	FileThreadGetSize(uint32_t index);
		bool		FileThreadSetSize(uint32_t index, uint64_t newFileSize);
		uint32_t	FileThreadWrite(uint32_t index, void *data, uint32_t size);
		uint32_t	FileThreadRead(uint32_t index, void *data, uint32_t size);
		void		FileThreadSetPointerTo(uint32_t index, uint64_t pos);

	private:

		struct RuntimeThreadInfo
		{
			FileThreadInfo header;
			FileThread interfaceObject;
			uint64_t currentOffset;
		};

		struct RuntimeFileInfo
		{
			MetafileHeader header;
			std::vector<RuntimeThreadInfo> threads;
		};

		typedef uint32_t(FileAccessInterface:: * IoOperationFunction)(void *buffer, uint32_t bufferSize);

		uint32_t FileIoOperation(uint32_t index, void *data, uint32_t size, IoOperationFunction operation);
		uint64_t FindAddressToAppendNewBlock();
		uint64_t GetBlockSizeByIndex(uint32_t index);
		bool	 GetBlockByAddress(uint64_t address, uint32_t &block, uint64_t &offsetInBlock);

		std::shared_ptr<FileAccessInterface> m_fileAccess;
		RuntimeFileInfo m_file;
		std::string m_errorMessage;
	};

} // namespace

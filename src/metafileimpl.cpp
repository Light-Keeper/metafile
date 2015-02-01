/* Copyright (C) 2015, Vadym Ianushkevych ( vadik.ya@gmail.com )
 * All Rights Reserved.
 * You may use, distribute and modify this code under the terms
 * of the Simplified BSD License. You should have received a copy
 * of the Simplified BSD License with this file. If not, see
 * http://opensource.org/licenses/BSD-2-Clause
*/

#include "metafileimpl.h"
#include <algorithm>
#include <assert.h>

namespace metafile
{
	MetafileImpl::MetafileImpl()
	{
	};

	MetafileImpl::~MetafileImpl()
	{
		FlushToDisk();
	};

	void MetafileImpl::SetFileAccessInterface(const  std::shared_ptr<FileAccessInterface> &fileAccess)
	{
		m_fileAccess = fileAccess;
	}

	void MetafileImpl::Init()
	{
		assert(m_fileAccess);

		m_fileAccess->SetPointerTo(0);
		m_fileAccess->Read(&m_file.header, sizeof(m_file.header));

		m_errorMessage = m_fileAccess->GetLastError();
		if (!m_errorMessage.empty()) return;

		if (m_file.header.signature != MetafileHeader::kSignature ||
			m_file.header.numberOfThreads > MetafileHeader::kMaxNumberOfThreads)
		{
			m_errorMessage = "Is not a metafile";
			return;
		}

		m_file.threads.resize(m_file.header.numberOfThreads);

		for (uint32_t i = 0; i < m_file.threads.size(); i++)
		{
			auto &item = m_file.threads[i];
			m_fileAccess->Read(&item.header, sizeof(item.header));
			item.interfaceObject.m_impl = this;
			item.interfaceObject.m_index = i;
			item.currentOffset = 0;
		}

		m_errorMessage = m_fileAccess->GetLastError();
	}

	void MetafileImpl::InitEmpty(const std::vector<std::string> &threadNames)
	{
		assert(m_fileAccess);
		m_fileAccess->SetPointerTo(0);

		memset(&m_file.header, 0, sizeof(m_file.header));
		m_file.header.signature = MetafileHeader::kSignature;
		m_file.header.numberOfThreads = threadNames.size();
		m_file.header.sizeOfCluster = MetafileHeader::kDefaultClusterSize;

		m_fileAccess->Write(&m_file.header, sizeof(m_file.header));

		m_file.threads.resize(m_file.header.numberOfThreads);

		for (uint32_t i = 0; i < m_file.threads.size(); i++)
		{
			auto &item = m_file.threads[i];

			memset(&item.header, 0, sizeof(item.header));
			strncpy(item.header.name, threadNames[i].c_str(), sizeof(item.header.name) - 1);
			item.header.size = 0;

			item.interfaceObject.m_impl = this;
			item.interfaceObject.m_index = i;

			item.currentOffset = 0;
		}

		FlushToDisk();
	}

	bool MetafileImpl::IsValid()
	{
		return m_errorMessage.empty();
	}

	std::string MetafileImpl::GetLastError()
	{
		return m_errorMessage;
	}

	std::vector< FileThread *> MetafileImpl::GetRefToAllThreads()
	{
		std::vector< FileThread *> res;
		for (auto &item : m_file.threads)
		{
			res.push_back(&item.interfaceObject);
		}

		return res;
	}

	void MetafileImpl::FlushToDisk()
	{
		m_fileAccess->SetPointerTo(0);
		m_fileAccess->Write(&m_file.header, sizeof(MetafileHeader));

		for (auto &item : m_file.threads)
		{
			m_fileAccess->Write(&item.header, sizeof(FileThreadInfo));
		}

		m_errorMessage = m_fileAccess->GetLastError();
	}

	std::string MetafileImpl::FileThreadGetName(uint32_t index)
	{
		assert(index < m_file.threads.size());
		return std::string(m_file.threads[index].header.name);
	}

	uint64_t MetafileImpl::FileThreadGetSize(uint32_t index)
	{
		assert(index < m_file.threads.size());
		return m_file.threads[index].header.size;
	}

	bool MetafileImpl::FileThreadSetSize(uint32_t index, uint64_t newFileSize)
	{
		assert(index < m_file.threads.size());
		RuntimeThreadInfo &item = m_file.threads[index];
		item.header.size = newFileSize;

		uint32_t blockNumber;
		uint64_t offsetInBlock;

		bool res = GetBlockByAddress(item.header.size, blockNumber, offsetInBlock);
		if (!res)	return false;

		if (offsetInBlock != 0) blockNumber++;

		while (blockNumber < FileThreadInfo::kNumberOfBlockRecords && item.header.blocks[blockNumber].offsetInUnderlyingFile != 0)
		{
			item.header.blocks[blockNumber].offsetInUnderlyingFile = 0;
			blockNumber++;
		}

		uint64_t futherstPosInFile = FindAddressToAppendNewBlock();
		m_fileAccess->SetFileSize(futherstPosInFile);
		return true;
	}


	uint32_t MetafileImpl::FileThreadWrite(uint32_t index, void *data, uint32_t size)
	{
		assert(index < m_file.threads.size());
		RuntimeThreadInfo &item = m_file.threads[index];

		if (item.currentOffset + size > item.header.size)
		{
			item.header.size = item.currentOffset + size;
		}

		return FileIoOperation(index, data, size, &FileAccessInterface::Write);
	}

	uint32_t MetafileImpl::FileThreadRead(uint32_t index, void *data, uint32_t size)
	{
		assert(index < m_file.threads.size());
		RuntimeThreadInfo &item = m_file.threads[index];

		if (item.currentOffset + size > item.header.size)
		{
			assert(item.currentOffset <= item.header.size);
			size = static_cast<uint32_t>(item.header.size - item.currentOffset);
		}

		return FileIoOperation(index, data, size, &FileAccessInterface::Read);
	}

	uint32_t MetafileImpl::FileIoOperation(uint32_t index, void *data, uint32_t size, MetafileImpl::IoOperationFunction operation)
	{
		assert(index < m_file.threads.size());
		RuntimeThreadInfo &item = m_file.threads[index];

		uint32_t actuallyProcessed = 0;
		char *_data = (char *)data;

		uint32_t blockNumber;
		uint64_t offsetInBlock;
		bool res = GetBlockByAddress(item.currentOffset, blockNumber, offsetInBlock);
		if (!res)	return false;

		while (actuallyProcessed < size && m_fileAccess->IsValid())
		{
			for (unsigned i = 0; i <= blockNumber && item.header.blocks[blockNumber].offsetInUnderlyingFile == 0; i++)
			{
				if (item.header.blocks[i].offsetInUnderlyingFile != 0) continue;
				item.header.blocks[i].offsetInUnderlyingFile = FindAddressToAppendNewBlock();
			}

			uint64_t blockSize = GetBlockSizeByIndex(blockNumber);
			uint64_t sizeToEndOfBlock = blockSize - offsetInBlock;
			uint32_t sizeToProcess = (uint32_t)std::min(sizeToEndOfBlock, (uint64_t)(size - actuallyProcessed));

			m_fileAccess->SetPointerTo(item.header.blocks[blockNumber].offsetInUnderlyingFile + offsetInBlock);

			(&*m_fileAccess->*operation)(_data + actuallyProcessed, sizeToProcess);
			actuallyProcessed += sizeToProcess;
			item.currentOffset += sizeToProcess;

			blockNumber++;
			offsetInBlock = 0;
		}

		m_errorMessage = m_fileAccess->GetLastError();
		return actuallyProcessed;
	}

	void MetafileImpl::FileThreadSetPointerTo(uint32_t index, uint64_t pos)
	{
		assert(index < m_file.threads.size());
		RuntimeThreadInfo &item = m_file.threads[index];
		item.currentOffset = pos;
	}

	uint64_t MetafileImpl::FindAddressToAppendNewBlock()
	{
		uint64_t ans = sizeof(MetafileHeader) + sizeof(FileThreadInfo) * m_file.threads.size();

		for (auto &item : m_file.threads)
		{
			uint32_t sizeOfBlocksArray = 0;

			for (uint32_t i = 0; i < sizeof(item.header.blocks); i++)
			{
				if (item.header.blocks[i].offsetInUnderlyingFile == 0)
				{
					sizeOfBlocksArray = i;
					break;
				}
			}

			if (sizeOfBlocksArray == 0) continue;

			uint64_t candidate = item.header.blocks[sizeOfBlocksArray - 1].offsetInUnderlyingFile
				+ GetBlockSizeByIndex(sizeOfBlocksArray - 1);

			if (ans < candidate) ans = candidate;
		}

		return ans;
	}

	uint64_t MetafileImpl::GetBlockSizeByIndex(uint32_t index)
	{
		if (index == 0) return m_file.header.sizeOfCluster;
		if (index < 5) return index * m_file.header.sizeOfCluster;

		uint32_t currentIndex = 4;
		uint64_t val = 4;

		int stepNumber = 0;
		while (currentIndex < index)
		{
			if (stepNumber % 2 == 0) val = val / 2 * 3;
			else val = val / 3 * 4;

			stepNumber++;
			currentIndex++;
		}

		return val * m_file.header.sizeOfCluster;
	}

	bool MetafileImpl::GetBlockByAddress(uint64_t address, uint32_t &block, uint64_t &offsetInBlock)
	{
		block = 0;
		uint64_t currertStartAddress = 0;

		while (block < FileThreadInfo::kNumberOfBlockRecords)
		{
			uint64_t currentSize = GetBlockSizeByIndex(block);

			if (currertStartAddress + currentSize <= address)
			{
				block++;
				currertStartAddress += currentSize;
				continue;
			}

			offsetInBlock = address - currertStartAddress;
			return true;
		}

		return false;
	}

} // namespace


/* Copyright (C) 2015, Vadym Ianushkevych ( vadik.ya@gmail.com )
 * All Rights Reserved.
 * You may use, distribute and modify this code under the terms
 * of the Simplified BSD License. You should have received a copy
 * of the Simplified BSD License with this file. If not, see
 * http://opensource.org/licenses/BSD-2-Clause
*/

#include "MetafileImpl.h"
#include <algorithm>
#include <assert.h>

MetafileImpl::MetafileImpl(){};

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
		m_file.header.numberOfThreads > MetafileHeader::kMaxNumberOfThreads )
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

		item.ReadPositionInThread = 0xFFFFFFFF;
		item.CurrentReadBlock = 0xFFFFFFFF;
		item.CurrentWriteBlock = 0xFFFFFFFF;
		item.CurrentWriteClusterIndex = 0xFFFFFFFF;
	}
	
	for (uint32_t i = 0; i < m_file.threads.size(); i++)
	{
		FileThreadSetPointerTo(i, 0);
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
	m_file.header.clustersPerGroup = MetafileHeader::kDefaultClustersPerGroup;

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
		
		item.ReadPositionInThread = 0xFFFFFFFF;
		item.CurrentReadBlock = 0xFFFFFFFF;
		item.CurrentReadClusterIndex= 0xFFFFFFFF;
	}
	
	for (uint32_t i = 0; i < m_file.threads.size(); i++)
	{
		auto &item = m_file.threads[i];
		item.header.blocks[0].offsetInUnderlyingFile = FindAddressToAppendNewBlock();
		item.header.blocks[0].offsetInThread = 0;

		FileThreadSetPointerTo(i, 0);
		item.CurrentReadClusters[0].setOffsetInFile(0);
		item.prefetchedOffset = 0;

		InitWriteBuffer(i);
		item.CurrentWriteClusters[0].setOffsetInFile(0);

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
	m_fileAccess->SetPointerTo(sizeof(MetafileHeader));

	for (auto &item : m_file.threads)
	{
		m_fileAccess->Write(&item.header, sizeof(FileThreadInfo));
	}

	for (auto &item : m_file.threads)
	{
		if (item.CurrentWriteBlock != 0xFFFFFFFF)
		{
			assert(item.CurrentWriteClusters.size() > 0);

			// order is important!
			// FlushWriteBuffer also updates item.CurrentWriteClusters
			FlushWriteBuffer(&item - &m_file.threads[0]);

			m_fileAccess->SetPointerTo(item.header.blocks[item.CurrentWriteBlock].offsetInUnderlyingFile);
			m_fileAccess->Write(&item.CurrentWriteClusters[0], sizeof(ClusterInfo) * item.CurrentWriteClusters.size());

		}
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

void MetafileImpl::FileThreadSetSize(uint32_t index, uint64_t newFileSize)
{
	assert(index < m_file.threads.size());
	RuntimeThreadInfo &item = m_file.threads[index];

	if (newFileSize > item.header.size)
	{
		uint32_t sizeToAppend = static_cast<uint32_t>(newFileSize - item.header.size);
		void *stub = malloc(sizeToAppend);
		FileThreadAppend(index, nullptr, sizeToAppend);
		free(stub);
	} 

	if (newFileSize < item.header.size)
	{
		uint64_t sizeToRemove = item.header.size - newFileSize;

		//1 - remove from write buffer

		uint32_t removeFromWriteBuffer = (uint32_t)std::min((uint64_t)item.writeBuffer.size(), sizeToRemove);
		item.writeBuffer.resize(item.writeBuffer.size() - removeFromWriteBuffer);
		sizeToRemove -= removeFromWriteBuffer;
		item.header.size -= removeFromWriteBuffer;

		//2 - remove blocks

		item.header.size = newFileSize;
		RemoveExtraBlocks(index);

		//3 - reinit write location

		if (item.writeBuffer.empty() && !item.CurrentWriteClusters.empty())
		{
			InitWriteBuffer(index);
		}

		//4 - reinit read cache

		Prefetch(index);
	}
}

bool MetafileImpl::FileThreadAppend(uint32_t index, void *data, uint32_t size)
{
	assert(index < m_file.threads.size());
	RuntimeThreadInfo &item = m_file.threads[index];

	if (item.CurrentWriteClusters.empty())
	{
		InitWriteBuffer(index);
	}

	uint32_t actuallyWritten = 0;
	char *_data = (char *)data;

	while (actuallyWritten < size && m_fileAccess->IsValid())
	{
		assert(item.CurrentWriteClusterIndex < item.CurrentWriteClusters.size());

		uint32_t clustersToEndOfBlock = item.CurrentWriteClusters.size() - item.CurrentWriteClusterIndex;
		uint32_t maxSizeInBuffer = std::min(m_file.header.clustersPerGroup, clustersToEndOfBlock) * m_file.header.sizeOfCluster;

		assert(maxSizeInBuffer > item.writeBuffer.size());

		uint32_t freeSizeInBuffer = maxSizeInBuffer - item.writeBuffer.size();	
		uint32_t sizeToWrite = std::min(freeSizeInBuffer, size - actuallyWritten);

		// if and of file is prefetched, append this data to read buffer

		if (item.prefetchedOffset + item.prefetcedAndDecoded.size() > item.header.size
			&& item.CurrentReadBlock == item.CurrentWriteBlock
			&& item.CurrentWriteClusterIndex == item.CurrentReadClusterIndex)
		{
			assert(item.header.size >= item.prefetchedOffset);
			uint32_t endOffsetInReadBuffer = static_cast<uint32_t>(item.header.size - item.prefetchedOffset);
			uint32_t freeSpaceInReadBuffer = item.prefetcedAndDecoded.size() - endOffsetInReadBuffer;
			uint32_t sizeToCopyToReadBuffer = std::min(freeSpaceInReadBuffer, sizeToWrite);

			item.prefetcedAndDecoded.insert(item.prefetcedAndDecoded.end(), _data + actuallyWritten, _data + actuallyWritten + sizeToCopyToReadBuffer);
		}

		item.header.size += sizeToWrite;

		item.writeBuffer.insert(item.writeBuffer.end(), _data + actuallyWritten, _data + actuallyWritten + sizeToWrite);
		actuallyWritten += sizeToWrite;

		assert(item.writeBuffer.size() <= maxSizeInBuffer);

		if (item.writeBuffer.size() == maxSizeInBuffer)
		{
			FlushWriteBuffer(index);
		}
	}

	m_errorMessage = m_fileAccess->GetLastError();
	return actuallyWritten == size && IsValid();
}

uint32_t MetafileImpl::FileThreadRead(uint32_t index, void *data, uint32_t size)
{
	assert(index < m_file.threads.size());
	RuntimeThreadInfo &item = m_file.threads[index];

	uint32_t actuallyRead = 0;
	char *_data = (char *)data;

	while (actuallyRead < size && item.ReadPositionInThread < item.header.size)
	{
		uint32_t readFromPrefetchedOffset = static_cast<uint32_t>(item.ReadPositionInThread - item.prefetchedOffset);
		assert(readFromPrefetchedOffset < item.prefetcedAndDecoded.size());

		uint32_t sizeToCopy = std::min(item.prefetcedAndDecoded.size() - readFromPrefetchedOffset, size - actuallyRead);
		memcpy(_data + actuallyRead, &item.prefetcedAndDecoded[readFromPrefetchedOffset], sizeToCopy);
		actuallyRead += sizeToCopy;
		item.ReadPositionInThread += sizeToCopy;

		if (item.ReadPositionInThread == item.prefetchedOffset + item.prefetcedAndDecoded.size())
		{
			Prefetch(index);
		}
	}

	return actuallyRead;
}

void MetafileImpl::FileThreadSetPointerTo(uint32_t index, uint64_t pos)
{
	assert(index < m_file.threads.size());
	RuntimeThreadInfo &item = m_file.threads[index];	
	item.ReadPositionInThread = pos;
	Prefetch(index);		
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
			+ GetClustersPerBlockByIndex(sizeOfBlocksArray - 1) * (m_file.header.sizeOfCluster + sizeof(ClusterInfo));

		if (ans < candidate) ans = candidate;
	}

	return ans;
}

void MetafileImpl::Prefetch(uint32_t index)
{
	assert(index < m_file.threads.size());
	RuntimeThreadInfo &item = m_file.threads[index];

	LoadClustersByAddress(index,	item.ReadPositionInThread, item.CurrentReadBlock, 
									item.CurrentReadClusterIndex, item.CurrentReadClusters );

	item.prefetchedOffset = item.CurrentReadClusters[item.CurrentReadClusterIndex].getOffsetInFile();
	item.prefetcedAndDecoded.clear();
	
	uint64_t dataOffsetInUnderlyingFile = item.header.blocks[item.CurrentReadBlock].offsetInUnderlyingFile
		+ m_file.header.sizeOfCluster * item.CurrentReadClusterIndex + sizeof(ClusterInfo) * GetClustersPerBlockByIndex(item.CurrentReadBlock);

	m_fileAccess->SetPointerTo(dataOffsetInUnderlyingFile);

	uint32_t clustersProcessed = 0;

	while (item.CurrentReadClusterIndex + clustersProcessed < item.CurrentReadClusters.size()
		&& item.prefetcedAndDecoded.size() < MetafileHeader::kPrefetchClusters * m_file.header.sizeOfCluster)
	{
		
		ClusterInfo *cluster = &item.CurrentReadClusters[item.CurrentReadClusterIndex + clustersProcessed];

		if (cluster->isPartOfCompressed())
		{
			m_errorMessage = "Invalid metafile. Part of compressed group was not expected";
			return;
		}

		if (cluster->isPlainData())
		{
			item.prefetcedAndDecoded.resize(item.prefetcedAndDecoded.size() + m_file.header.sizeOfCluster);
			m_fileAccess->Read(&item.prefetcedAndDecoded[item.prefetcedAndDecoded.size() - m_file.header.sizeOfCluster], m_file.header.sizeOfCluster);
			clustersProcessed++;
			continue;
		} 

		if (cluster->isCompressed())
		{
			uint32_t maxClustersInCompressedGroup = item.CurrentReadClusters.size() - item.CurrentReadClusterIndex - clustersProcessed;
			uint32_t maxCompressedSize = maxClustersInCompressedGroup * m_file.header.sizeOfCluster;
			uint32_t compressedSize = cluster->getCompressedSize();
			uint32_t compressedClusters = compressedSize / m_file.header.sizeOfCluster + (compressedSize % m_file.header.sizeOfCluster != 0);

			if (compressedSize > maxCompressedSize || compressedSize == 0)
			{
				m_errorMessage = "Invalid metafile. compressedSize > maxCompressedSize || compressedSize == 0";
				return;
			}

			assert(compressedClusters * m_file.header.sizeOfCluster >= compressedSize);

			std::vector<char> readBuffer(compressedClusters * m_file.header.sizeOfCluster);
			m_fileAccess->Read(&readBuffer[0], readBuffer.size());
			readBuffer.resize(compressedSize);

			std::vector<char> decpmpressed = Decompress(&readBuffer[0], compressedSize);

			if (decpmpressed.empty())
			{
				m_errorMessage = "zipped file content is damaged";
				return;
			}

			item.prefetcedAndDecoded.insert(item.prefetcedAndDecoded.end(), decpmpressed.begin(), decpmpressed.end() );

			clustersProcessed += compressedClusters;
			continue;
		}


		// we shold'm be here
		m_errorMessage = "incorrect file format";
		return;
	}
}

void MetafileImpl::LoadClustersByAddress(uint32_t index, uint64_t address, uint32_t &block, uint32_t &cluster, std::vector<ClusterInfo> &clusters)
{
	assert(index < m_file.threads.size());
	RuntimeThreadInfo &item = m_file.threads[index];
	
	if (address > item.header.size) address = item.header.size;

	uint32_t currentBlock = 0;
	while (currentBlock < FileThreadInfo::kNumberOfBlockRecords && item.header.blocks[currentBlock].offsetInThread <= address
		&& item.header.blocks[currentBlock].offsetInUnderlyingFile != 0)
	{
		currentBlock++;
	}

	if (currentBlock == FileThreadInfo::kNumberOfBlockRecords || currentBlock == 0)
	{
		m_errorMessage = "too big or invalid metafile";
		return;
	}

	currentBlock--;
	
	if (currentBlock != block)
	{
		if (currentBlock == item.CurrentWriteBlock)
		{
			assert(&block == &item.CurrentReadBlock);
			FlushToDisk();
		}

		clusters.resize((uint32_t)GetClustersPerBlockByIndex(currentBlock));
		m_fileAccess->SetPointerTo(item.header.blocks[currentBlock].offsetInUnderlyingFile);
		m_fileAccess->Read(&clusters[0], clusters.size() * sizeof(ClusterInfo));
	}

	block = currentBlock;
	
	uint32_t left = 0;
	uint32_t right = clusters.size();
	
	while (left + 1 != right)
	{
		uint32_t center = (left + right) / 2;
		if (clusters[center].getOffsetInFile() > address)
		{
			right = center;
		}
		else
		{
			left = center;
		}
	}

	while (left > 0 && clusters[left].isPartOfCompressed())
	{
		left--;
	}

	cluster = left;
}

void MetafileImpl::FlushWriteBuffer(uint32_t index)
{
	assert(index < m_file.threads.size());
	RuntimeThreadInfo &item = m_file.threads[index];

	if (item.writeBuffer.empty()) return;

	uint32_t completeClusters = item.writeBuffer.size() / m_file.header.sizeOfCluster;
	uint32_t totalClusters = completeClusters + (item.writeBuffer.size() % m_file.header.sizeOfCluster != 0);

	std::vector<char> compressedData;
	uint32_t clustersToSave = completeClusters;
	char *dataToSave = &item.writeBuffer[0];
	bool isUsingCompressedData = false;
	uint32_t compressedStreamSize = 0;
	uint64_t startingOffsetInFileForBuffer = item.CurrentWriteClusters[item.CurrentWriteClusterIndex].getOffsetInFile();
	uint64_t startingOffsetInUnderlyingFile = 
		item.header.blocks[item.CurrentWriteBlock].offsetInUnderlyingFile 
		+ GetClustersPerBlockByIndex(item.CurrentWriteBlock) * sizeof(ClusterInfo)
		+ item.CurrentWriteClusterIndex * m_file.header.sizeOfCluster;

	compressedData = Compress(&item.writeBuffer[0], completeClusters * m_file.header.sizeOfCluster);	
	compressedStreamSize = compressedData.size();

	uint32_t compressedClusters = compressedData.size() / m_file.header.sizeOfCluster
		+ (compressedData.size() % m_file.header.sizeOfCluster != 0);
	
	if (compressedClusters < completeClusters && compressedStreamSize != 0)
	{
		clustersToSave = compressedClusters;
		compressedData.resize(compressedClusters *  m_file.header.sizeOfCluster);
		dataToSave = &compressedData[0];
		isUsingCompressedData = true;
	}

	m_fileAccess->SetPointerTo(startingOffsetInUnderlyingFile);
	m_fileAccess->Write(dataToSave, clustersToSave * m_file.header.sizeOfCluster);
	
	for (uint32_t i = 0; i < clustersToSave; i++)
	{
		if (isUsingCompressedData)
		{
			if (i == 0)
				item.CurrentWriteClusters[item.CurrentWriteClusterIndex].setCompressed(compressedStreamSize);
			else
				item.CurrentWriteClusters[item.CurrentWriteClusterIndex].setPartOfCompressed();
		}
		else
		{
			item.CurrentWriteClusters[item.CurrentWriteClusterIndex].setPlainData();
		}

		if (item.CurrentReadBlock == item.CurrentWriteBlock)
		{
			item.CurrentReadClusters[item.CurrentWriteClusterIndex] = item.CurrentWriteClusters[item.CurrentWriteClusterIndex];
		}

		item.CurrentWriteClusterIndex++;

		if (item.CurrentWriteClusterIndex < item.CurrentWriteClusters.size())
		{
			if (isUsingCompressedData) 
				item.CurrentWriteClusters[item.CurrentWriteClusterIndex].setOffsetInFile(startingOffsetInFileForBuffer);
			else
				item.CurrentWriteClusters[item.CurrentWriteClusterIndex]
				.setOffsetInFile(item.CurrentWriteClusters[item.CurrentWriteClusterIndex - 1].getOffsetInFile() + m_file.header.sizeOfCluster);
			
			if (item.CurrentReadBlock == item.CurrentWriteBlock)
			{
				item.CurrentReadClusters[item.CurrentWriteClusterIndex] = item.CurrentWriteClusters[item.CurrentWriteClusterIndex];
			}
		}
	}

	if (totalClusters > completeClusters)
	{
		item.CurrentWriteClusters[item.CurrentWriteClusterIndex].setOffsetInFile(startingOffsetInFileForBuffer + completeClusters * m_file.header.sizeOfCluster);
		item.CurrentWriteClusters[item.CurrentWriteClusterIndex].setPlainData();

		if (item.CurrentReadBlock == item.CurrentWriteBlock)
		{
			item.CurrentReadClusters[item.CurrentWriteClusterIndex] = item.CurrentWriteClusters[item.CurrentWriteClusterIndex];
		}

		m_fileAccess->Write(&item.writeBuffer[completeClusters * m_file.header.sizeOfCluster], 
			item.writeBuffer.size() - completeClusters * m_file.header.sizeOfCluster);
	
		item.writeBuffer.erase(item.writeBuffer.begin(), item.writeBuffer.begin() + completeClusters * m_file.header.sizeOfCluster);
	}
	else
	{
		assert(item.CurrentWriteClusterIndex <= item.CurrentWriteClusters.size());
		assert(totalClusters == completeClusters);

		if (item.CurrentWriteClusterIndex == item.CurrentWriteClusters.size())
		{
			// append new block

			m_fileAccess->SetPointerTo(item.header.blocks[item.CurrentWriteBlock].offsetInUnderlyingFile);
			m_fileAccess->Write(&item.CurrentWriteClusters[0], sizeof(ClusterInfo) * item.CurrentWriteClusters.size());

			item.CurrentWriteBlock++;
			assert(item.CurrentWriteBlock < FileThreadInfo::kNumberOfBlockRecords);

			uint64_t clustersInNewBlock = GetClustersPerBlockByIndex(item.CurrentWriteBlock);
			uint64_t startingOffsetOfNewBlock = startingOffsetInFileForBuffer + totalClusters * m_file.header.sizeOfCluster;
			item.header.blocks[item.CurrentWriteBlock].offsetInUnderlyingFile = FindAddressToAppendNewBlock();
			item.header.blocks[item.CurrentWriteBlock].offsetInThread = startingOffsetOfNewBlock;

			item.CurrentWriteClusterIndex = 0;
			item.CurrentWriteClusters.clear();
			item.CurrentWriteClusters.resize((uint32_t)clustersInNewBlock);
			item.CurrentWriteClusters[0].setOffsetInFile(startingOffsetOfNewBlock);
		}

		item.writeBuffer.clear();
	}
}

void MetafileImpl::InitWriteBuffer(uint32_t index)
{
	assert(index < m_file.threads.size());
	RuntimeThreadInfo &item = m_file.threads[index];

	LoadClustersByAddress(index, item.header.size, item.CurrentWriteBlock, item.CurrentWriteClusterIndex, item.CurrentWriteClusters);

	uint32_t IncompleteDataSize = item.header.size % m_file.header.sizeOfCluster;
	
	if (IncompleteDataSize)
	{
		item.writeBuffer.resize(IncompleteDataSize);
		uint64_t dataOffsetInUnderlyingFile =
			item.CurrentWriteClusters[item.CurrentWriteClusterIndex].getOffsetInFile()
			- item.header.blocks[item.CurrentWriteBlock].offsetInThread
			+ item.header.blocks[item.CurrentWriteBlock].offsetInUnderlyingFile;

		m_fileAccess->SetPointerTo(dataOffsetInUnderlyingFile);
		m_fileAccess->Read(&item.writeBuffer[0], item.writeBuffer.size());
	}
}

std::vector<char> MetafileImpl::Compress(const char *data, uint32_t size)
{
	return std::vector<char>(data, data + size);
}

std::vector<char> MetafileImpl::Decompress(const char *data, uint32_t size)
{
	return std::vector<char>();
}

void MetafileImpl::RemoveExtraBlocks(uint32_t index)
{
	assert(index < m_file.threads.size());
	RuntimeThreadInfo &item = m_file.threads[index];

	for (uint32_t i = sizeof(item.header.blocks) - 1; i != 0xFFFFFFFF; --i)
	{
		if (item.header.blocks[i].offsetInUnderlyingFile >= item.header.size)
		{
			item.header.blocks[i].offsetInUnderlyingFile = 0;
			item.header.blocks[i].offsetInThread = 0;
		}
	}

	uint64_t futherstPos = FindAddressToAppendNewBlock();
	
	//TODO: we can also remove part of unused space in last block
	m_fileAccess->SetFileSize(futherstPos);
}

uint64_t MetafileImpl::GetClustersPerBlockByIndex(uint32_t index)
{
	if (index == 0) return 1;
	if (index < 5) return index;

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

	return val;
}

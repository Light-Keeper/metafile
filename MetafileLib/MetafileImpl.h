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

#include "FileAccessInterface.h"
#include "FileThread.h"
#include "Layout.h"

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

	void UseCompression(bool compression);
	// threads

	std::string FileThreadGetName(uint32_t index);
	uint64_t	FileThreadGetSize(uint32_t index);
	void		FileThreadSetSize(uint32_t index, uint64_t newFileSize);
	bool		FileThreadAppend(uint32_t index, void *data, uint32_t size);
	uint32_t	FileThreadRead(uint32_t index, void *data, uint32_t size);
	void		FileThreadSetPointerTo(uint32_t index, uint64_t pos);

private:

	struct RuntimeThreadInfo
	{
		FileThreadInfo header;
		FileThread interfaceObject;
		
		uint64_t ReadPositionInThread;
		uint32_t CurrentReadBlock;
		uint32_t CurrentReadClusterIndex;

		uint32_t CurrentWriteBlock;
		uint32_t CurrentWriteClusterIndex;

		std::vector<ClusterInfo> CurrentReadClusters;
		std::vector<ClusterInfo> CurrentWriteClusters;

		uint64_t prefetchedOffset;
		std::vector<char> prefetcedAndDecoded;

		std::vector<char> writeBuffer;
	};

	struct RuntimeFileInfo
	{
		MetafileHeader header;
		std::vector<RuntimeThreadInfo> threads;
	};
	
	uint64_t FindAddressToAppendNewBlock();
	void FlushWriteBuffer(uint32_t index);
	void Prefetch(uint32_t index);
	void InitWriteBuffer(uint32_t index);
	void RemoveExtraBlocks(uint32_t index);
	uint64_t GetClustersPerBlockByIndex(uint32_t index);
	std::vector<char> Compress(const char *data, uint32_t size);
	std::vector<char> Decompress(const char *data, uint32_t size);

	void LoadClustersByAddress(uint32_t index, uint64_t address, uint32_t &block, uint32_t &cluster, std::vector<ClusterInfo> &clusters);

	std::shared_ptr<FileAccessInterface> m_fileAccess;
	RuntimeFileInfo m_file;
	bool m_useCompression;
	std::string m_errorMessage;
};

/* Copyright (C) 2015, Vadym Ianushkevych ( vadik.ya@gmail.com )
 * All Rights Reserved.
 * You may use, distribute and modify this code under the terms
 * of the Simplified BSD License. You should have received a copy
 * of the Simplified BSD License with this file. If not, see
 * http://opensource.org/licenses/BSD-2-Clause
*/

/*

	file looks like this:

	+0		-----------------------
			MetafileHeader
	+512b	-----------------------
			FileThreadInfo
	+2k		-----------------------
			FileThreadInfo
	+2k		-----------------------
	......
			block
			block
			block
	
	each block belongs to one file.
	start position of all clusters are listed in FileThreadInfo
	size of cluster depends on it's number in file. (see MetafileImpl::GetClustersPerBlockByIndex)

	block looks like this:
	
	+0		-----------------------
			ClusterInfo
			ClusterInfo
			ClusterInfo
			......  ( for each cluster  )
			-----------------------
	+ MetafileImpl::GetClustersPerBlockByIndex * MetafileHeader::sizeOfCluster
			-----------------------
			Cluster
			Cluster
			Cluster
			........... 
	
	Cluster is just chunck of data. 
	cluster may contain raw data or be compressed. 
*/

#pragma once
#include <stdint.h>

struct MetafileHeader
{
	static const uint32_t kSignature = 0x12345678;
	static const uint32_t kMaxNumberOfThreads = 10000;
	static const uint32_t kPrefetchClusters = 16;
	static const uint32_t kDefaultClustersPerGroup = 16;
	static const uint32_t kDefaultClusterSize = 4 * 1024;

	uint32_t signature;
	uint32_t numberOfThreads;
	uint32_t sizeOfCluster;
	uint32_t clustersPerGroup;

	char reserved[512 - 4 * 4];
};

struct FileThreadInfo
{
	char name[128];
	uint64_t size;
	uint64_t reserved;

	struct BlockRecord
	{
		uint64_t offsetInUnderlyingFile;
		uint64_t offsetInThread;
	};

	static const int kNumberOfBlockRecords = (2048 - 128 - 8 - 8) / sizeof(BlockRecord);
	BlockRecord blocks[kNumberOfBlockRecords];
};

struct ClusterInfo
{
	uint32_t reserved;
	uint32_t flags;
	uint64_t offsetInFile;

	ClusterInfo()
	{
		reserved = 0;
		flags = 0;
		offsetInFile = ~0;
	}

	bool isCompressed() { return (flags & 0x80000000) != 0; }
	
	uint32_t getCompressedSize()
	{
		return flags & 0x00FFFFFF;
	}

	void setCompressed(uint32_t blockSize)
	{
		flags = 0x80000000 | blockSize;
	}

	bool isPartOfCompressed()
	{
		return (flags & 0xC0000000) != 0;
	}

	void setPartOfCompressed()
	{
		flags = 0xC0000000;
	}

	bool isPlainData()
	{
		return flags == 0;
	}

	void setPlainData()
	{
		flags = 0;
	}

	uint64_t getOffsetInFile(){ return offsetInFile; }
	void setOffsetInFile(uint64_t offset){ offsetInFile = offset; }
};
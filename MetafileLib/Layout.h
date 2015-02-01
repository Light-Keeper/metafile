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
	start position of all blocks are listed in FileThreadInfo
	size of block depends on it's number in file. (see MetafileImpl::GetSizeOfBlock)
*/

#pragma once
#include <stdint.h>

struct MetafileHeader
{
	static const uint32_t kSignature = 0x12345678;
	static const uint32_t kMaxNumberOfThreads = 10000;
	static const uint32_t kCurrentVersion = 1;
	static const uint32_t kDefaultClusterSize = 4 * 1024;

	uint32_t signature;
	uint32_t version;
	uint32_t numberOfThreads;
	uint32_t sizeOfCluster;

	char reserved[64 - 4 * 4];
};

struct FileThreadInfo
{
	char name[64];
	uint64_t size;
	uint64_t reserved;

	struct BlockRecord
	{
		uint64_t offsetInUnderlyingFile;
	};

	static const int kNumberOfBlockRecords = (1024 - 64 - 8 - 8) / sizeof(BlockRecord);
	BlockRecord blocks[kNumberOfBlockRecords];
};


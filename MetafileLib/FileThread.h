/* Copyright (C) 2015, Vadym Ianushkevych ( vadik.ya@gmail.com )
 * All Rights Reserved.
 * You may use, distribute and modify this code under the terms
 * of the Simplified BSD License. You should have received a copy
 * of the Simplified BSD License with this file. If not, see
 * http://opensource.org/licenses/BSD-2-Clause
*/

#pragma once
#include <string>
#include <vector>
#include <stdint.h>

class FileThread
{
public:
	~FileThread();

	std::string GetName();
	uint64_t GetSize();

	void SetSize(uint64_t newFileSize);	
	uint32_t Write(void *data, uint32_t size);
	uint32_t Read(void *data, uint32_t size);

	void SetPointerTo(uint64_t pos);

private: 
	friend class MetafileImpl;
	
	int m_index;
	MetafileImpl *m_impl;
};


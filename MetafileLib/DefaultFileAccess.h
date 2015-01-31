/* Copyright (C) 2015, Vadym Ianushkevych ( vadik.ya@gmail.com )
 * All Rights Reserved.
 * You may use, distribute and modify this code under the terms
 * of the Simplified BSD License. You should have received a copy
 * of the Simplified BSD License with this file. If not, see
 * http://opensource.org/licenses/BSD-2-Clause
*/

#pragma once
#include "FileAccessInterface.h"
#include <cstdio>

class DefaultFileAccess : public FileAccessInterface
{
public:
	DefaultFileAccess();
	~DefaultFileAccess();

	virtual void UseFile(const std::string &name) override;
	virtual bool IsValid() override;
	virtual std::string GetLastError() override;
	virtual void SetPointerTo(uint64_t offset) override;
	virtual void SetFileSize(uint64_t) override;
	virtual uint32_t Read(void *buffer, uint32_t bufferSize) override;
	virtual bool Write(void *buffer, uint32_t bufferSize) override;

private:
	std::string m_error;
	FILE *m_file;
};


class DefaultFileAccessFactory : public FileAccessInterfaceAbstractFactory
{
public:
	std::shared_ptr<FileAccessInterface> CreateFile()
	{
		return std::make_shared<DefaultFileAccess>();
	}
};

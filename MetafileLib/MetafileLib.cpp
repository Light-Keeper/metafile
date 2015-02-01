/* Copyright (C) 2015, Vadym Ianushkevych ( vadik.ya@gmail.com )
 * All Rights Reserved.
 * You may use, distribute and modify this code under the terms
 * of the Simplified BSD License. You should have received a copy
 * of the Simplified BSD License with this file. If not, see
 * http://opensource.org/licenses/BSD-2-Clause
*/

#include "MetafileLib.h"
#include "Metafile.h"
#include <assert.h>


MetafileLib::MetafileLib(const std::shared_ptr<FileAccessInterfaceAbstractFactory> &factory)
	: m_AccessFactory(factory)
{
	assert(m_AccessFactory);
}

MetafileLib::~MetafileLib()
{

}

std::shared_ptr<Metafile> MetafileLib::OpenInternal(const std::string &path)
{
	auto fileAccess = m_AccessFactory->CreateFile();
	fileAccess->UseFile(path);

	auto file = std::make_shared<Metafile>();
	file->SetFileAccessInterface(fileAccess);
	return file;
}

std::shared_ptr<Metafile> MetafileLib::CreateNewFile(const std::string &path, const std::vector<std::string> &threadNames)
{
	auto file = OpenInternal(path);
	file->InitEmpty(threadNames);
	return file;
}

std::shared_ptr<Metafile> MetafileLib::OpenFile(const std::string &path)
{
	auto file = OpenInternal(path);
	file->Init();
	return file;
}


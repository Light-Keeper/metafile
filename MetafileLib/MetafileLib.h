/* Copyright (C) 2015, Vadym Ianushkevych ( vadik.ya@gmail.com )
 * All Rights Reserved.
 * You may use, distribute and modify this code under the terms
 * of the Simplified BSD License. You should have received a copy
 * of the Simplified BSD License with this file. If not, see
 * http://opensource.org/licenses/BSD-2-Clause
*/

#pragma once
#include <vector>
#include <memory>

#include "FileAccessInterface.h"
#include "DefaultFileAccess.h"

class Metafile;

class MetafileLib
{
public:
	MetafileLib(const std::shared_ptr<FileAccessInterfaceAbstractFactory> &factory
		= std::shared_ptr<DefaultFileAccessFactory>(new DefaultFileAccessFactory()));
	~MetafileLib();

	// never returns null.
	// call like CreateNewFile("c:\\test.dat", {"data1", "data2"});
	std::shared_ptr<Metafile> CreateNewFile(const std::string &path, 
											const std::vector<std::string> &threadNames);
	
	// never returns null.
	// call like CreateNewFile("c:\\test.dat");
	std::shared_ptr<Metafile> OpenFile(const std::string &path);

private:
	std::shared_ptr<FileAccessInterfaceAbstractFactory> m_AccessFactory;
	std::shared_ptr<Metafile> OpenInternal(const std::string &path);
};


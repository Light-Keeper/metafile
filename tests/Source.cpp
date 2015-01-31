#include "MetafileLib/MetafileLib.h"
#include "MetafileLib/Metafile.h"
#include "MetafileLib/FileThread.h"

#define EXPECT_TRUE(x) if (x) {printf("ok\t\"" #x "\"\n");} else {printf("--\t\"" #x "\"\n");}
#define ASSERT_TRUE(x) if (x) {printf("ok\t\"" #x "\"\n");} else {printf("--\t\"" #x "\"\n"); exit(0);}

MetafileLib libInstance;

void CreateFileTest()
{
	auto file = libInstance.CreateNewFile("c:\\testfile.dat", { "data1", "data2", "data3" });

	ASSERT_TRUE(file->IsValid());
	EXPECT_TRUE(nullptr == file->GetFileThrad("zzz"));
	FileThread *data1 = file->GetFileThrad("data1");
	ASSERT_TRUE(nullptr != data1);

	EXPECT_TRUE(data1->GetSize() == 0);

	std::vector<char> testData(100);
	for (unsigned i = 0; i < testData.size(); i++)
	{
		testData[i] = i;
	}

	data1->Append(&testData[0], testData.size());
	std::vector<char> res(100);

//	data1->SetPointerTo(0);
	EXPECT_TRUE(data1->Read(&res[0], res.size()) == res.size());
	EXPECT_TRUE(res == testData);
}

void ReopenFile()
{
	auto file = libInstance.OpenFile("c:\\testfile.dat");

	ASSERT_TRUE(file->IsValid());
	EXPECT_TRUE(nullptr == file->GetFileThrad("zzz"));
	FileThread *data1 = file->GetFileThrad("data1");
	ASSERT_TRUE(nullptr != data1);

	std::vector<char> testData(100);
	for (unsigned i = 0; i < testData.size(); i++)
	{
		testData[i] = i;
	}

	std::vector<char> res(100);
	data1->SetPointerTo(0);
	EXPECT_TRUE(data1->Read(&res[0], res.size()) == res.size());
	EXPECT_TRUE(res == testData);
}

void ParallelWrite1(int size1, int size2, int step1)
{
	int step2 = step1 * size2 / size1;

	std::vector<char> data1(size1);
	std::vector<char> data2(size2);
	for (uint32_t i = 0; i < data1.size(); i++)
	{
		data1[i] = (char)i; 
	}
	for (uint32_t i = 0; i < data2.size(); i++)
	{
		data2[i] = -(char)i;
	}

	{
		auto file = libInstance.CreateNewFile("c:\\testfile1.dat", { "data1", "data2", "data3" });
		ASSERT_TRUE(file->IsValid());

		FileThread *f1 = file->GetFileThrad("data1");
		FileThread *f2 = file->GetFileThrad("data2");

		for (uint32_t i = 0; step1 * i < data1.size(); i++)
		{
			f1->Append(&data1[step1 * i], step1);
	//		f2->Append(&data2[step2 * i], step2);
		}
	
		std::vector<char> res1(data1.size());
		f1->SetPointerTo(0);
		f1->Read(&res1[0], res1.size());
		EXPECT_TRUE(res1 == data1);
	}



}
int main()
{
	CreateFileTest();
	printf("---------\n");
	ReopenFile();
	printf("---------\n");
	printf("--------- ParallelWrite1(1024 * 1024, 1024 * 1024, 1) -------\n");
	ParallelWrite1(8 * 1024, 8 * 1024, 4096);

	return 0;
}
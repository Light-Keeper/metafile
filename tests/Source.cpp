#include "metafile/metafilelib.h"

#define EXPECT_TRUE(x) if (x) {printf("ok\t\"" #x "\"\n");} else {printf("fail\t\"" #x "\"\n");}
#define ASSERT_TRUE(x) if (x) {printf("ok\t\"" #x "\"\n");} else {printf("fail\t\"" #x "\"\n"); exit(0);}

using namespace metafile;

MetafileLib libInstance;

void CreateFileTest()
{
	auto file = libInstance.CreateNewFile("c:\\testfile.dat", { "data1", "data2", "data3" });
	
	ASSERT_TRUE(file->IsValid());
	EXPECT_TRUE(nullptr == file->GetFileThread("zzz"));
	FileThread *data1 = file->GetFileThread("data1");
	ASSERT_TRUE(nullptr != data1);

	EXPECT_TRUE(data1->GetSize() == 0);

	std::vector<char> testData(100);
	for (unsigned i = 0; i < testData.size(); i++)
	{
		testData[i] = i;
	}

	data1->Write(&testData[0], testData.size());
	std::vector<char> res(100);

	data1->SetPointerTo(0);
	EXPECT_TRUE(data1->Read(&res[0], res.size()) == res.size());
	EXPECT_TRUE(res == testData);
}

void ReopenFile()
{
	auto file = libInstance.OpenFile("c:\\testfile.dat");

	ASSERT_TRUE(file->IsValid());
	EXPECT_TRUE(nullptr == file->GetFileThread("zzz"));
	FileThread *data1 = file->GetFileThread("data1");
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
	int step2 = (int)(1ll * step1 * size2 / size1);

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

		FileThread *f1 = file->GetFileThread("data1");
		FileThread *f2 = file->GetFileThread("data2");

		for (uint32_t i = 0; step1 * i < data1.size(); i++)
		{
			f1->Write(&data1[step1 * i], step1);
			f2->Write(&data2[step2 * i], step2);
		}
	
		std::vector<char> res1(data1.size());
		f1->SetPointerTo(0);
		f1->Read(&res1[0], res1.size());
		EXPECT_TRUE(res1 == data1);
		EXPECT_TRUE(f1->GetSize() == size1);
		EXPECT_TRUE(f2->GetSize() == size2);

		std::vector<char> res2(data2.size());
		f2->SetPointerTo(0);
		f2->Read(&res2[0], res2.size());
		EXPECT_TRUE(res2 == data2);
	}

	{
		auto file = libInstance.OpenFile("c:\\testfile1.dat");
		ASSERT_TRUE(file->IsValid());

		FileThread *f1 = file->GetFileThread("data1");
		FileThread *f2 = file->GetFileThread("data2");

		std::vector<char> res1(data1.size());
		f1->SetPointerTo(0);
		f1->Read(&res1[0], res1.size());
		EXPECT_TRUE(f1->GetSize() == size1);
		EXPECT_TRUE(f2->GetSize() == size2);

		std::vector<char> res2(data2.size());
		f2->SetPointerTo(0);
		f2->Read(&res2[0], res2.size());
		EXPECT_TRUE(res2 == data2);
	}

	{
		auto file = libInstance.CreateNewFile("c:\\testfile4.dat", { "data1", "data2", "data3" });
		ASSERT_TRUE(file->IsValid());

		FileThread *f1 = file->GetFileThread("data1");
		FileThread *f2 = file->GetFileThread("data2");

		for (uint32_t i = data1.size() / step1 - 1; i != ~0; i--)
		{
			f1->SetPointerTo(step1 * i);
			f2->SetPointerTo(step2 * i);
			f1->Write(&data1[step1 * i], step1);
			f2->Write(&data2[step2 * i], step2);
		}

		std::vector<char> res1(data1.size());
		f1->SetPointerTo(0);
		f1->Read(&res1[0], res1.size());
		EXPECT_TRUE(res1 == data1);
		EXPECT_TRUE(f1->GetSize() == size1);
		EXPECT_TRUE(f2->GetSize() == size2);

		std::vector<char> res2(data2.size());
		f2->SetPointerTo(0);
		f2->Read(&res2[0], res2.size());
		EXPECT_TRUE(res2 == data2);
	}

}

void TestByteToByteFollow()
{
	auto file = libInstance.CreateNewFile("c:\\testfile2.dat", { "data1", "data2", "data3" });

	FileThread *f3 = file->GetFileThread("data3");
	FileThread *f1 = file->GetFileThread("data1");

	std::vector<char> res;

	for (int i = 0; i < 10000; i++)
	{
		char x = rand();
		f1->SetPointerTo(i);
		f3->SetPointerTo(i);
		f3->Write(&x, 1);
		f1->Write(&x, 1);
		res.push_back(x);

		char y;
		f1->SetPointerTo(i);
		f3->SetPointerTo(i);
		f3->Read(&y, 1);
		if (y == x) continue;
		printf("fail\tTestByteToByteFollow i = %d\n", i);
		return;
	}

	std::vector<char> actuall(res.size());
	f3->SetPointerTo(0);
	f3->Read(&actuall[0], actuall.size());
	EXPECT_TRUE(res == actuall);

	memset(&actuall[0], 0, actuall.size());
	f1->SetPointerTo(0);
	f1->Read(&actuall[0], actuall.size());
	EXPECT_TRUE(res == actuall);

}

void RandomReads()
{
	auto file = libInstance.CreateNewFile("c:\\testfile2.dat", { "data1", "data2", "data3" });

	FileThread *f3 = file->GetFileThread("data3");
	FileThread *f1 = file->GetFileThread("data1");

	std::vector<char> res3(4000000);
	std::vector<char> res1(4000000);

	for (unsigned i = 0; i < res3.size(); i++)
	{
		res3[i] = (char)i;
		res1[i] = -(char)i;
	}

	f3->Write(&res3[0], res3.size());
	f1->Write(&res1[0], res1.size());


	for (int i = 0; i < 10000; i++)
	{
		static const int datasize = 20000;
		int index = rand();
		index %= (res3.size() - datasize);

		char data[datasize];
		f3->SetPointerTo(index);
		f3->Read(data, datasize);
		int cmp3 = memcmp(data, &res3[index], datasize);

		f1->SetPointerTo(index);
		f1->Read(data, datasize);
		int cmp1 = memcmp(data, &res1[index], datasize);

		
		if (cmp1 == 0 && cmp3 == 0) continue;
		
		EXPECT_TRUE(cmp1 == 0 && cmp3 == 0);
		printf("i = %d, index = %d\n", i, index);
		return;
	}

	printf("ok\tRandomReads\n");
}

void TestDisbalance()
{
	auto file = libInstance.CreateNewFile("c:\\testfile3.dat", { "data1", "data2", "data3" });

	ASSERT_TRUE(file->IsValid());
	FileThread *data1 = file->GetFileThread("data1");
	FileThread *data2 = file->GetFileThread("data2");
	ASSERT_TRUE(nullptr != data1);
	ASSERT_TRUE(nullptr != data2);

	std::vector<char> testData1(10 * 1024 + 1);
	std::vector<char> testData2(testData1.size() * 2);
	for (unsigned i = 0; i < testData1.size(); i++)
	{
		testData1[i] = rand();
		testData2[i] = testData2[i + testData1.size()] = rand();
	}

	data1->Write(&testData1[0], testData1.size());
	data2->Write(&testData2[0], testData2.size());
	std::vector<char> res1(testData1.size());
	std::vector<char> res2(res1.size() * 2);

	EXPECT_TRUE(data1->GetSize() == testData1.size());
	EXPECT_TRUE(data2->GetSize() == testData2.size());

	data1->SetPointerTo(0);
	data2->SetPointerTo(0);
	EXPECT_TRUE(data1->Read(&res1[0], res1.size()) == res1.size());
	EXPECT_TRUE(res1 == testData1);

	EXPECT_TRUE(data2->Read(&res2[0], res2.size()) == res2.size());
	EXPECT_TRUE(res2 == testData2);
}

void Test1ByteInBlock()
{
	auto file = libInstance.CreateNewFile("c:\\testfile3.dat", { "data1", "data2", "data3" });

	ASSERT_TRUE(file->IsValid());
	FileThread *data1 = file->GetFileThread("data1");
	ASSERT_TRUE(nullptr != data1);

	std::vector<char> testData(4 * 1024 + 1);
	for (unsigned i = 0; i < testData.size(); i++)
	{
		testData[i] = rand();
	}

	data1->Write(&testData[0], testData.size());
	std::vector<char> res(4 * 1024 + 1);

	data1->SetPointerTo(0);
	EXPECT_TRUE(data1->Read(&res[0], res.size()) == res.size());
	EXPECT_TRUE(res == testData);

	file.reset();

	file = libInstance.OpenFile("c:\\testfile3.dat");

	ASSERT_TRUE(file->IsValid());
	data1 = file->GetFileThread("data1");
	ASSERT_TRUE(nullptr != data1);
	
	memset(&res[0], 0, res.size());
	EXPECT_TRUE(data1->Read(&res[0], res.size()) == res.size());
	EXPECT_TRUE(res == testData);

	char buff[2];
	data1->SetPointerTo(4 * 1024 - 1);
	data1->Read(buff, 2);
	EXPECT_TRUE(buff[0] == res[4 * 1024 - 1] && buff[1] == res[4 * 1024]);
}

void TestDelete()
{
	auto file = libInstance.CreateNewFile("c:\\testfile5.dat", { "data1", "data2", "data3" });

	ASSERT_TRUE(file->IsValid());
	FileThread *data1 = file->GetFileThread("data1");
	ASSERT_TRUE(nullptr != data1);

	std::vector<char> testData(10 * 1024 + 1);
	for (unsigned i = 0; i < testData.size(); i++)
	{
		testData[i] = rand();
	}

	data1->Write(&testData[0], testData.size());
	std::vector<char> res1(testData.size());
	
	EXPECT_TRUE(data1->GetSize() == testData.size());
	
	data1->SetPointerTo(0);
	EXPECT_TRUE(data1->Read(&res1[0], res1.size()) == res1.size());
	EXPECT_TRUE(res1 == testData);

	testData.resize(testData.size() - 1);
	data1->SetSize(testData.size());
	EXPECT_TRUE(data1->GetSize() == testData.size());
	res1.resize(testData.size());

	data1->SetPointerTo(0);
	EXPECT_TRUE(data1->Read(&res1[0], res1.size()) == res1.size());
	EXPECT_TRUE(res1 == testData);

	testData.resize(4 * 1024 + 1);
	data1->SetSize(testData.size());
	EXPECT_TRUE(data1->GetSize() == testData.size());
	res1.resize(testData.size());

	data1->SetPointerTo(0);
	EXPECT_TRUE(data1->Read(&res1[0], res1.size()) == res1.size());
	EXPECT_TRUE(res1 == testData);
}

void WriteBigFile()
{
	auto file = libInstance.CreateNewFile("c:\\testfile4.dat", { "data1", "data2", "data3" });

	ASSERT_TRUE(file->IsValid());
	FileThread *data1 = file->GetFileThread("data1");
	ASSERT_TRUE(nullptr != data1);

	std::vector<char> data(1024 * 1024 * 40);

	for (unsigned i = 0; i < data.size(); i++)
	{
		data[i] = (char)i;
	}

	for (int i = 0; i < 100; i++)
	{
		data1->Write(&data[0], data.size());
	}
}

int main()
{
	printf("---------\n");
	CreateFileTest();
	printf("---------\n");
	ReopenFile();
	printf("---------\n");
	printf("--------- ParallelWrite1(8 * 1024, 8 * 1024, 4096) -------\n");
	ParallelWrite1(8 * 1024, 8 * 1024, 4096);
	printf("--------- ParallelWrite1(800 * 1024, 800 * 1024, 4096 / 16) -------\n");
	ParallelWrite1(800 * 1024, 800 * 1024, 4096 / 16);
	printf("--------- ParallelWrite1(8 * 1024, 800 * 1024, 4096 / 4) -------\n");
	ParallelWrite1(8 * 1024, 800 * 1024, 4096 / 4);
	printf("--------- ParallelWrite1(8 * 1024, 16 * 1024, 1024) -------\n");
	ParallelWrite1(8 * 1024, 16 * 1024, 1024);
	printf("--------- ParallelWrite1(15 * 40 * 1024, 15 * 80 * 1024, 15) -------\n");
	ParallelWrite1(15 * 40 * 1024, 15 * 80 * 1024, 15);
	printf("--------- TestByteToByteFollow -------\n");
	TestByteToByteFollow();
	printf("--------- RandomReads -------\n");
	RandomReads();
	printf("--------- Test1ByteInBlock -------\n");
	Test1ByteInBlock();
	printf("--------- TestDisbalance -------\n");
	TestDisbalance();
	printf("--------- TestDelete -------\n");
	TestDelete();

//	WriteBigFile();

	return 0;
}
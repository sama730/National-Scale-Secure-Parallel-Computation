//========================================================================
// This conversion was produced by the Free Edition of
// C# to C++ Converter courtesy of Tangible Software Solutions.
// Order the Premium Edition at https://www.tangiblesoftwaresolutions.com
//========================================================================

#include "TestEncoder.h"

using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace Utility;

namespace MaskedEvaluationTest
{

	void TestEncoder::TestBitArrayEncoder()
	{
		for (int i = 0;i < 100;i++)
		{
			for (int j = 12;j < 30;j++)
			{
				BitArray *b1 = BitArrayUtility::SampleRandomBitArray(j);
				BitArray *copy = BitArrayEncoder::DecodeBitArray(b1->UnsafeEncode(),j);
				Assert::IsTrue(b1->AreEqual(copy));
			}
		}
	}

	void TestEncoder::TestBitArrayListEncoder()
	{
		for (int i = 0; i < 100; i++)
		{
			for (int j = 12; j < 30; j++)
			{
				std::vector<BitArray*> list = Enumerable::Range(0, j)->Select([&] (void *x)
				{
					BitArrayUtility::SampleRandomBitArray(j);
				}).ToList();

				std::vector<unsigned char> encoding = BitArrayEncoder::EncodeBitArrayList(list);
				std::vector<BitArray*> decoding = BitArrayEncoder::DecodeArrayBitArray(encoding);

				Assert::IsTrue(list.Zip(decoding, BitArrayUtility::AreEqual).All([&] (void *x)
				{
					return x;
				}));
			}
		}
	}
}

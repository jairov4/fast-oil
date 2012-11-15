#include "StdAfx.h"
#include "Ndfa.h"
#include "NdfaDotExporter.h"
#include "OilTrainer.h"
#include "SamplesReader.h"
#include "Testing.h"

using namespace std;

namespace Testing {

	void Test1()
	{
		Ndfa ndfa(5);		
		ndfa.SetTransition(0, 1, 3);
		ndfa.SetInitial(0);
		ndfa.SetFinal(1);

		unsigned array1[] = {1,2,3,4};
		unsigned array2[] = {1};
		unsigned array3[] = {3};
				
		assert(!ndfa.IsMatch(OilTrainer::TSample(array1, array1+4)));
		assert(!ndfa.IsMatch(OilTrainer::TSample(array2, array2+1)));
		assert(ndfa.IsMatch(OilTrainer::TSample(array3, array3+1)));
	}

	void Test2()
	{
		Ndfa ndfa(2);		
		ndfa.SetTransition(0, 1, 0);
		ndfa.SetTransition(0, 0, 1);
		ndfa.SetInitial(0);
		ndfa.SetFinal(1);
		NdfaDotExporter::Export(ndfa, "test2.dot");
	}

	void Test3()
	{
		Ndfa ndfa(2);		
		ndfa.SetTransition(0, 1, 0);
		ndfa.SetTransition(0, 0, 1);
		ndfa.SetInitial(0);
		ndfa.SetFinal(1);
		ndfa.Merge(0, 1);
		NdfaDotExporter::Export(ndfa, "test3.dot");
	}

	void Test4()
	{
		BitVector vec;
		vec.ClearAll();
		vec.Set(3);
		vec.Set(5);
		vec.Set(6);
		vec.Set(63);
		vec.Set(64);
		vec.Set(66);
		int i = 0;
		for(auto it=vec.GetBitSetIterator(); !it.IsEnd(); it.Next())
		{
			auto n = it.GetBit();		
			// implicaciones p -> q  === ~p | q
			assert(i!=0 || n==3); // (i=0) -> n=3
			assert(i!=1 || n==5);
			assert(i!=2 || n==6);
			assert(i!=3 || n==63);
			assert(i!=4 || n==64);
			assert(i!=5 || n==66);
			i++;			
		}
		assert(i == 6);

		vec = BitVector();
		vec.ClearAll();
		vec.Set(63);
		for(auto it=vec.GetBitSetIterator(); !it.IsEnd(); it.Next())
		{
			assert(it.GetBit()==63);			
		}
	}
		
	OilTrainer::TSamples makeSamples(OilTrainer::TSymbol* ini, size_t sampLen, size_t total)
	{
		OilTrainer::TSamples samples;
		for (auto it = ini; it != ini+total; it+=sampLen)
		{
			OilTrainer::TSample sample;
			for (auto it2 = it; it2 != it+sampLen; it2++)
			{
				sample.push_back(*it2);
			}
			samples.push_back(sample);
		}
		return samples;
	}

	void Test5()
	{
		OilTrainer trainer;
		const int alpha = 4;
		const int len = 4;
		OilTrainer::TSymbol pos[] = {
			0, 1, 2, 3,
			0, 0, 0, 1,
			0, 0, 0, 2,
			0, 0, 1, 2,
			0, 0, 1, 3,
		};
		OilTrainer::TSymbol neg[] = {
			1, 2, 3, 3,
			1, 3, 2, 2,
			2, 0, 1, 1,
			2, 0, 0, 0,
			3, 0, 2, 0,
		};
		
		auto vpos = makeSamples(pos, 4, 20);
		auto vneg = makeSamples(neg, 4, 20);
		trainer.ShowProgress = true;
		trainer.ShowMerges = true;
		auto ndfa = trainer.Train(vpos, vneg, alpha);
		delete ndfa;
	}

	void Test6()
	{
		SamplesReader reader;
		SamplesReader::TSamples pos, neg;
		unsigned alpha;
		reader.ReadSamples("training.sample", pos, neg, &alpha);
	}

	void Test7()
	{
		SamplesReader reader;
		SamplesReader::TSamples pos, neg;
		unsigned alpha;
		reader.ReadSamples("training2.sample", pos, neg, &alpha);

		OilTrainer trainer;
		trainer.ShowProgress = true;
		trainer.ShowMerges = true;
		trainer.SkipSearchBestMerge = true;
		trainer.DoNotUseRandomSort = true;
		trainer.ShowPossibleMerges = true;
		auto ndfa = trainer.Train(pos, neg, alpha);
		NdfaDotExporter::Export(*ndfa, "Test7.dot");
		NdfaDotExporter::ExportDestinoPlainText(*ndfa, "Test7.auto");
		delete ndfa;
	}

	void AllTesting()
	{	
		list<function<void()>> s;

		s.push_back(Test1);
		s.push_back(Test2);
		s.push_back(Test3);
		s.push_back(Test4);
		s.push_back(Test5);
		s.push_back(Test6);
		s.push_back(Test7);
		
		int i=0;
		for_each(s.begin(), s.end(), [&i](function<void()> t){
			auto ini = time(NULL);
			t();
			auto fin = time(NULL);
			i++;
			cout << "Test passed " << i << " (elapsed: " << (fin-ini) << ")" << endl;
		});
		getc(stdin);
	}
}
#include "StdAfx.h"
#include "Nfa.h"
#include "NfaDotExporter.h"
#include "OilTrainer.h"
#include "SamplesReader.h"
#include "Testing.h"

using namespace std;

namespace Testing {
	
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
		auto nfa = trainer.Train(pos, neg, alpha);
		NfaDotExporter::Export(*nfa, "Test7.dot");
		NfaDotExporter::ExportDestinoPlainText(*nfa, "Test7.auto");
		delete nfa;
	}

	void AllTesting()
	{	
		list<function<void()>> s;
		
		//s.push_back(Test4);
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
		cout << "Testing finished (press ENTER to quit)..." << endl;
		getc(stdin);
	}
}
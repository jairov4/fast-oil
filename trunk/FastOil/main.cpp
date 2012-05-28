#include "stdafx.h"
#include "Ndfa.h"
#include "SamplesReader.h"
#include "OilTrainer.h"
#include "NdfaDotExporter.h"
#include "Testing.h"

void exec(std::string fn)
{
	SamplesReader reader;
	SamplesReader::TSamples pos, neg;
	unsigned alpha;
	reader.ReadSamples(fn, pos, neg, &alpha);

	OilTrainer trainer;
	trainer.ShowProgress = true;
	trainer.ShowMerges = true;
	trainer.SkipSearchBestMerge = true;
	trainer.DoNotUseRandomSort = true;
	trainer.ShowPossibleMerges = true;
	auto ndfa = trainer.Train(pos, neg, alpha);
	NdfaDotExporter::Export(*ndfa, "automata.dot");
	NdfaDotExporter::ExportDestinoPlainText(*ndfa, "automata.auto");
	delete ndfa;
}

int main(int argc, char* argv[])
{
	auto t = time(NULL);
	t = 1;
	srand((unsigned)t);
	//Testing::AllTesting();
	if(argc < 2) return 255;
	exec(argv[1]);
	return 0;
}

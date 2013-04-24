#pragma once

#include "Nfa.h"
#include <vector>

class OilTrainer
{
public:
	typedef Nfa::TSymbol TSymbol;
	typedef Nfa::TSample TSample;
	typedef std::vector<TSample> TSamples;
	
private:	
	// donde se agregaron los estados en el arreglo de identificadores
	unsigned statesAddedBeginInRandom;

	Nfa* nfa;
	Nfa* testNfa;
	Nfa* bestNfa;

	TSamples* posSamples;
	TSamples* negSamples;
	std::vector<int> randomIds;
	
	void CoreceMatch(TSamples::const_iterator currentPosSampleIterator);
	void DoAllMergesPossible(TSamples::const_iterator currentPosSampleIterator);

public:
	/// Indica si durante el entrenamiento se muestran mensajes de combinacion de estados
	bool ShowMerges;
	/// Indica si durante el entrenamiento se muestran mensajes progreso
	bool ShowProgress;
	bool SkipSearchBestMerge;
	bool ShowPossibleMerges;
	bool DoNotUseRandomSort;
		
	Nfa* Train(TSamples& posSamples, TSamples& negSamples, unsigned alpha);	
	OilTrainer();
};


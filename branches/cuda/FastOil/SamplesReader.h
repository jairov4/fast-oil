#pragma once

#include <vector>
#include <string>

class SamplesReader
{
public:
	typedef unsigned TSymbol;
	typedef std::vector<TSymbol> TSample;
	typedef std::vector<TSample> TSamples;

	void ReadSamples(std::string filename, TSamples& pos, TSamples& neg, unsigned* alphabetLength);

	SamplesReader(void);
	~SamplesReader(void);
};


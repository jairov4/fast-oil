#pragma once

#include "BitVector.h"
#include <vector>

/** Representa un automata no determinista
*/
class Ndfa
{
public:
	typedef unsigned TSymbol;
	typedef std::vector<TSymbol> TSample;
	typedef TSample::iterator TSampleIter;
	typedef TSample::const_iterator TSampleConstIter;

private:	
	BitVector ActiveStates;
	BitVector Initial;
	BitVector Final;
	std::vector<BitVector> Succesors;
	std::vector<BitVector> Predecessors;	
	unsigned AlphabetLenght;
	
	unsigned _GetIndex(unsigned st, TSymbol sym) const;
	BitVector& _GetPred(unsigned state, TSymbol sym);
	BitVector& _GetSuc(unsigned state, TSymbol sym);	
	void ActivateState(unsigned st);
	
public:
	Ndfa(unsigned alpha);
	~Ndfa(void);
				
	bool IsMatch(TSampleConstIter begin, TSampleConstIter end) const;
	bool IsMatch(const TSample& sample) const;
	void Merge(unsigned ns1, unsigned ns2);	
	void SetTransition(unsigned src, unsigned dest, TSymbol sym);
	void SetInitial(unsigned st);
	void SetFinal(unsigned st);
	const BitVector& GetPredecessors(unsigned state, TSymbol sym) const;
	const BitVector& GetSuccesors(unsigned state, TSymbol sym) const;	
	const BitVector& GetInitial() const;
	const BitVector& GetFinal() const;
	const BitVector& GetActiveStates() const;
	unsigned GetAlphabetLenght() const;	
};

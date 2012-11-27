#pragma once

#include <vector>

/** Representa un automata no determinista
*/
class Nfa
{
public:
	typedef unsigned TSymbol;
	typedef __int64 TToken;
	typedef std::vector<TSymbol> TSample;
	typedef TSample::iterator TSampleIter;
	typedef TSample::const_iterator TSampleConstIter;
	typedef TToken* TTokenVector;

	const unsigned BitsPerToken = sizeof(TToken) * 8;

private:	
	TTokenVector ActiveStates;
	TTokenVector Initial;
	TTokenVector Final;
	TTokenVector Predecessors;
	TTokenVector Succesors;

	// Pack data
	// ActiveStates, Initial, Final, Pred, Suc
	TTokenVector AllMemory;

	unsigned AlphabetLenght;
	unsigned Tokens;	
	
	unsigned _GetIndex(unsigned st, TSymbol sym) const;
	TTokenVector _GetPred(unsigned state, TSymbol sym);
	TTokenVector _GetSuc(unsigned state, TSymbol sym);

	void ActivateState(unsigned st);

	// Token management
	TTokenVector CreateTokenVector() const;
	void CloneTokenVector(TTokenVector dest, const TTokenVector source) const;
	size_t GetVectorSize() const;
	// bit operators
	void ClearTokenVector(TTokenVector dest) const;
	void OrTokenVector(TTokenVector dest, const TTokenVector v) const;
	bool AnyAndTokenVector(const TTokenVector dest, const TTokenVector v) const;
	
public:
	Nfa(unsigned alpha);
	~Nfa(void);

	void SetTransition(unsigned src, unsigned dest, TSymbol sym);
	void SetInitial(unsigned st);
	void SetFinal(unsigned st);

	bool IsMatch(TSampleConstIter begin, TSampleConstIter end) const;
	bool IsMatch(const TSample& sample) const;
	void Merge(unsigned ns1, unsigned ns2);		
	
	const TTokenVector GetPredecessors(unsigned state, TSymbol sym) const;
	const TTokenVector GetSuccesors(unsigned state, TSymbol sym) const;	
	const TTokenVector GetInitial() const;
	const TTokenVector GetFinal() const;
	const TTokenVector GetActiveStates() const;
	
	unsigned GetAlphabetLenght() const;	
};

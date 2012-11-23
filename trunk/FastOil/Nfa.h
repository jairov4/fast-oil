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
	typedef std::vector<TToken> TTokenVector;
private:
	TTokenVector ActiveStates;
	TTokenVector Initial;
	TTokenVector Final;
	TTokenVector Predecessors;
	TTokenVector Succesors;

	unsigned AlphabetLenght;
	
	unsigned _GetIndex(unsigned st, TSymbol sym) const;
	TToken* _GetPred(unsigned state, TSymbol sym);
	TToken* _GetSuc(unsigned state, TSymbol sym);
	void ActivateState(unsigned st);
	
public:
	Nfa(unsigned alpha);
	~Nfa(void);
				
	bool IsMatch(TSampleConstIter begin, TSampleConstIter end) const;
	bool IsMatch(const TSample& sample) const;
	void Merge(unsigned ns1, unsigned ns2);	
	void SetTransition(unsigned src, unsigned dest, TSymbol sym);
	void SetInitial(unsigned st);
	void SetFinal(unsigned st);
	
	const std::vector<TToken>& GetPredecessors(unsigned state, TSymbol sym) const;
	const std::vector<TToken>& GetSuccesors(unsigned state, TSymbol sym) const;	
	const std::vector<TToken>& GetInitial() const;
	const std::vector<TToken>& GetFinal() const;
	const std::vector<TToken>& GetActiveStates() const;
	
	unsigned GetAlphabetLenght() const;	
};

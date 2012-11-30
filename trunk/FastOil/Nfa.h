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

	static const unsigned BitsPerToken = sizeof(TToken) * 8;

private:

	TTokenVector ActiveStates;
	TTokenVector Initial;
	TTokenVector Final;
	TTokenVector Predecessors;
	TTokenVector Succesors;

	// Packed data
	// ActiveStates, Initial, Final, Pred, Suc
	TTokenVector AllMemory;

	// Cantidad de simbolos en el alfabeto. Se inicializa solo durante el constructor
	unsigned AlphabetLenght;

	// Indica la cantidad de tokens alojados actualmente
	// Se actualiza cuando se redimensiona la cantidad de estados soportados
	unsigned Tokens;

	// Indica la cantidad total de tokens alojados
	unsigned TotalTokens;

	// Indica la cantidad de estados maxima actual, se actualiza cuando se redimensiona
	// la cantidad de estados soportados
	unsigned MaxStates;
	
	// Obtiene el indice de los tokens para sucesores y predecesores
	unsigned _GetIndex(unsigned st, TSymbol sym) const;
	unsigned _GetIndex(unsigned st, TSymbol sym, unsigned Tokens) const;

	void _MoveActiveTokenVectors(TTokenVector dest, const TTokenVector source, unsigned beforeTokens, unsigned beforeVectorSize);

	// Obtiene la referencia modificable a los tokens para predecesores
	TTokenVector _GetPred(unsigned state, TSymbol sym) const;

	// Obtiene la referencia modificable a los tokens para sucesores
	TTokenVector _GetSuc(unsigned state, TSymbol sym) const;

	// Activa un estado
	void ActivateState(unsigned st);

	// Redimensiona la cantidad de estados soportada
	void ResizeFor(unsigned states);

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
	Nfa(const Nfa& c);
	~Nfa(void);

	void Clear();
	void CloneFrom(const Nfa& c);

	void SetTransition(unsigned src, unsigned dest, TSymbol sym);
	void SetInitial(unsigned st);
	void SetFinal(unsigned st);

	bool IsInitial(unsigned st) const;
	bool IsFinal(unsigned st) const;
	bool IsActiveState(unsigned st) const;
	bool ExistTransition(unsigned src, unsigned dest, TSymbol sym) const;

	bool IsMatch(TSampleConstIter begin, TSampleConstIter end) const;
	bool IsMatch(const TSample& sample) const;
	void Merge(unsigned ns1, unsigned ns2);		
	
	const TTokenVector GetPredecessors(unsigned state, TSymbol sym) const;
	const TTokenVector GetSuccesors(unsigned state, TSymbol sym) const;	
	const TTokenVector GetInitial() const;
	const TTokenVector GetFinal() const;
	const TTokenVector GetActiveStates() const;
		
	unsigned GetInactiveState() const;	
	unsigned GetMaxStates() const;	
	unsigned GetAlphabetLenght() const;		
};

bool _SetBit(Nfa::TTokenVector vec, unsigned bit);
bool _ClearBit(Nfa::TTokenVector vec, unsigned bit);
bool _TestBit(const Nfa::TTokenVector vec, unsigned bit);
void _ClearAllBits(Nfa::TTokenVector vec, unsigned tokens);
void _OrAndClearSecondBit(Nfa::TTokenVector vec, unsigned b1, unsigned b2);
Nfa::TTokenVector AllocTokens(unsigned tokens);
Nfa::TTokenVector ReallocTokens(Nfa::TTokenVector v, unsigned tokens);
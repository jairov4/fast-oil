#pragma once

#include <vector>
#include <stdint.h>

template<class TSymbol = uint8_t>
class NfaInterface
{
public:
	virtual void CopyBufferTo(void* memDest) = 0;

	virtual void SetTransition(uint32_t src, uint32_t dest, TSymbol sym) = 0;
	virtual void SetInitial(uint32_t st) = 0;
	virtual void SetFinal(uint32_t st) = 0;

	virtual bool IsInitial(uint32_t st) const = 0;
	virtual bool IsFinal(uint32_t st) const = 0;
	virtual bool IsActiveState(uint32_t st) const = 0;
	virtual bool ExistTransition(uint32_t src, uint32_t dest, TSymbol sym) const = 0;

	virtual unsigned GetInactiveState() const = 0;	
	virtual unsigned GetMaxStates() const = 0;	
	virtual unsigned GetAlphabetLenght() const = 0;

	virtual void Merge(uint32_t ns1, uint32_t ns2) = 0;
		
	virtual bool IsMatch(std::iterator<std::input_iterator_tag, TSymbol> begin, std::iterator<std::input_iterator_tag, TSymbol> end) = 0;

	virtual ~NfaInterface() = 0;
};

/** Representa un automata no determinista
*/
template<class TSymbol, class TToken>
class Nfa : public NfaInterface<TSymbol>
{
public:	
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

	void _MoveActiveTokenVectors(TTokenVector dest, const TTokenVector source, unsigned beforeTokens, size_t beforeVectorSize);

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
	virtual ~Nfa();

	void Clear();
	void CloneFrom(const Nfa& c);
	
	const TTokenVector GetPredecessors(unsigned state, TSymbol sym) const;
	const TTokenVector GetSuccesors(unsigned state, TSymbol sym) const;	
	const TTokenVector GetInitial() const;
	const TTokenVector GetFinal() const;
	const TTokenVector GetActiveStates() const;

	virtual void CopyBufferTo(void* memDest);

	virtual void SetTransition(uint32_t src, uint32_t dest, TSymbol sym);
	virtual void SetInitial(uint32_t st);
	virtual void SetFinal(uint32_t st);

	virtual bool IsInitial(uint32_t st) const;
	virtual bool IsFinal(uint32_t st) const;
	virtual bool IsActiveState(uint32_t st) const;
	virtual bool ExistTransition(uint32_t src, uint32_t dest, TSymbol sym) const;

	virtual unsigned GetInactiveState() const;	
	virtual unsigned GetMaxStates() const;	
	virtual unsigned GetAlphabetLenght() const;

	virtual void Merge(uint32_t ns1, uint32_t ns2);
	
	virtual bool IsMatch(std::iterator<std::input_iterator_tag, TSymbol> begin, std::iterator<std::input_iterator_tag, TSymbol> end);
};

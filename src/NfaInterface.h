#pragma once

#include <vector>
#include <stdint.h>

template<class TSymbol = uint8_t>
class NfaInterface
{
public:
	typedef typename std::vector<TSymbol> TVectorSymbol;
	typedef typename TVectorSymbol::const_iterator TVectorSymbolIterConst;

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

	virtual bool IsMatch(TVectorSymbolIterConst begin, TVectorSymbolIterConst end) = 0;

	virtual ~NfaInterface() { }
};

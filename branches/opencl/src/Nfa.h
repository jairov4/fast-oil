#pragma once

#include <vector>
#include <stdint.h>
#include <boost/static_assert.hpp>

#include "NfaInterface.h"

template<class T, bool UseAVX256>
class BitUtil
{
public:
	static bool BitScanForward(unsigned long* idx, T v);
	static bool BitScanReverse(unsigned long* idx, T v);
	static bool SetBit(T* vec, unsigned bit);
	static bool ClearBit(T* vec, unsigned bit);
	static bool TestBit(const T* vec, unsigned bit);
	static void ClearAllBits(T* vec, unsigned tokens);
	static void OrAndClearSecondBit(T* vec, unsigned b1, unsigned b2);
	static void OrVector(T* o, const T* v1, const T*v2, unsigned s);
	static bool AnyBitOfAndVector(const T* v1, const T*v2, unsigned s);
};

template<bool UseAVX256>
class BitUtil<uint64_t, UseAVX256>
{
public:
	static bool BitScanForward(unsigned long* idx, uint64_t v)
	{
		return _BitScanForward64(idx, v) != 0;
	}

	static bool BitScanReverse(unsigned long* idx, uint64_t v)
	{
		return _BitScanReverse64(idx, v) != 0;
	}

	static bool SetBit(uint64_t* vec, unsigned bit)
	{
		return _bittestandset64((int64_t*)vec, bit) != 0;
	}
	
	static bool ClearBit(uint64_t* vec, unsigned bit)
	{
		return _bittestandreset64((int64_t*)vec, bit) != 0;
	}

	static bool TestBit(const uint64_t* vec, unsigned bit)
	{
		return _bittest64((const int64_t*)vec, bit) != 0;
	}

	static void ClearAllBits(uint64_t* vec, unsigned tokens)
	{
		memset(vec, 0, tokens * sizeof(uint64_t));	
	}

	static void OrAndClearSecondBit(uint64_t* vec, unsigned b1, unsigned b2) 
	{	
		auto c2 = _bittestandreset64((int64_t*)vec, b2);
		if(c2) _bittestandset64((int64_t*)vec, b1);	
	}

	static void OrVector(uint64_t* o, const uint64_t* v1, const uint64_t* v2, unsigned s)
	{
		if(UseAVX256)
		{
			assert(s * 64 % 256 == 0); // tokens debe ser multiplo de 256
			const int incr = 256 / 64; // AVX use 256-bit registers
			for(unsigned i=0; i<s; i+=incr)
			{
				__m256 rs = _mm256_loadu_ps((const float*)&v1[i]);
				__m256 rd = _mm256_loadu_ps((const float*)&v2[i]);		
				rd = _mm256_or_ps(rs, rd);
				_mm256_storeu_ps((float*)&o[i], rd);
			}
		} else {
			while(s--)
			{
				*o++ = *v1++ | *v2++;
			}
		}
	}

	static bool AnyBitOfAndVector(const uint64_t* v1, const uint64_t* v2, unsigned s)
	{
		if(UseAVX256) 
		{
			assert(s * 64 % 256 == 0); // tokens debe ser multiplo de 256
			const int incr = 256 / 64; // AVX use 256-bit registers
			for(unsigned i=0; i<s; i+=incr)
			{
				__m256i rs = _mm256_loadu_si256((__m256i*)&v1[i]);
				__m256i rd = _mm256_loadu_si256((__m256i*)&v2[i]);
				if(!_mm256_testz_si256(rd, rs)) return true;
			}
		} else {
			while(s--)
			{
				if(*v1++ & *v2++) return true;
			}
			return false;
		}
	}
};


template<bool UseAVX256>
class BitUtil<uint32_t, UseAVX256>
{
public:
	// No se permite usar AVX256 con tokens de 32 bits
	BOOST_STATIC_ASSERT(!UseAVX256);

	static bool BitScanForward(unsigned long* idx, uint32_t v)
	{
		return _BitScanForward(idx, v) != 0;
	}

	static bool BitScanReverse(unsigned long* idx, uint32_t v)
	{
		return _BitScanForward(idx, v) != 0;
	}

	static bool SetBit(uint32_t* vec, unsigned bit)
	{
		return _bittestandset((long*)vec, bit) != 0;
	}
	
	static bool ClearBit(uint32_t* vec, unsigned bit)
	{
		return _bittestandreset((long*)vec, bit) != 0;
	}

	static bool TestBit(const uint32_t* vec, unsigned bit)
	{
		return _bittest((const long*)vec, bit) != 0;
	}

	static void ClearAllBits(uint32_t* vec, unsigned tokens)
	{
		memset(vec, 0, tokens * sizeof(uint32_t));	
	}

	static void OrAndClearSecondBit(uint32_t* vec, unsigned b1, unsigned b2) 
	{
		auto c2 = _bittestandreset((long*)vec, b2);
		if(c2) _bittestandset((long*)vec, b1);	
	}

	static void OrVector(uint32_t* o, const uint32_t* v1, const uint32_t* v2, unsigned s)
	{
		while(s--)
		{
			*o++ = *v1++ | *v2++;
		}
	}

	static bool AnyBitOfAndVector(const uint32_t* v1, const uint32_t* v2, unsigned s)
	{		
		while(s--)
		{
			if(*v1++ & *v2++) return true;
		}
		return false;
	}
};

/** Representa un automata no determinista
*/
template<class TSymbol, class TToken, bool UseAVX256>
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

	typedef BitUtil<TToken, UseAVX256> BU;
	
	// Obtiene el indice de los tokens para sucesores y predecesores
	unsigned _GetIndex(unsigned st, TSymbol sym) const
	{
		assert(sym < AlphabetLenght);
		return _GetIndex(st, sym, Tokens);
	}

	unsigned _GetIndex(unsigned st, TSymbol sym, unsigned Tokens) const
	{
		assert(sym < AlphabetLenght);
		return (st*AlphabetLenght+sym)*Tokens;
	}

	void _MoveActiveTokenVectors(TTokenVector dest, const TTokenVector source, unsigned beforeTokens, size_t beforeVectorSize)
	{		
		unsigned bitToken = 0;
		for(unsigned it=0; it<beforeTokens; it++)
		{
			TToken fetch = ActiveStates[beforeTokens-it-1];
			unsigned long idx = 0;
			
			while(BU::BitScanReverse(&idx, fetch))
			{
				BU::ClearBit(&fetch, idx);
				unsigned state = idx + bitToken;
				
				for(TSymbol sym=0; sym<AlphabetLenght; sym++)
				{
					auto symr = AlphabetLenght-sym-1;
					auto n_old = _GetIndex(state, sym, beforeTokens);
					auto n_new = _GetIndex(state, sym, Tokens);
					memcpy(dest+n_new, source+n_old, beforeVectorSize);
				}
			}
			bitToken+=BitsPerToken;
		}
	}

	// Obtiene la referencia modificable a los tokens para predecesores
	TTokenVector _GetPred(unsigned state, TSymbol sym) const
	{
		return &Predecessors[_GetIndex(state, sym)];
	}

	// Obtiene la referencia modificable a los tokens para sucesores
	TTokenVector _GetSuc(unsigned state, TSymbol sym) const
	{
		return &Succesors[_GetIndex(state, sym)];
	}

	// Activa un estado
	void ActivateState(unsigned st)
	{
		if(st >= MaxStates)
		{
			ResizeFor(MaxStates * 2);
		}

		// si no estaba activo se realiza algo de limpieza
		if(!BU::SetBit(ActiveStates, st))	
		{
			auto idxBegin = _GetIndex(st, 0);
			auto idxEnd = _GetIndex(st+1, 0);
			// si ya hay espacio solo los limpia.
			// No se inicializan Initial y Final ya que por diseño esas banderas se limpian
			// cuando el estado se desactiva en Merge()
			memset(Succesors + idxBegin, 0, GetVectorSize()*AlphabetLenght);
			memset(Predecessors + idxBegin, 0, GetVectorSize()*AlphabetLenght);
		}
	}

	// Redimensiona la cantidad de estados soportada
	void ResizeFor(unsigned states)
	{
		unsigned beforeTokens = Tokens;
		unsigned beforeMaxStates = MaxStates;
		size_t beforeVectorSize = GetVectorSize();

		if(UseAVX256) 
		{
			assert(BitsPerToken <= 256); // Condicion necesaria para la compatibilidad con AVX
			// asegura que la cantidad de estados sea multiplo de 256 para aplicar instrucciones AVX
			states = ((states - 1) / 256 + 1) * 256;
		}

		// Layout: Active, Initial, Final, Predecessors, Successors
		// Token allocation count:
		// Active, Initial, Final => Tokens
		// Predecessors, Successors => Tokens*MaxStates*AlphabetLenght
		// Tokens*3 + Tokens*MaxStates*AlphabetLenght*2			
		Tokens = (states - 1) / BitsPerToken + 1;	
		MaxStates = Tokens * BitsPerToken;
		TotalTokens = Tokens*3 + Tokens*MaxStates*AlphabetLenght*2;
		
		AllMemory = ReallocTokens(AllMemory, TotalTokens);

		auto beforeActiveStates = ActiveStates;
		ActiveStates = &AllMemory[Tokens*0];

		auto beforeInitial = Initial;
		Initial = &AllMemory[Tokens*1];

		auto beforeFinal = Final;
		Final = &AllMemory[Tokens*2];

		auto beforePredecessors = Predecessors;
		Predecessors = &AllMemory[Tokens*3 + Tokens*MaxStates*AlphabetLenght*0];
		
		auto beforeSuccesors = Succesors;
		Succesors = &AllMemory[Tokens*3 + Tokens*MaxStates*AlphabetLenght*1];

		_MoveActiveTokenVectors(Succesors, beforeSuccesors, beforeTokens, beforeVectorSize);
		_MoveActiveTokenVectors(Predecessors, beforePredecessors, beforeTokens, beforeVectorSize);
		memcpy(Final, beforeFinal, beforeVectorSize);
		memcpy(Initial, beforeInitial, beforeVectorSize);
	}

	// Token management
	TTokenVector CreateTokenVector() const
	{
		return AllocTokens(Tokens);
	}

	void CloneTokenVector(TTokenVector dest, const TTokenVector source) const
	{
		memcpy(dest, source, GetVectorSize());
	}
	
	size_t GetVectorSize() const
	{
		return Tokens * sizeof(TToken);
	}

	// bit operators
	void ClearTokenVector(TTokenVector dest) const
	{
		BU::ClearAllBits(dest, Tokens);
	}
	
	TTokenVector AllocTokens(unsigned tokens) const
	{
		return (TTokenVector)malloc(tokens * sizeof(TToken));
	}

	TTokenVector ReallocTokens(TTokenVector v, unsigned tokens) const
	{
		return (TTokenVector)realloc(v, tokens * sizeof(TToken));
	}
	
public:
	Nfa(unsigned alpha)
		: 	
		AlphabetLenght(alpha),
		ActiveStates(NULL), 
		Tokens(0),
		TotalTokens(0),
		MaxStates(0),
		Initial(NULL),
		Final(NULL),
		Succesors(NULL), 
		Predecessors(NULL),
		AllMemory(NULL)
	{	
		ResizeFor(256);
		Clear();
	}

	Nfa(const Nfa& c)
			:	
		ActiveStates(NULL), 
		Tokens(0),
		TotalTokens(0),
		MaxStates(0),
		Initial(NULL),
		Final(NULL),
		Succesors(NULL), 
		Predecessors(NULL),
		AllMemory(NULL)
	{
		CloneFrom(nfa);
	}
	
	virtual ~Nfa()
	{
		free(AllMemory);
	}

	void Clear()
	{	
		auto totalSize = GetVectorSize()*3;
		memset(AllMemory, 0, totalSize);
	}
	
	void CloneFrom(const Nfa& c)
	{
		AlphabetLenght = nfa.AlphabetLenght;
		ResizeFor(nfa.MaxStates);
		TotalTokens = nfa.TotalTokens;
		auto totalSize = TotalTokens * sizeof(TToken);
		memcpy(AllMemory, nfa.AllMemory, totalSize);
		assert(Tokens == nfa.Tokens);
		assert(MaxStates == nfa.MaxStates);
		assert(AlphabetLenght == nfa.AlphabetLenght);
	}
	
	const TTokenVector GetPredecessors(unsigned state, TSymbol sym) const
	{
		return &Predecessors[_GetIndex(state, sym)];
	}

	const TTokenVector GetSuccesors(unsigned state, TSymbol sym) const
	{
		return &Succesors[_GetIndex(state, sym)];
	}

	const TTokenVector GetInitial() const
	{
		return Initial;
	}

	const TTokenVector GetFinal() const
	{
		return Final;
	}

	const TTokenVector GetActiveStates() const
	{
		return ActiveStates;
	}
		
	virtual void SetTransition(uint32_t src, uint32_t dest, TSymbol sym)
	{
		assert(sym < GetAlphabetLenght());
		
		// activar los estados
		ActivateState(src);
		ActivateState(dest);		
		
		BU::SetBit(_GetSuc(src, sym), dest);
		BU::SetBit(_GetPred(dest, sym), src);
	}

	virtual void SetInitial(uint32_t st)
	{
		// activar el estado
		ActivateState(st);
		BU::SetBit(Initial, st);
	}

	virtual void SetFinal(uint32_t st)
	{
		// activar el estado
		ActivateState(st);
		BU::SetBit(Final, st);
	}

	virtual bool IsInitial(uint32_t st) const
	{
		return BU::TestBit(GetInitial(), st);
	}
	
	virtual bool IsFinal(uint32_t st) const
	{
		return BU::TestBit(GetFinal(), st);
	}

	virtual bool IsActiveState(uint32_t st) const
	{
		return BU::TestBit(ActiveStates, st);
	}

	virtual bool ExistTransition(uint32_t src, uint32_t dest, TSymbol sym) const
	{
		return BU::TestBit(GetSuccesors(src, sym), dest);
	}

	virtual unsigned GetInactiveState() const
	{
		unsigned bit = 0;
		for(unsigned token=0; token<Tokens; token++)
		{		
			auto fetch = ~ActiveStates[token];
			unsigned long idx;
			if(BU::BitScanForward(&idx, fetch))
			{
				unsigned n = idx + bit;
				return n;
			}
			bit += BitsPerToken;
		}
		return bit;
	}

	virtual unsigned GetMaxStates() const
	{
		return MaxStates;
	}

	virtual unsigned GetAlphabetLenght() const
	{
		return AlphabetLenght;
	}

	virtual void Merge(uint32_t ns1, uint32_t ns2)
	{
		assert(_TestBit(ActiveStates, ns1));
		assert(_TestBit(ActiveStates, ns2));
		
		BU::OrAndClearSecondBit(Initial, ns1, ns2);
		BU::OrAndClearSecondBit(Final, ns1, ns2);
		BU::ClearBit(ActiveStates, ns2);
		
		for (TSymbol sym=0; sym<AlphabetLenght; sym++)
		{	
			unsigned long bitToken;

			// Ajustar Predecesores 

			auto predS1 = _GetPred(ns1, sym);
			auto predS2 = GetPredecessors(ns2, sym);
			BU::OrVector(predS1, predS1, predS2, Tokens); // ahora predecesores de s1 tambien tiene los de s2
			
			bitToken = 0;
			for(unsigned it=0; it<Tokens; it++)
			{
				TToken fetch = predS2[it];
				unsigned long idx = 0;
				while(BU::BitScanForward(&idx, fetch))
				{
					BU::ClearBit(&fetch, idx);
					unsigned state = idx + bitToken;
					auto suc = _GetSuc(state, sym);
					BU::SetBit(suc, ns1); // estado n ahora va a s1
					BU::ClearBit(suc, ns2); // estado n iba a s2
				}
				bitToken += BitsPerToken;
			}

			// Ajustar Sucesores

			auto sucS1 = _GetSuc(ns1, sym);
			auto sucS2 = GetSuccesors(ns2, sym);
			BU::OrVector(sucS1, sucS1, sucS2, Tokens);

			bitToken = 0;
			for(unsigned it=0; it<Tokens; it++)
			{
				TToken fetch = sucS2[it];
				unsigned long idx = 0;
				while(BU::BitScanForward(&idx, fetch))
				{
					BU::ClearBit(&fetch, idx);
					unsigned state = idx + bitToken;
					auto pred = _GetPred(state, sym);				
					BU::SetBit(pred, ns1); // estado n ahora viene de s1
					BU::ClearBit(pred, ns2); // estado n venia de s2
				}
				bitToken += BitsPerToken;
			}
		}
	}

	
	/** Indica si una muestra es reconocida por el automata
	*/
	virtual bool IsMatch(TVectorSymbolIterConst begin, TVectorSymbolIterConst end)
	{
		TTokenVector next = CreateTokenVector();
		TTokenVector current = CreateTokenVector();
		CloneTokenVector(current, Initial);
			
		for (auto i=begin; i!=end; i++)
		{
			unsigned sym = *i;
			ClearTokenVector(next);
			bool any = false;		
			unsigned BitIdx = 0;
			for(unsigned tokenIdx=0; tokenIdx<Tokens; tokenIdx++)
			{
				TToken fetch = current[tokenIdx];
				unsigned long idx;
				while(BU::BitScanForward(&idx, fetch))
				{
					BU::ClearBit(&fetch, idx);
					unsigned bit = BitIdx + idx;
					any = true;
					auto s = GetSuccesors(bit, sym);
					BU::OrVector(next, next, s, Tokens);
				}
				BitIdx += BitsPerToken;
			}		
			if(!any) return false;
			std::swap(next, current);
		}	
		
		auto match = BU::AnyBitOfAndVector(current, Final, Tokens);
		
		free(next);
		free(current);

		return match;
	}

	virtual void CopyBufferTo(void* memDest)
	{
		memcpy(memDest, AllMemory, TotalTokens*sizeof(TToken));
	}
};

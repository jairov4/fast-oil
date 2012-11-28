#include "StdAfx.h"
#include "Nfa.h"

using namespace std;

///////////////////UTIL
bool _SetBit(Nfa::TTokenVector vec, unsigned bit)
{
	return _bittestandset64((__int64*)vec, bit);
}

bool _ClearBit(Nfa::TTokenVector vec, unsigned bit)
{
	return _bittestandreset64((__int64*)vec, bit);
}

bool _TestBit(const Nfa::TTokenVector vec, unsigned bit)
{
	return _bittest64((__int64*)vec, bit);
}

void _ClearAllBits(Nfa::TTokenVector vec, unsigned tokens)
{
	memset(vec, 0, tokens * sizeof(Nfa::TToken));	
}

void _OrAndClearSecondBit(Nfa::TTokenVector vec, unsigned b1, unsigned b2)
{
	_bittest64(vec, b1);
	auto c2 = _bittestandreset64(vec, b2);
	if(c2) _bittestandset64(vec, b1);	
}

Nfa::TTokenVector AllocTokens(unsigned tokens)
{
	return (Nfa::TTokenVector)malloc(tokens * sizeof(Nfa::TToken));
}

Nfa::TTokenVector ReallocTokens(Nfa::TTokenVector v, unsigned tokens)
{
	return (Nfa::TTokenVector)realloc(v, tokens * sizeof(Nfa::TToken));
}
///////////////////!UTIL

/** Construye un nuevo automata no determinista vacio.
    La cantidad de estados del automata es variable pero la longitud del alfabeto debe ser
		especificada y no podra ser cambiada.
*/
Nfa::Nfa(unsigned alpha)
	: 	
	AlphabetLenght(alpha),
	ActiveStates(NULL), 
	Tokens(0),
	MaxStates(0),
	Initial(NULL),
	Final(NULL),
	Succesors(NULL), 
	Predecessors(NULL),
	AllMemory(NULL)
{	
	ResizeFor(256);
}

Nfa::~Nfa(void)
{
}

size_t Nfa::GetVectorSize() const
{
	return Tokens * sizeof(Nfa::TToken);
}

Nfa::TTokenVector Nfa::CreateTokenVector() const
{
	return AllocTokens(Tokens);
}

void Nfa::CloneTokenVector(Nfa::TTokenVector dest, const Nfa::TTokenVector source) const
{
	memcpy(dest, source, GetVectorSize());
}

void Nfa::ClearTokenVector(Nfa::TTokenVector dest) const
{
	_ClearAllBits(dest, Tokens);
}

// TODO: cambiar usando AVX-256 (haria 4 por instruccion)
void Nfa::OrTokenVector(Nfa::TTokenVector dest, const Nfa::TTokenVector v) const
{
	for(unsigned i=0; i<Tokens; i++)
	{
		dest[i] |= v[i];
	}
}

// TODO: cambiar usando AVX-256 (haria 4 por instruccion)
bool Nfa::AnyAndTokenVector(const Nfa::TTokenVector dest, const Nfa::TTokenVector v) const
{
	for(unsigned i=0; i<Tokens; i++)
	{
		if(dest[i] & v[i]) 
		{
			return true;
		}
	}	
	return false;
}

/** Indica si una muestra es reconocida por el automata
*/
bool Nfa::IsMatch(Nfa::TSampleConstIter begin, Nfa::TSampleConstIter end) const
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
			while(_BitScanForward64(&idx, fetch))
			{
				_ClearBit(&fetch, idx);
				unsigned bit = BitIdx + idx;
				any = true;
				auto s = GetSuccesors(bit, sym);
				OrTokenVector(next, s);
			}
			BitIdx += BitsPerToken;
		}		
		if(!any) return false;
		std::swap(next, current);
	}
	
	auto match = AnyAndTokenVector(current, Final);
	return match;
}

/** Indica si una muestra es reconocida por el automata
*/
bool Nfa::IsMatch( const TSample& sample ) const
{
	return IsMatch(sample.begin(), sample.end());
}

/** Combina los estados suministrados. El segundo estado es eliminado
*/
void Nfa::Merge( unsigned ns1, unsigned ns2 )
{
	assert(_TestBit(ActiveStates, ns1));
	assert(_TestBit(ActiveStates, ns2));
	
	_OrAndClearSecondBit(Initial, ns1, ns2);
	_OrAndClearSecondBit(Final, ns1, ns2);
	_ClearBit(ActiveStates, ns2);
	
	for (TSymbol sym=0; sym<AlphabetLenght; sym++)
	{	
		unsigned long bitToken;

		// Ajustar Predecesores 

		auto predS1 = _GetPred(ns1, sym);
		auto predS2 = GetPredecessors(ns2, sym);
		OrTokenVector(predS1, predS2); // ahora predecesores de s1 tambien tiene los de s2
		
		bitToken = 0;
		for(unsigned it=0; it<Tokens; it++)
		{
			TToken fetch = predS2[it];
			unsigned long idx = 0;
			while(_BitScanForward64(&idx, fetch))
			{
				_ClearBit(&fetch, idx);
				unsigned state = idx + bitToken;
				auto suc = _GetSuc(state, sym);
				_SetBit(suc, ns1); // estado n ahora va a s1
				_ClearBit(suc, ns2); // estado n iba a s2
			}
			bitToken += BitsPerToken;
		}

		// Ajustar Sucesores

		auto sucS1 = _GetSuc(ns1, sym);
		auto sucS2 = GetSuccesors(ns2, sym);
		OrTokenVector(sucS1, sucS2);

		bitToken = 0;
		for(unsigned it=0; it<Tokens; it++)
		{
			TToken fetch = sucS2[it];
			unsigned long idx = 0;
			while(_BitScanForward64(&idx, fetch))
			{
				_ClearBit(&fetch, idx);
				unsigned state = idx + bitToken;
				auto pred = _GetPred(state, sym);				
				_SetBit(pred, ns1); // estado n ahora viene de s1
				_ClearBit(pred, ns2); // estado n venia de s2
			}
			bitToken += BitsPerToken;
		}
	}
}

/** Activa un estado para que pueda ser usado
*/
void Nfa::ActivateState( unsigned st )
{
	if(st >= MaxStates)
	{
		ResizeFor(MaxStates * 2);
	}

	// si no estaba activo toca activarlo
	if(!_SetBit(ActiveStates, st))	
	{
		auto idxBegin = _GetIndex(st, 0);
		auto idxEnd = _GetIndex(st+1, 0);
		// si ya hay espacio solo los limpia
		memset(Succesors + idxBegin, 0, GetVectorSize()*AlphabetLenght);
		memset(Predecessors + idxBegin, 0, GetVectorSize()*AlphabetLenght);		
	}
}

void Nfa::ResizeFor(unsigned states)
{
	// Layout: Active, Initial, Final, Predecessors, Successors
	// Token allocation count:
	// Active, Initial, Final => Tokens
	// Predecessors, Successors => Tokens*MaxStates*AlphabetLenght
	// Tokens*3 + Tokens*MaxStates*AlphabetLenght*2
	Tokens = states / BitsPerToken + 1;
	MaxStates = Tokens * BitsPerToken;
	auto n = Tokens*3 + Tokens*MaxStates*AlphabetLenght*2;

	AllMemory = ReallocTokens(AllMemory, n);

	ActiveStates = &AllMemory[Tokens*0];
	Initial = &AllMemory[Tokens*1];
	Final = &AllMemory[Tokens*2];
	Predecessors = &AllMemory[Tokens*3 + Tokens*MaxStates*AlphabetLenght*0];
	Succesors = &AllMemory[Tokens*3 + Tokens*MaxStates*AlphabetLenght*1];

	// TODO: Falta acomodar los datos que ya estaban en sus nuevas locaciones
}

/** Añade la informacion necesaria a la estructura de datos para que el automata
    contenga una transicion desde el estado src al estado dest con el simbolo sym
*/
void Nfa::SetTransition( unsigned src, unsigned dest, TSymbol sym )
{
	assert(sym < GetAlphabetLenght());
	
	// activar los estados
	ActivateState(src);
	ActivateState(dest);		
	
	_SetBit(_GetSuc(src, sym), dest);
	_SetBit(_GetPred(dest, sym), src);
}

/** Ajusta el estado como estado inicial
*/
void Nfa::SetInitial( unsigned st )
{
	// activar el estado
	ActivateState(st);
	_SetBit(Initial, st);
}

/** Ajusta el estado como estado final
*/
void Nfa::SetFinal( unsigned st )
{
	// activar el estado
	ActivateState(st);
	_SetBit(Final, st);
}

/** Indica si un estado es inicial
*/
bool Nfa::IsInitial(unsigned st) const
{
	return _TestBit(GetInitial(), st);
}
	
/** Indica si un estado es final
*/
bool Nfa::IsFinal(unsigned st) const
{
	return _TestBit(GetFinal(), st);
}

/** Indica si existe una transicion entre el estado src y el estado dest a traves del simbolo sym
*/
bool Nfa::ExistTransition(unsigned src, unsigned dest, Nfa::TSymbol sym) const
{
	return _TestBit(GetSuccesors(src, sym), dest);
}

/** Obtiene un arreglo de bits que representa los estados que son predecesores de un
    estado determinado por un simbolo determinado
*/
Nfa::TTokenVector Nfa::_GetPred( unsigned state, TSymbol sym ) const
{
	return &Predecessors[_GetIndex(state, sym)];
}

/** Obtiene un arreglo de bits que representa los estados que son sucesores de un
    estado determinado por un simbolo determinado
*/
Nfa::TTokenVector Nfa::_GetSuc( unsigned state, TSymbol sym ) const
{
	return &Succesors[_GetIndex(state, sym)];
}

/** Obtiene un arreglo de bits que representa los estados que son predecesores de un
    estado determinado por un simbolo determinado
*/
const Nfa::TTokenVector Nfa::GetPredecessors( unsigned state, TSymbol sym ) const
{
	return &Predecessors[_GetIndex(state, sym)];
}

/** Obtiene un arreglo de bits que representa los estados que son sucesores de un
    estado determinado por un simbolo determinado
*/
const Nfa::TTokenVector Nfa::GetSuccesors( unsigned state, TSymbol sym ) const
{
	return &Succesors[_GetIndex(state, sym)];
}

/** Obtiene un vector de bits que representa los estados etiquetados como estados iniciales
*/
const Nfa::TTokenVector Nfa::GetInitial() const
{
	return Initial;
}

/** Obtiene un vector de bits que representa los estados etiquetados como estados finales
*/
const Nfa::TTokenVector Nfa::GetFinal() const
{
	return Final;
}

/** Obtiene un vector de bits que representa los estados etiquetados como estados activos
*/
const Nfa::TTokenVector Nfa::GetActiveStates() const
{
	return ActiveStates;
}

/** Obtiene la longitud del alfabeto del automata
*/
unsigned Nfa::GetAlphabetLenght() const
{
	return AlphabetLenght;
}

unsigned Nfa::GetMaxStates() const
{
	return MaxStates;
}

unsigned Nfa::GetInactiveState() const
{
	unsigned bit = 0;
	for(unsigned token=0; token<Tokens; token++)
	{		
		auto fetch = ~ActiveStates[token];
		unsigned long idx;
		if(_BitScanForward64(&idx, fetch))
		{
			unsigned n = idx + bit;
			return n;
		}
		bit += BitsPerToken;
	}
	return bit;
}

/** Obtiene el indice para ser usado con los vectores de predecesores y sucesores
*/
unsigned Nfa::_GetIndex( unsigned st, TSymbol sym ) const
{	
	assert(sym < AlphabetLenght);
	return st*AlphabetLenght+sym;
}


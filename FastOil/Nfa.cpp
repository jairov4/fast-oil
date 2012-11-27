#include "StdAfx.h"
#include "Nfa.h"

using namespace std;

///////////////////UTIL
void _SetBit(Nfa::TTokenVector vec, unsigned bit)
{
	_bittestandset64((__int64*)vec, bit);
}

void _ClearBit(Nfa::TTokenVector vec, unsigned bit)
{
	_bittestandreset64((__int64*)vec, bit);
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
	ActiveStates(), 
	Initial(),
	Final(),
	Succesors(), 
	Predecessors()
{	
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

// TODO: cambiar usando AVX
void Nfa::ClearTokenVector(Nfa::TTokenVector dest) const
{
	_ClearAllBits(dest, Tokens);
}

// TODO: cambiar usando AVX (haria 4 por instruccion)
void Nfa::OrTokenVector(Nfa::TTokenVector dest, const Nfa::TTokenVector v) const
{
	for(unsigned i=0; i<Tokens; i++)
	{
		dest[i] |= v[i];
	}
}

// TODO: cambiar usando AVX (haria 4 por instruccion)
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
	unsigned sym;	
	for (auto i=begin; i!=end; i++)
	{
		sym = *i;
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
				_SetBit(pred, ns1); // estado n ahora va a s1
				_ClearBit(pred, ns2); // estado n iba a s2
			}
			bitToken += BitsPerToken;
		}
	}
}

/** Activa un estado para que pueda ser usado
*/
void Nfa::ActivateState( unsigned st )
{	
	// si no estaba activo toca activarlo
	if(!ActiveStates.Set(st))
	{
		auto idxBegin = _GetIndex(st, 0);
		auto idxEnd = idxBegin + AlphabetLenght;
		if(idxEnd > Succesors.size())
		{
			Succesors.resize(idxEnd);
			Predecessors.resize(idxEnd);
		} 
		// si ya hay espacio solo los limpia
		for (auto it = idxBegin; it != idxEnd; it++)
		{
			Succesors[it].ClearAll();
			Predecessors[it].ClearAll();
		}		
	}
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

/** Obtiene un arreglo de bits que representa los estados que son predecesores de un
    estado determinado por un simbolo determinado
*/
Nfa::TTokenVector Nfa::_GetPred( unsigned state, TSymbol sym )
{
	return &Predecessors[_GetIndex(state, sym)];
}

/** Obtiene un arreglo de bits que representa los estados que son sucesores de un
    estado determinado por un simbolo determinado
*/
Nfa::TTokenVector Nfa::_GetSuc( unsigned state, TSymbol sym )
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

/** Obtiene el indice para ser usado con los vectores de predecesores y sucesores
*/
unsigned Nfa::_GetIndex( unsigned st, TSymbol sym ) const
{	
	assert(sym < AlphabetLenght);
	return st*AlphabetLenght+sym;
}


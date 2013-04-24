#include "StdAfx.h"
#include "Nfa.h"

using namespace std;

///////////////////UTIL
bool _SetBit(Nfa::TTokenVector vec, unsigned bit)
{
	return (bool)_bittestandset64((__int64*)vec, bit);
}

bool _ClearBit(Nfa::TTokenVector vec, unsigned bit)
{
	return (bool)_bittestandreset64((__int64*)vec, bit);
}

bool _TestBit(const Nfa::TTokenVector vec, unsigned bit)
{
	return (bool)_bittest64((__int64*)vec, bit);
}

void _ClearAllBits(Nfa::TTokenVector vec, unsigned tokens)
{
	memset(vec, 0, tokens * sizeof(Nfa::TToken));	
}

void _OrAndClearSecondBit(Nfa::TTokenVector vec, unsigned b1, unsigned b2)
{	
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

Nfa::Nfa(const Nfa& nfa)
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

Nfa::~Nfa(void)
{
	free(AllMemory);
}

void Nfa::Clear()
{	
	auto totalSize = GetVectorSize()*3;
	memset(AllMemory, 0, totalSize);
}

void Nfa::CloneFrom(const Nfa& nfa)
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

void Nfa::OrTokenVector(Nfa::TTokenVector dest, const Nfa::TTokenVector v) const
{	
#ifndef _NOT_USE_AVX256	
	assert(Tokens * BitsPerToken % 256 == 0); // tokens debe ser multiplo de 256
	const int incr = 256 / BitsPerToken; // AVX use 256-bit registers
	for(unsigned i=0; i<Tokens; i+=incr)
	{
		__m256 rs = _mm256_loadu_ps((const float*)&v[i]);
		__m256 rd = _mm256_loadu_ps((const float*)&dest[i]);		
		rd = _mm256_or_ps(rs, rd);
		_mm256_storeu_ps((float*)&dest[i], rd);
	}
#else
	/* NON AVX */
	for(unsigned i=0; i<Tokens; i++)
	{
		dest[i] |= v[i];
	}
#endif
}

bool Nfa::AnyAndTokenVector(const Nfa::TTokenVector dest, const Nfa::TTokenVector v) const
{	
#ifndef _NOT_USE_AVX256	
	assert(Tokens * BitsPerToken % 256 == 0); // tokens debe ser multiplo de 256
	const int incr = 256 / BitsPerToken; // AVX use 256-bit registers
	for(unsigned i=0; i<Tokens; i+=incr)
	{
		__m256i rs = _mm256_loadu_si256((__m256i*)&v[i]);
		__m256i rd = _mm256_loadu_si256((__m256i*)&dest[i]);
		if(!_mm256_testz_si256(rd, rs)) return true;
	}
#else
	/* NON AVX */
	for(unsigned i=0; i<Tokens; i++)
	{
		if(dest[i] & v[i]) 
		{
			return true;
		}
	}	
#endif
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
	
	free(next);
	free(current);

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

	// si no estaba activo se realiza algo de limpieza
	if(!_SetBit(ActiveStates, st))	
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

void Nfa::_MoveActiveTokenVectors(TTokenVector dest, const TTokenVector source, unsigned beforeTokens, size_t beforeVectorSize)
{		
	unsigned bitToken = 0;
	for(unsigned it=0; it<beforeTokens; it++)
	{
		TToken fetch = ActiveStates[beforeTokens-it-1];
		unsigned long idx = 0;
		
		while(_BitScanReverse64(&idx, fetch))
		{
			_ClearBit(&fetch, idx);
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

void Nfa::ResizeFor(unsigned states)
{
	unsigned beforeTokens = Tokens;
	unsigned beforeMaxStates = MaxStates;
	size_t beforeVectorSize = GetVectorSize();

#ifndef _NOT_USE_AVX256
	assert(BitsPerToken <= 256); // Condicion necesaria para la compatibilidad con AVX
	// asegura que la cantidad de estados sea multiplo de 256 para aplicar instrucciones AVX
	states = ((states - 1) / 256 + 1) * 256;
#endif

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

bool Nfa::IsActiveState(unsigned st) const
{
	return _TestBit(ActiveStates, st);
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
	return _GetIndex(st, sym, Tokens);
}

unsigned Nfa::_GetIndex( unsigned st, TSymbol sym, unsigned Tokens) const
{
	assert(sym < AlphabetLenght);
	return (st*AlphabetLenght+sym)*Tokens;
}
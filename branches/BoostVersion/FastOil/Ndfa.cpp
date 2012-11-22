#include "StdAfx.h"
#include "Ndfa.h"

using namespace std;

/** Construye un nuevo automata no determinista vacio.
    La cantidad de estados del automata es variable pero la longitud del alfabeto debe ser
		especificada y no podra ser cambiada.
*/
Ndfa::Ndfa(unsigned alpha)
	: 	
	AlphabetLenght(alpha),
	ActiveStates(), 
	Initial(),
	Final(),
	Succesors(), 
	Predecessors()
{
	ActiveStates.ClearAll();
	Initial.ClearAll();
	Final.ClearAll();
}

Ndfa::~Ndfa(void)
{
}

/** Indica si una muestra es reconocida por el automata
*/
bool Ndfa::IsMatch(Ndfa::TSampleConstIter begin, Ndfa::TSampleConstIter end) const
{
	BitVector* next = &BitVector();
	BitVector* current = &BitVector(Initial); // empezamos con una copia de Initial
	unsigned sym;
	for (auto i=begin; i!=end; i++)
	{
		sym = *i;
		next->ClearAll();
		bool any = false;
		for (auto it=current->GetBitSetIterator(); !it.IsEnd(); it.Next())
		{
			auto n = it.GetBit();
			any = true;
			// pongo "const auto&" con el fin de obtener una referencia 
			// sin posibilidad de modificacion lo cual no requiere la copia
			// del objeto completo
			const auto& a = GetSuccesors(n, sym);
			next->Or(a);
		}		
		if(!any) return false;
		std::swap(next, current);
	}
	auto match = current->TestAnd(Final);
	return match;
}

/** Indica si una muestra es reconocida por el automata
*/
bool Ndfa::IsMatch( const TSample& sample ) const
{
	return IsMatch(sample.begin(), sample.end());
}

/** Combina los estados suministrados. El segundo estado es eliminado
*/
void Ndfa::Merge( unsigned ns1, unsigned ns2 )
{
	assert(ActiveStates.Test(ns1));
	assert(ActiveStates.Test(ns2));

	Initial.OrBitsAndClearSecond(ns1, ns2);
	Final.OrBitsAndClearSecond(ns1, ns2);
	ActiveStates.Clear(ns2);

	for (TSymbol sym=0; sym<AlphabetLenght; sym++)
	{
		auto& predS1 = _GetPred(ns1, sym);
		auto& predS2 = _GetPred(ns2, sym);
		predS1.Or(predS2); // ahora predecesores de s1 tambien tiene los de s2
		for (auto it=predS2.GetBitSetIterator(); !it.IsEnd(); it.Next())
		{
			auto n = it.GetBit();
			auto& suc = _GetSuc(n, sym);
			suc.Set(ns1); // estado n ahora va a s1
			suc.Clear(ns2); // estado n iba a s2
		}		

		auto& sucS1 = _GetSuc(ns1, sym);
		auto& sucS2 = _GetSuc(ns2, sym);
		sucS1.Or(sucS2);
		for (auto it=sucS2.GetBitSetIterator(); !it.IsEnd(); it.Next())
		{
			auto n = it.GetBit();
			auto& pred = _GetPred(n, sym);
			pred.Set(ns1);
			pred.Clear(ns2);			
		}
	}
}

/** Activa un estado para que pueda ser usado
*/
void Ndfa::ActivateState( unsigned st )
{
	assert(st < BitVector::TotalBits);
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
void Ndfa::SetTransition( unsigned src, unsigned dest, TSymbol sym )
{
	assert(sym < GetAlphabetLenght());
	
	// activar los estados
	ActivateState(src);
	ActivateState(dest);		

	_GetSuc(src, sym).Set(dest);
	_GetPred(dest, sym).Set(src);
}

/** Ajusta el estado como estado inicial
*/
void Ndfa::SetInitial( unsigned st )
{
	// activar el estado
	ActivateState(st);
	Initial.Set(st);
}

/** Ajusta el estado como estado final
*/
void Ndfa::SetFinal( unsigned st )
{
	// activar el estado
	ActivateState(st);
	Final.Set(st);
}

/** Obtiene un arreglo de bits que representa los estados que son predecesores de un
    estado determinado por un simbolo determinado
*/
BitVector& Ndfa::_GetPred( unsigned state, TSymbol sym )
{
	return Predecessors[_GetIndex(state, sym)];
}

/** Obtiene un arreglo de bits que representa los estados que son sucesores de un
    estado determinado por un simbolo determinado
*/
BitVector& Ndfa::_GetSuc( unsigned state, TSymbol sym )
{
	return Succesors[_GetIndex(state, sym)];
}

/** Obtiene un arreglo de bits que representa los estados que son predecesores de un
    estado determinado por un simbolo determinado
*/
const BitVector& Ndfa::GetPredecessors( unsigned state, TSymbol sym ) const
{
	return Predecessors[_GetIndex(state, sym)];
}

/** Obtiene un arreglo de bits que representa los estados que son sucesores de un
    estado determinado por un simbolo determinado
*/
const BitVector& Ndfa::GetSuccesors( unsigned state, TSymbol sym ) const
{
	return Succesors[_GetIndex(state, sym)];
}

/** Obtiene un vector de bits que representa los estados etiquetados como estados iniciales
*/
const BitVector& Ndfa::GetInitial() const
{
	return Initial;
}

/** Obtiene un vector de bits que representa los estados etiquetados como estados finales
*/
const BitVector& Ndfa::GetFinal() const
{
	return Final;
}

/** Obtiene un vector de bits que representa los estados etiquetados como estados activos
*/
const BitVector& Ndfa::GetActiveStates() const
{
	return ActiveStates;
}

/** Obtiene la longitud del alfabeto del automata
*/
unsigned Ndfa::GetAlphabetLenght() const
{
	return AlphabetLenght;
}

/** Obtiene el indice para ser usado con los vectores de predecesores y sucesores
*/
unsigned Ndfa::_GetIndex( unsigned st, TSymbol sym ) const
{	
	assert(sym < AlphabetLenght);
	return st*AlphabetLenght+sym;
}


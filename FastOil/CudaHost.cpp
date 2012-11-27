#include "stdafx.h"
#include "Ndfa.h"
#include <cuda_runtime.h>

void cuda_proxy(const Ndfa& ndfa, Ndfa::TSampleConstIter begin, Ndfa::TSampleConstIter end)
{
	
}

bool Ndfa::IsMatchUsingCuda(Ndfa::TSampleConstIter begin, Ndfa::TSampleConstIter end) const
{	
	cuda_proxy(*this, begin, end);
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
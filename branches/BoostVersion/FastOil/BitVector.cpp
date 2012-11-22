#include "stdafx.h"
#include "BitVector.h"

/** Este constructor no inicializa en ceros el vector de bits, 
	si lo requiere debe llamar explicitamente a ClearAll()
*/
BitVector::BitVector(void)
{	
}

BitVector::BitVector(const BitVector& vec)
{
	memcpy_s(Tokens, TotalTokens*sizeof(TokenType), vec.Tokens, TotalTokens*sizeof(TokenType));
}

BitVector::~BitVector(void)
{
}

/** Pone en uno el bit indicado
*/
bool BitVector::Set( unsigned n )
{
	assert(n < TotalBits);
	return _bittestandset64((__int64*)Tokens, n) != 0;
}

/** Pone en cero el bit indicado
*/
bool BitVector::Clear( unsigned n )
{
	assert(n < TotalBits);
	return _bittestandreset64((__int64*)Tokens, n) != 0;
}

void BitVector::And(const BitVector& v)
{
	for (int i=0; i<TotalTokens; i++)
	{
		Tokens[i] &= v.Tokens[i];
	}
}

void BitVector::Or(const BitVector& v)
{
	for (int i=0; i<TotalTokens; i++)
	{
		Tokens[i] |= v.Tokens[i];
	}
}

void BitVector::Xor(const BitVector& v)
{
	for (int i=0; i<TotalTokens; i++)
	{
		Tokens[i] ^= v.Tokens[i];
	}
}

void BitVector::Not()
{
	for (int i=0; i<TotalTokens; i++)
	{
		Tokens[i] = ~Tokens[i];
	}
}

/** Comprueba si el bit indicado esta activo
*/
bool BitVector::Test( unsigned n ) const
{
	assert(n < TotalBits);
	return _bittest64((__int64*)Tokens, n) != 0;
}

/** Comprueba si al menos un bit esta activo
*/
bool BitVector::TestAny() const
{
	for (int i=0; i<TotalTokens; i++)
	{
		if(Tokens[i] != 0) return true;
	}
	return false;
}

/** Comprueba si como resultado de la operacion AND bit a bit al menos
	un bit termina activo
*/
bool BitVector::TestAnd(const BitVector& v) const
{
	for (int i=0; i<TotalTokens; i++)
	{
		if((Tokens[i] & v.Tokens[i]) != 0) return true;
	}
	return false;
}

/** Comprueba si como resultado de la operacion OR bit a bit al menos
	un bit termina activo
*/
bool BitVector::TestOr(const BitVector& v) const
{
	for (int i=0; i<TotalTokens; i++)
	{
		if((Tokens[i] | v.Tokens[i]) != 0) return true;
	}
	return false;
}

/** Comprueba si como resultado de la operacion XOR bit a bit al menos
	un bit termina activo
*/
bool BitVector::TestXor(const BitVector& v) const
{
	for (int i=0; i<TotalTokens; i++)
	{
		if((Tokens[i] ^ v.Tokens[i]) != 0) return true;
	}
	return false;
}

/** Pone todos los bits en cero (desactivado)
*/
void BitVector::ClearAll()
{
	memset(Tokens, 0, sizeof(Tokens));
}

/** realiza la Or de los bits indicados. El resultado se almacena en el primer operando
	mientras el segundo siempre se deshabilita (pone en cero)
*/
void BitVector::OrBitsAndClearSecond( unsigned n1, unsigned n2 )
{	
	assert(n1 < TotalBits);
	assert(n2 < TotalBits);
	auto r1 = _bittest64((__int64*)Tokens, n1);
	auto r2 = _bittestandreset64((__int64*)Tokens, n2);
	if(r1 || r2) _bittestandset64((__int64*)Tokens, n1);
}

/** Obtiene un iterador para recorrer los bits habilitados en el vector
*/
BitVector::BitSetIterator BitVector::GetBitSetIterator() const {
	return BitSetIterator(*this);
}

BitVector::BitSetIterator::BitSetIterator(const BitVector& vec)
	: tokens(vec.Tokens), token(0), bitn(0)
{
	fetch = tokens[token];
	Next();
}

bool BitVector::BitSetIterator::IsEnd() const 
{
	return token == TotalTokens;
}

unsigned BitVector::BitSetIterator::GetBit() const
{
	return lastbit;
}

BitVector& BitVector::operator=(const BitVector& vec)
{
	memcpy_s(Tokens, TotalTokens*sizeof(TokenType), vec.Tokens, TotalTokens*sizeof(TokenType));
	return *this;
}

void BitVector::BitSetIterator::Next()
{
	assert(!IsEnd());
	unsigned long idx;
	while(!_BitScanForward64(&idx, fetch))
	{
		bitn += BitsPerToken;
		token++;
		if(!IsEnd()) fetch = tokens[token];
		else return;
	}
	_bittestandreset64((__int64*)&fetch, idx);
	lastbit = idx + bitn;
}
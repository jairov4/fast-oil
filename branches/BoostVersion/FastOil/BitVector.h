#pragma once

#include <functional>

class BitVector
{
public:
	typedef unsigned __int64 TokenType;
	static const unsigned int TotalTokens = 64;
	static const unsigned int BitsPerToken = sizeof(TokenType)*8;
	static const unsigned int TotalBits = TotalTokens * BitsPerToken;	

	class BitSetIterator
	{
		const TokenType* tokens;
		TokenType fetch;
		unsigned token;
		unsigned bitn;		
		unsigned lastbit;
	public:
		BitSetIterator(const BitVector& vec);
		void Next();		
		bool IsEnd() const;
		unsigned GetBit() const;
	};
	
private:
	TokenType Tokens[TotalTokens];

public:	
	BitVector(void);
	BitVector(const BitVector& vec);
	~BitVector(void);

	bool Set(unsigned n);
	bool Clear(unsigned n);
	void ClearAll();
	
	void And(const BitVector& v);
	void Or(const BitVector& v);
	void Xor(const BitVector& v);
	void Not();

	bool Test(unsigned n) const;
	bool TestAny() const;

	bool TestAnd(const BitVector& v) const;
	bool TestOr(const BitVector& v) const;
	bool TestXor(const BitVector& v) const;

	BitSetIterator GetBitSetIterator() const;

	BitVector& operator=(const BitVector& vec);

	void OrBitsAndClearSecond(unsigned n1, unsigned n2);
};


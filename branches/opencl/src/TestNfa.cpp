#include "StdAfx.h"
#include "Nfa.h"

using namespace std;

namespace Testing {

#define ASSIGN_VEC(tv, s) (tv).assign((s), (s)+sizeof(s))
#define MATCH(s) nfa.IsMatch((s).cbegin(), (s).cend())

	void test_assert(bool b)
	{
		if(!b) throw runtime_error("Test assert not pass");
	}

	template<class T>
	void Test1()
	{
		Nfa<uint8_t, T, false> nfa(5);
		vector<uint8_t> tv;

		// {0}-3->(1)
		nfa.SetTransition(0,1, 3);
		nfa.SetInitial(0);
		nfa.SetFinal(1);

		uint8_t array01[] = { 1,2,3,4 };
		uint8_t array02[] = { 1 };
		uint8_t array03[] = { 3 };

		ASSIGN_VEC(tv, array01);
		test_assert(!MATCH(tv));

		ASSIGN_VEC(tv, array02);
		test_assert(!MATCH(tv));

		ASSIGN_VEC(tv, array03);
		test_assert(MATCH(tv));
				
		nfa.Clear();

		//               -0-
		//               \ /
		// {0}-3->[1]-4->[2]-5->[3]-4->[4]-3->(5)
		nfa.SetTransition(0,1, 3);
		nfa.SetTransition(1,2, 4);
		nfa.SetTransition(2,3, 5);
		nfa.SetTransition(3,4, 4);
		nfa.SetTransition(4,5, 3);
		nfa.SetTransition(2,2, 0);
		nfa.SetInitial(0);
		nfa.SetFinal(5);

		uint8_t array04[] = { 3, 3, 3, 4, 0, 0, 0, 5, 4, 3 };
		uint8_t array05[] = { 3, 4, 0, 0, 0, 0, 0, 5, 4, 3, 3, 3 };
		uint8_t array06[] = { 3, 4, 0, 0, 0, 0, 0, 5, 4, 3 };
		uint8_t array07[] = { 3, 4, 0, 0, 0, 0, 5, 4, 3 };
		uint8_t array08[] = { 3, 4, 0, 0, 0, 5, 4, 3 };
		uint8_t array09[] = { 3, 4, 0, 0, 5, 4, 3 };
		uint8_t array10[] = { 3, 4, 0, 5, 4, 3 };
		uint8_t array11[] = { 3, 4, 5, 4, 3 };
		uint8_t array12[] = { 3, 4, 4, 3 };

		ASSIGN_VEC(tv, array04);
		test_assert(!MATCH(tv));
		
		ASSIGN_VEC(tv, array05);
		test_assert(!MATCH(tv));

		ASSIGN_VEC(tv, array06);
		test_assert(MATCH(tv));

		ASSIGN_VEC(tv, array07);
		test_assert(MATCH(tv));

		ASSIGN_VEC(tv, array08);
		test_assert(MATCH(tv));

		ASSIGN_VEC(tv, array09);
		test_assert(MATCH(tv));

		ASSIGN_VEC(tv, array10);
		test_assert(MATCH(tv));

		ASSIGN_VEC(tv, array11);
		test_assert(MATCH(tv));

		ASSIGN_VEC(tv, array12);
		test_assert(!MATCH(tv));
		
		tv.clear();
		tv.push_back(3);
		tv.push_back(4);
		for(int i=0; i<700; i++) tv.push_back(0);
		tv.push_back(5);
		tv.push_back(4);
		tv.push_back(3);		
		test_assert(MATCH(tv));

		tv.push_back(3);
		test_assert(!MATCH(tv));
	}
		
	/*void Test2()
	{
		Nfa nfa(2);		
		nfa.SetTransition(0, 1, 0);
		nfa.SetTransition(0, 0, 1);
		nfa.SetInitial(0);
		nfa.SetFinal(1);
		NfaDotExporter::Export(nfa, "test2.dot");
	}

	void Test3()
	{
		Nfa nfa(2);		
		nfa.SetTransition(0, 1, 0);
		nfa.SetTransition(0, 0, 1);
		nfa.SetInitial(0);
		nfa.SetFinal(1);
		nfa.Merge(0, 1);
		NfaDotExporter::Export(nfa, "test3.dot");
	}*/
		
}

void main(void)
{
	Testing::Test1<uint64_t>();	
	Testing::Test1<uint32_t>();	
	cout << "All tests succeded";
}
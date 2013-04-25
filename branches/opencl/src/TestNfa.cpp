#include "StdAfx.h"
#include "Nfa.h"

using namespace std;

namespace Testing {

	void test_assert(bool b)
	{
		if(!b) throw runtime_error("Test assert not pass");
	}

	void Test1()
	{		
		Nfa<uint8_t, uint64_t, false> nfa(5);
		nfa.SetTransition(0, 1, 3);
		nfa.SetInitial(0);
		nfa.SetFinal(1);

		uint8_t array1[] = {1,2,3,4};
		uint8_t array2[] = {1};
		uint8_t array3[] = {3};

		vector<uint8_t> v1(array1, array1+4);
		vector<uint8_t> v2(array2, array2+1);
		vector<uint8_t> v3(array3, array3+1);
				
		test_assert(!nfa.IsMatch(v1.cbegin(), v1.cend()));
		test_assert(!nfa.IsMatch(v2.cbegin(), v2.cend()));
		test_assert(nfa.IsMatch(v3.cbegin(), v3.cend()));
	}

	void Test2()
	{		
		Nfa<uint8_t, uint32_t, false> nfa(5);
		nfa.SetTransition(0, 1, 3);
		nfa.SetInitial(0);
		nfa.SetFinal(1);

		uint8_t array1[] = {1,2,3,4};
		uint8_t array2[] = {1};
		uint8_t array3[] = {3};

		vector<uint8_t> v1(array1, array1+4);
		vector<uint8_t> v2(array2, array2+1);
		vector<uint8_t> v3(array3, array3+1);
				
		test_assert(!nfa.IsMatch(v1.cbegin(), v1.cend()));
		test_assert(!nfa.IsMatch(v2.cbegin(), v2.cend()));
		test_assert(nfa.IsMatch(v3.cbegin(), v3.cend()));
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
	Testing::Test1();
	Testing::Test2();	
}
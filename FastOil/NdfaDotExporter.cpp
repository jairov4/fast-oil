#include "StdAfx.h"
#include "NdfaDotExporter.h"

using namespace std;
using namespace boost::algorithm;
using boost::lexical_cast;

const string initialStyle = "style=\"filled\"";
const string finalStyle = "style=\"bold,dashed\"";
const string initialFinalStyle = "style=\"filled,bold,dashed\"";

void NdfaDotExporter::Export(const Ndfa& ndfa, std::string filename)
{
	ofstream out(filename);
	
	out << "digraph \"NDFA\" {" << endl;
	out << "  rankdir=LR" << endl;
	out << "  node [shape=box width=0.1 height=0.1 fontname=Arial]" << endl;
	out << "  edge [fontname=Arial]" << endl;

	out << "/* Estados */" << endl;
	for (auto it=ndfa.GetActiveStates().GetBitSetIterator(); !it.IsEnd(); it.Next())
	{
		unsigned i = it.GetBit();
	
		auto isInitial = ndfa.GetInitial().Test(i);
		auto isFinal = ndfa.GetFinal().Test(i);
		auto isDeleted = !ndfa.GetActiveStates().Test(i);
		if(isDeleted) continue;
		string fmt;
		if (isInitial && !isFinal) fmt = initialStyle;
		else if (!isInitial && isFinal) fmt = finalStyle;
		else if (isInitial && isFinal) fmt = initialFinalStyle;
		
		out << " s" << i << " [label=\"" << i << "\" " << fmt << "] " 
			<< "/* I:"<< isInitial
			<< " F:" << isFinal
			<< " D:" << isDeleted
			<< " */" << endl;
	}

	list<string> l;
	out << "/* Transiciones */" << endl;
	for (auto it1=ndfa.GetActiveStates().GetBitSetIterator(); !it1.IsEnd(); it1.Next())
	{
		unsigned i = it1.GetBit();

		if(!ndfa.GetActiveStates().Test(i)) continue;
		for (auto it2=ndfa.GetActiveStates().GetBitSetIterator(); !it2.IsEnd(); it2.Next())
		{
			unsigned k = it2.GetBit();

			l.clear();
			for (unsigned j = 0; j < ndfa.GetAlphabetLenght(); j++)
			{
				if (!ndfa.GetSuccesors(i, j).Test(k)) continue;
				l.push_back(lexical_cast<string>(j));
			}
			if (l.size() > 0)
			{
				string str;
				auto syms = join(l, ","); // une por comas 2,3,7,22,....
				out << "  s" << i << " -> s" << k << " [label=\"" << syms << "\"]" << endl;
			}
		}
	}
	out << "}" << endl;
	out.close();
}


void NdfaDotExporter::ExportDestinoPlainText(const Ndfa& ndfa, std::string filename)
{
	ofstream out(filename);	

	out << "# Alfabeto" << endl;
	out << "{";	
	for(unsigned i=0; i<ndfa.GetAlphabetLenght(); i++)
	{
		out << i << (i==ndfa.GetAlphabetLenght()-1? "" : ",");
	}
	out << "}" << endl << endl;

	out << "# Numero de estados" << endl;
	int states = 0;
	for(auto it=ndfa.GetActiveStates().GetBitSetIterator(); !it.IsEnd(); it.Next())
	{
		// contar estados activos
		states++;
	}
	out << states << endl;

	out << "# Estados iniciales" << endl;
	for(auto it=ndfa.GetInitial().GetBitSetIterator(); !it.IsEnd(); it.Next())
	{
		out << it.GetBit() << " ";
	}
	out << endl;

	out << "# Estados finales" << endl;
	for(auto it=ndfa.GetFinal().GetBitSetIterator(); !it.IsEnd(); it.Next())
	{
		out << it.GetBit() << " ";
	}
	out << endl;

	out << "# Descripcion de las transiciones" << endl;	
	ostringstream buffer;
	int transitions = 0;
	int st1=0;
	for (auto it1=ndfa.GetActiveStates().GetBitSetIterator(); !it1.IsEnd(); it1.Next())
	{
		unsigned i = it1.GetBit();		
		int st2=0;
		for (auto it2=ndfa.GetActiveStates().GetBitSetIterator(); !it2.IsEnd(); it2.Next())
		{
			unsigned k = it2.GetBit();			
			for (unsigned j = 0; j < ndfa.GetAlphabetLenght(); j++)
			{
				// estado(st1) con estado(st2) con simbolo(j)
				if (!ndfa.GetSuccesors(i, j).Test(k)) continue;
				buffer << st1 << " " << st2 << " " << j << endl;				
				transitions++;
			}			
			st2++;
		}
		st1++;
	}
	out << transitions << endl;
	out << buffer.str();
	out.close();
}
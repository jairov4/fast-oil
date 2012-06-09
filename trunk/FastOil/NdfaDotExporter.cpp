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


vector<string> _splitBySpaces( string line );


void NdfaDotExporter::ExportDestinoPlainText(const Ndfa& ndfa, std::string filename)
{
	ofstream out(filename);	

	out << "# Alfabeto" << endl;	
	out << ndfa.GetAlphabetLenght() << endl;
	
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

Ndfa NdfaDotExporter::ImportDestinoPlainText(std::string filename)
{
	ifstream file(filename);
	if(!file.is_open())
	{
		throw std::exception("No fue posible abrir el modelo");
	}
	
	string line;
	Ndfa ndfa(1);
	enum { header_alphabet, header_states, header_initial, header_final, header_transitions_count, body_transitions } state;
	state = header_alphabet;

	while(!file.eof())
	{
		// Leemos la informacion de cabecera
		getline(file, line);
		trim(line);
		if(line[0] == '#') continue;

		if(state == header_alphabet)
		{
			vector<string> splits;
			int alpha = lexical_cast<int>(line);
			ndfa = Ndfa(alpha);
			state = header_states;
		} 
		else if(state == header_states)
		{
			// omitimos esto
			state = header_initial;
		}
		else if(state == header_initial)
		{
			auto splits = _splitBySpaces(line);
			for_each(splits.begin(), splits.end(), [&ndfa](string item){ ndfa.SetInitial(lexical_cast<int>(item)); });
			state = header_final;
		}
		else if(state == header_final)
		{
			auto splits = _splitBySpaces(line);
			for_each(splits.begin(), splits.end(), [&ndfa](string item){ ndfa.SetFinal(lexical_cast<int>(item)); });
			state = header_transitions_count;
		}
		else if(state == header_transitions_count)
		{
			// omitimos esto
			state = body_transitions;
		}
		else if(state == body_transitions)
		{
			auto splits = _splitBySpaces(line);
			ndfa.SetTransition(lexical_cast<int>(splits[0]), lexical_cast<int>(splits[1]), lexical_cast<int>(splits[2]));
		}
	}
	file.close();
}
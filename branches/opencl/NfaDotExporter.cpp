#include "StdAfx.h"
#include "NfaDotExporter.h"

using namespace std;
using namespace boost::algorithm;
using boost::lexical_cast;

const string initialStyle = "style=\"filled\"";
const string finalStyle = "style=\"bold,dashed\"";
const string initialFinalStyle = "style=\"filled,bold,dashed\"";

void NfaDotExporter::Export(const Nfa& nfa, std::string filename)
{
	ofstream out(filename);
	
	out << "digraph \"NDFA\" {" << endl;
	out << "  rankdir=LR" << endl;
	out << "  node [shape=box width=0.1 height=0.1 fontname=Arial]" << endl;
	out << "  edge [fontname=Arial]" << endl;

	map<unsigned, unsigned> active;
	for (unsigned i=0; i<nfa.GetMaxStates(); i++)
	{
		// contar estados activos
		if(nfa.IsActiveState(i))
		{
			active[(unsigned)active.size()] = i;
		}		
	}

	out << "/* Estados */" << endl;		
	for (auto i=active.begin(); i!=active.end(); i++)
	{	
		auto isInitial = nfa.IsInitial(i->second);
		auto isFinal = nfa.IsFinal(i->second);
		
		string fmt;
		if (isInitial && !isFinal) fmt = initialStyle;
		else if (!isInitial && isFinal) fmt = finalStyle;
		else if (isInitial && isFinal) fmt = initialFinalStyle;
		
		out << " s" << i->first << " [label=\"" << i->first << "\" " << fmt << "] " 
			<< "/* I:"<< isInitial
			<< " F:" << isFinal			
			<< " ORG:" << i->second
			<< " */" << endl;
	}

	list<string> l;
	out << "/* Transiciones */" << endl;
	for (auto i=active.begin(); i!=active.end(); i++)
	{		
		for (auto k=active.begin(); k!=active.end(); k++)
		{			
			l.clear();
			for (Nfa::TSymbol j = 0; j<nfa.GetAlphabetLenght(); j++)
			{
				auto s = nfa.GetSuccesors(i->second, j);
				if (!_TestBit(s, k->second)) continue;
				l.push_back(lexical_cast<string>(j));
			}
			if (l.size() > 0)
			{
				string str;
				auto syms = join(l, ","); // une por comas 2,3,7,22,....
				out << "  s" << i->first << " -> s" << k->first << " [label=\"" << syms << "\"]" << endl;
			}
		}
	}
	out << "}" << endl;
	out.close();
}


vector<string> _splitBySpaces(const string& line);

void NfaDotExporter::ExportDestinoPlainText(const Nfa& nfa, std::string filename)
{
	ofstream out(filename);	
	map<unsigned, unsigned> active;

	out << "# Alfabeto" << endl;	
	out << nfa.GetAlphabetLenght() << endl;
	
	out << "# Numero de estados" << endl;	
	for (unsigned i=0; i<nfa.GetMaxStates(); i++)
	{
		// contar estados activos
		if(nfa.IsActiveState(i))
		{
			active[(unsigned)active.size()] = i;
		}		
	}
	out << active.size() << endl;

	out << "# Estados iniciales" << endl;	
	for (auto i=active.begin(); i!=active.end(); i++)
	{
		if(nfa.IsInitial(i->second)) 
		{
			out << i->first << " ";
		}				
	}	
	out << endl;

	out << "# Estados finales" << endl;	
	for (auto i=active.begin(); i!=active.end(); i++)
	{
		if(nfa.IsFinal(i->second))
		{
			out << i->first << " ";
		}		
	}	
	out << endl;

	out << "# Descripcion de las transiciones" << endl;
	ostringstream buffer;
	int transitions = 0;	
	for (auto i=active.begin(); i!=active.end(); i++)
	{ 
		for (auto k=active.begin(); k!=active.end(); k++)
		{			
			for (Nfa::TSymbol j = 0; j<nfa.GetAlphabetLenght(); j++)
			{
				// estado(st1) con estado(st2) con simbolo(j)
				if(!nfa.ExistTransition(i->second, k->second, j)) continue;				
				buffer << i->first << " " << k->first << " " << j << endl;
				transitions++;
			}			
		}		
	}
	out << transitions << endl;
	out << buffer.str();
	out.close();
}

Nfa NfaDotExporter::ImportDestinoPlainText(std::string filename)
{
	ifstream file(filename);
	if(!file.is_open())
	{
		throw runtime_error("No fue posible abrir el modelo");
	}
	int stateCount;
	int transitionCount;
	int currentTransition;
	string line;
	Nfa ndfa(1);

	// maquina de estados de lectura de archivo
	enum { header_alphabet, header_states, header_initial, header_final, header_transitions_count, body_transitions } state;
	state = header_alphabet;

	while(!file.eof())
	{
		// Leemos la informacion de cabecera
		getline(file, line);
		trim(line);

		// Omitimos lineas vacias o con comentarios
		if(line.size() == 0 || line[0] == '#') continue;

		if(state == header_alphabet)
		{
			vector<string> splits;
			int alpha = lexical_cast<int>(line);
			ndfa = Nfa(alpha);
			state = header_states;
		}
		else if(state == header_states)
		{
			stateCount = lexical_cast<int>(line);
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
			transitionCount = lexical_cast<int>(line);
			currentTransition = 0;
			state = body_transitions;
		}
		else if(state == body_transitions)
		{
			auto splits = _splitBySpaces(line);
			auto src = lexical_cast<int>(splits[0]);
			auto dst = lexical_cast<int>(splits[1]);						
			auto sym = lexical_cast<unsigned>(splits[2]);			
			if(src >= stateCount) 
			{
				throw runtime_error("Numero de estado fuente invalido");
			}
			if(dst >= stateCount) 
			{
				throw runtime_error("Numero de estado destino invalido");
			}
			if(sym >= ndfa.GetAlphabetLenght()) 
			{
				throw runtime_error("Codigo de simbolo invalido");
			}
			if(currentTransition >= transitionCount)
			{
				throw runtime_error("Cantidad de transiciones incorrecta");
			}
			ndfa.SetTransition(src, dst, sym);
			currentTransition++;			
		}
	}
	file.close();
	return ndfa;
}
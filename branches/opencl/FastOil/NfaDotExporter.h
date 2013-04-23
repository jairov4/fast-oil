#pragma once

#include <string>
#include <vector>
#include "Nfa.h"

class NfaDotExporter
{
public:	
	static void Export(const Nfa& nfa, std::string filename);
	static void ExportDestinoPlainText(const Nfa& nfa, std::string filename);
	static Nfa ImportDestinoPlainText(std::string filename);		
};


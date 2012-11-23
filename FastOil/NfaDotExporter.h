#pragma once

#include <string>
#include "Nfa.h"

class NdfaDotExporter
{
public:	
	static void Export(const Nfa& nfa, std::string filename);
	static void ExportDestinoPlainText(const Nfa& nfa, std::string filename);
	static Nfa ImportDestinoPlainText(std::string filename);
};


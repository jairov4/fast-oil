#pragma once

#include <string>
#include "Ndfa.h"

class NdfaDotExporter
{
public:	
	static void Export(const Ndfa& ndfa, std::string filename);
	static void ExportDestinoPlainText(const Ndfa& ndfa, std::string filename);
	static Ndfa ImportDestinoPlainText(std::string filename);
};


/**
	Very simple File Format for sample dataset exchange
*/

#include "StdAfx.h"
#include "SamplesReader.h"

using namespace std;
using boost::lexical_cast;
using boost::trim;

typedef SamplesReader::TSamples TSamples;
typedef SamplesReader::TSymbol TSymbol;
typedef SamplesReader::TSample TSample;

SamplesReader::SamplesReader(void)
{
}


SamplesReader::~SamplesReader(void)
{
}

vector<string> _splitBySpaces(const string& line )
{
	vector<string> splits;
	// divide la cadena por los espacios
	boost::split(splits, line, [](char s){return s==' ';});
	return splits;
}

/// <summary>
/// Lee dos conjuntos de muestras a partir de un archivo de texto
/// </summary>
void SamplesReader::ReadSamples( string filename, TSamples& pos, TSamples& neg, unsigned* alphabetLength )
{
	assert(alphabetLength != NULL);

	ifstream file(filename);
	if(!file.is_open()) 
	{
		throw runtime_error("El archivo de muestras no pudo ser abierto");
	}

	string line;
	enum { header, sample } state;
	state = header;	
	
	while (!file.eof())
	{
		// Leemos la informacion de cabecera
		getline(file, line);
		trim(line);
		if(state == header)
		{
			auto splits = _splitBySpaces(line);
			if(splits.size() != 2)
			{
				throw runtime_error("Formato de archivo de secuencias invalido, cabecera mal formada");
			}			
			// int cnt = lexical_cast<int>(splits[0]); // non-used
			*alphabetLength = lexical_cast<int>(splits[1]);
			state = sample;
		}
		else if(state == sample)
		{
			// ignora lineas vacias
			if(line.empty()) continue;

			// lee los tokens
			auto splits = _splitBySpaces(line);
			if(splits.size() < 2)
			{
				throw runtime_error("Formato de archivo de secuencias invalido, linea incompleta");
			}
			// determina si es muestra positiva
			bool isPositive = lexical_cast<int>(splits[0]) == 1;
			unsigned sampleLenght = lexical_cast<unsigned>(splits[1]);
			if(splits.size() != sampleLenght + 2)
			{
				throw runtime_error("Formato de archivo de secuencias invalido, dimensiones invalidas");
			}

			// Lee la nueva muestra, convierte cada simbolo
			TSample newSample;
			newSample.reserve(sampleLenght);
			for (auto it = splits.cbegin()+2; it!=splits.cend(); ++it)
			{
				auto val = lexical_cast<TSymbol>(*it);
				if(val >= *alphabetLength)
				{
					throw runtime_error("Formato de archivo de secuencias invalido, longitud de alfabeto incorrecta");
				}
				newSample.push_back(val);
			}

			// etiqueta la muestra
			if(isPositive) pos.push_back(newSample);
			else neg.push_back(newSample);
		}
	}
}

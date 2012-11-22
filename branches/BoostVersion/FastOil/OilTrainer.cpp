#include "stdafx.h"
#include "OilTrainer.h"
#include "NdfaDotExporter.h"

using namespace std;
using boost::lexical_cast;

typedef OilTrainer::TSample TSample;
typedef OilTrainer::TSamples TSamples;
typedef OilTrainer::TSymbol TSymbol;

/** Cuenta el numero de muestras de una secuencia que son reconocidas por un automata
*/
int _countMatches(TSamples::const_iterator begin, TSamples::const_iterator end, const Ndfa& ndfa)
{
	int count = 0;
	for (auto it=begin; it!=end; ++it)
	{
		bool match = ndfa.IsMatch(*it);
		if(match) count++;
	}
	return count;
}

/** Cuenta el numero de muestras de una secuencia que son reconocidas por un automata
*/
int _countMatches(const TSamples& samples, const Ndfa& ndfa)
{	
	return _countMatches(samples.cbegin(), samples.cend(), ndfa);
}

/** Indica si alguna muestra es reconocida por el automata
*/
bool _anyMatch(const TSamples& samples, const Ndfa& ndfa)
{	
	for (auto it=samples.cbegin(); it!=samples.cend(); ++it)
	{
		auto match = ndfa.IsMatch(*it);
		if(match) return true;
	}
	return false;
}


/** Indica si todas las muestras son reconocidas por un automata
*/
bool _allMatch(TSamples::const_iterator begin, TSamples::const_iterator end, const Ndfa& ndfa)
{
	for (auto it=begin; it!=end; ++it)
	{
		auto match = ndfa.IsMatch(*it);
		if(!match) return false;
	}
	return true;
}


/** Indica si todas las muestras son reconocidas por un automata
*/
bool _allMatch(const TSamples& samples, const Ndfa& ndfa)
{
	return _allMatch(samples.cbegin(), samples.cend(), ndfa);
}

/** Comparador de muestras usado para ordenar las muestras en forma lexicografica.
    Retorna true cuando la primera muestra es menor que la segunda
*/
bool _sampleComparer(const TSample& sample1, const TSample sample2)
{	
	auto eq = sample1.size() == sample2.size();
	if(eq) return std::lexicographical_compare(sample1.cbegin(), sample1.cend(), sample2.cbegin(), sample2.cend());
	else return sample1.size() < sample2.size();
}

/** Entrena un nuevo modelo de automata no determinista usando las muestras positivas y negativas que se le suministren
	@posSamples Muestras positivas
	@negSamples Muestras negativas
	@len Longitud de cada muestra
	@alpha Longitud del alfabeto
*/
Ndfa* OilTrainer::Train(TSamples& positiveSamples, TSamples& negativeSamples, unsigned alpha )
{
	ndfa = new Ndfa(alpha);
	testNdfa = new Ndfa(alpha);
	bestNdfa = new Ndfa(alpha);
	
	// Asegura orden lexicografico
	sort(positiveSamples.begin(), positiveSamples.end(), _sampleComparer);
	sort(negativeSamples.begin(), negativeSamples.end(), _sampleComparer);

	posSamples = &positiveSamples;
	negSamples = &negativeSamples;
	
	int currentPosSample=0;
	for (auto currentPosSampleIter=posSamples->cbegin(); currentPosSampleIter != posSamples->cend(); ++currentPosSampleIter)
	{		
		auto acceptPos = ndfa->IsMatch(*currentPosSampleIter);
		if(!acceptPos)
		{
			CoreceMatch(currentPosSampleIter);
			DoAllMergesPossible(currentPosSampleIter);
		}
		currentPosSample++;

		if(ShowProgress)
		{
			//NdfaDotExporter::Export(*ndfa, "ndfa" + lexical_cast<string>(currentPosSample) + ".dot");
			cout << "Procesada muestra " << currentPosSample << " de " << posSamples->size() << " (" << (currentPosSample*100L/posSamples->size()) << "%)" << endl;
		}
	}

	delete testNdfa;
	delete bestNdfa;
	
	// Asegura que reconoce todas las muestras positivas
	assert(_allMatch(positiveSamples, *ndfa));
	// Asegura que no reconoce ninguna muestra negativa
	assert(!_anyMatch(negativeSamples, *ndfa));

	return ndfa;
}

/** Agrega estados al automata de tal manera que lo fuerza a reconocer una nueva muestra positiva
*/
void OilTrainer::CoreceMatch(TSamples::const_iterator currentPosSampleIterator)
{
	// muestra positiva actual
	const TSample& currentPosSample = *currentPosSampleIterator;
	
	// apartir de este momento los nuevos estados empezaran ubicarse desde este indice
	// en el vector de identificadores aleatorios
	statesAddedBeginInRandom = (unsigned)randomIds.size();	
	
	// ultimo estado añadido
	int lastStateId = -1;

	auto inactive = ndfa->GetActiveStates();
	// negar los activos nos indica cuales son los inactivos
	inactive.Not();
		
	// indica en que simbolo de la muestra vamos
	auto symIter = currentPosSample.cbegin();

	// inserta estados en los espacios inactivos
	for(auto inactiveStateIter = inactive.GetBitSetIterator(); symIter != currentPosSample.cend(); inactiveStateIter.Next())
	{
		auto inactiveStateId = inactiveStateIter.GetBit();
		if(lastStateId == -1) // indica que es el primer estado
		{
			// El primer estado es inicial
			ndfa->SetInitial(inactiveStateId);
		} else {			
			ndfa->SetTransition(lastStateId, inactiveStateId, *symIter);			
			// avanza al siguiente simbolo
			symIter++;
		}
		lastStateId = inactiveStateId;
		randomIds.push_back(lastStateId);				
	}
	ndfa->SetFinal(lastStateId);

	// Aseguramos que reconocemos la nueva muestra
	assert(ndfa->IsMatch(currentPosSample));	
}

/** Realiza todas las mezclas de estados posibles sobre el automata
	@sampleBegin Inicio de muestra positiva actual
	@sampleEnd Fin de muestra positiva actual
*/
void OilTrainer::DoAllMergesPossible(TSamples::const_iterator currentPosSampleIterator)
{
	auto nextPosSampleIterator = currentPosSampleIterator + 1;
	vector<int>::iterator it = randomIds.begin() + statesAddedBeginInRandom;
	if(!DoNotUseRandomSort)	random_shuffle(it, randomIds.end()); // revuelve los nuevos elementos añadidos
	unsigned totalLenght = (unsigned)randomIds.size();
	int mergeCounter = 0;

	// nuevos estados en orden aleatorio
	for (unsigned i=statesAddedBeginInRandom; i<totalLenght; /* ver final del ciclo para ver como avanza */)
	{		
		int bestScore = -1;
		int bestJ = -1;

		int s1 = randomIds[i];
		// viejos y nuevos estados en orden aleatorio
		// ojo con la condicion de parada: sin repetir
		for (unsigned j=0; j<i; j++)
		{
			int s2 = randomIds[j];
			*testNdfa = *ndfa; // copiamos en el de prueba
			testNdfa->Merge(s2, s1); // hacemos la mezcla

			bool anyNegMatch = _anyMatch(*negSamples, *testNdfa);
			if(anyNegMatch) continue;
			
			// cuenta las que reconozca en adelante porque las anteriores y la actual es fijo que debe reconocerlas
			int score = _countMatches(nextPosSampleIterator, posSamples->cend(), *testNdfa);
			if(score > bestScore)
			{
				bestScore = score;
				bestJ = j;
				swap(bestNdfa, testNdfa);
				// acaba con la busqueda de estados que se puedan combinar					
				if(SkipSearchBestMerge) break;
			}
			if(ShowPossibleMerges) 
			{
				// Para mostrar las combinaciones evaluadas (y ver si estan completas)
				cout << "Posible Mezcla " << j << " " << i << " -> " << s2 << " " << s1 << " (score: " << score << ")" << endl;
			}
		}

		// si se encontro mezcla exitosa
		// eliminamos el eliminado (el indice I)
		if(bestScore != -1) 
		{
			mergeCounter++;
			if(ShowMerges)
			{
				cout << "Mezcla "<< bestJ << " " << i << " -> " << randomIds[bestJ] << " " << s1 << " (score: " << bestScore << ")" << endl;
				//NdfaDotExporter::Export(*bestNdfa, "ndfa"+lexical_cast<string>(currentPosSampleIterator-posSamples->cbegin())+"-"+lexical_cast<string>(mergeCounter)+".dot");
			}
			// intercambia los automatas de prueba y final
			swap(ndfa, bestNdfa);
						
			if(DoNotUseRandomSort) 
			{
				// aunque este modo podria ser optimizado eliminando el indicador en una lista enlazada
				// se prefiere el mecanismo usando vector porque se presume que mantiene mayor localidad
				// en cache para el escenario de orden aleatorio (el mas usado)
				// TODO: Comprobar si ambos casos son mejorados usando lista enlazada
				move(randomIds.cbegin()+1+i, randomIds.cend(), randomIds.begin()+i);
			} 
			else 
			{
				// guarda el ultimo en el lugar donde estaba el estado
				// que fue eliminado, asi podemos descartar y reducir el
				// tamaño del vector sin penalizar el desempeño
				randomIds[i] = randomIds.back();				
			}
			randomIds.pop_back();
			totalLenght--;			
		}
		else
		{
			i++;
		}
	}
	
	// no debe quedar reconociendo muestras negativas
	assert(!_anyMatch(*negSamples, *ndfa));
	// no debe perderse la capacidad de reconocer la nueva muestra ni las anteriores
	assert(_allMatch(posSamples->cbegin(), nextPosSampleIterator, *ndfa));
}

OilTrainer::OilTrainer()
	: ShowMerges(false), ShowProgress(false), SkipSearchBestMerge(false), DoNotUseRandomSort(false), ShowPossibleMerges(false)
{
}

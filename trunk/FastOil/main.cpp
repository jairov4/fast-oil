#include "stdafx.h"
#include "Nfa.h"
#include "SamplesReader.h"
#include "OilTrainer.h"
#include "NfaDotExporter.h"
#include "Testing.h"

using namespace std;
using boost::starts_with;
using boost::lexical_cast;

// Entrena un solo modelo
void TrainSingle(string samplesFilename, string modelFilename, bool showProgress, bool showMerges, bool skipSearch, bool noRandom)
{
	cout << "Cargando muestras" << endl;
	SamplesReader reader;
	SamplesReader::TSamples pos, neg;
	unsigned alpha;
	reader.ReadSamples(samplesFilename, pos, neg, &alpha);

	cout << "Entrenando modelo" << endl;
	OilTrainer trainer;
	trainer.ShowProgress = showProgress;
	trainer.ShowMerges = showMerges;
	trainer.SkipSearchBestMerge = skipSearch;
	trainer.DoNotUseRandomSort = noRandom;
	trainer.ShowPossibleMerges = showMerges;
	auto ndfa = trainer.Train(pos, neg, alpha);

	cout << "Exportando modelo" << endl;
	NfaDotExporter::Export(*ndfa, modelFilename+".dot");
	NfaDotExporter::ExportDestinoPlainText(*ndfa, modelFilename);
	delete ndfa;
}

// Entrena un conjunto de modelos
void TrainMultiple(string samplesFilename, string modelsManifestFilename, int count, bool showProgress, bool showMerges, bool skipSearch, bool noRandom, int customSeed)
{
	ofstream manifest(modelsManifestFilename);
	if(!manifest.is_open())
	{
		throw exception("No fue posible abrir el archivo de manifiesto");
	}
	manifest << "# Manifiesto de clasificador" << endl;
	manifest << "# Los siguientes archivos de modelos referenciados" << endl;
	string modelFilename;	
	for(int i=0; i<count; i++)
	{
		modelFilename = string("automata-") + lexical_cast<string>(i) + ".auto";
		TrainSingle(samplesFilename, modelFilename, showProgress, showMerges, skipSearch, noRandom);
		manifest << modelFilename << endl;
		cout << "Progreso global: modelo " << i << " (" << ((i+1)*100/count) << "%)" << endl;
	}
	manifest.close();
}

// Prueba una muestra con el clasificador NDFA
int TestSample(ofstream& report, size_t n, const Nfa& model, const SamplesReader::TSample& sample)
{
	auto c = model.IsMatch(sample);		
	report << "Evaluation # " << n << " class: " << c << endl;
	return c == true ? 1 : 0;
}

// Escribe los resultados de un experimento
void ReportMetric(ostream& report, int tp, int tn, int totalP, int totalN)
{
	auto fp = totalP - tp;
	auto fn = totalN - tn;
	auto acc = (tp + tn)/(float)(totalP + totalN);
	auto sens = tp/(float)totalP;
	auto spec = tn/(float)totalN;
	auto mcc = (tp*tn - fp*fn)/sqrt((float)(tp+fp)*(tp+fn)*(tn+fp)*(tn+fn));

	// informa resultado
	report << "True Positives: " << tp << ", True Negatives: " << tn << endl;	
	report << "Total Samples: " << (totalP + totalN) << ", Total P-samples: " << totalP << ", Total N-samples: " << totalN << endl;
	report << "Accuracy: " << acc << ", Sensitivity: " << sens << ", Specificity: " << spec << ", MCC: " << mcc << endl;
}

// Evalua un modelo en un conjunto de muestras
void TestSingle(string samplesFilename, string modelFilename, string reportFilename)
{	
	SamplesReader::TSamples pos, neg;
	cout << "Cargando modelo." << endl;
	auto model = NfaDotExporter::ImportDestinoPlainText(modelFilename);

	cout << "Cargando archivo de muestras." << endl;
	SamplesReader reader;
	
	unsigned alpha;
	reader.ReadSamples(samplesFilename, pos, neg, &alpha);
		
	ofstream report(reportFilename);
	if(!report.is_open())
	{
		throw std::exception("Error abriendo el archivo de reporte");
		return;
	}

	cout << "Evaluando..." << endl;

	int pc = 0, nc = 0;
	report << "Muestras Positivas" << endl;
	for(size_t i = 0; i < pos.size(); i++)
	{
		auto r = TestSample(report, i, model, pos[i]);		
		if(r == 1) pc++;
	}
	report << "Muestras Negativas" << endl;
	for(size_t i=0; i<neg.size(); i++)
	{
		auto r = TestSample(report, i, model, neg[i]);		
		if(r == 0) nc++;
	}

	ReportMetric(cout, pc, nc, (int)pos.size(), (int)neg.size());
	ReportMetric(report, pc, nc, (int)pos.size(), (int)neg.size());

	report.close();
}

// Lee los modelos indicados en el manifiesto
void ReadManifest(string manifestFilename, vector<string>& models)
{
	ifstream manifest(manifestFilename);
	if(!manifest.is_open())
	{
		throw std::exception("Error with manifest file");
		return;
	}
	string line;
	while(!manifest.eof())
	{
		getline(manifest, line);
		boost::trim(line);
		if(line.size() == 0 || line[0] == '#') continue;
		models.push_back(line);
	}
	manifest.close();
}


// Evalua un conjunto de modelos sobre un conjunto de muestras
void TestMultiple(string samplesFilename, string modelsManifestFilename, string reportFilename)
{
	SamplesReader::TSamples pos, neg;
	vector<Nfa> models;
		
	// Carga modelos del clasificador
	{
		cout << "Cargando manifiesto." << endl;
		vector<string> modelFiles;
		ReadManifest(modelsManifestFilename, modelFiles);
		for(auto i = modelFiles.begin(); i!=modelFiles.end(); i++)
		{			
			auto model = NfaDotExporter::ImportDestinoPlainText(*i);
			models.push_back(model);
			cout << "Modelo \"" << *i << "\" cargado." << endl;
		}
		cout << "Cargados " << models.size() << " modelos" << endl;
	}
	
	cout << "Cargando archivo de muestras." << endl;
	SamplesReader reader;
	
	unsigned alpha;
	reader.ReadSamples(samplesFilename, pos, neg, &alpha);

	ofstream report(reportFilename);
	if(!report.is_open())
	{
		throw std::exception("Error abriendo el archivo de reporte");		
	}
	
	cout << "Evaluando..." << endl;
	
	// el umbral se fija en la mitad entera del numero de modelos	
	auto threshold = models.size() / 2;
	int pc = 0, nc = 0;
	report << "Muestras Positivas" << endl;
	for(size_t i = 0; i < pos.size(); i++)
	{
		int answerCounter = 0;
		for(auto j=models.begin(); j != models.end(); j++)
		{
			auto rj = TestSample(report, i, *j, pos[i]);
			// cuenta los modelos que votan por "positiva"
			if(rj == 1) answerCounter++; 
		}		
		// si mas de la mitad de los votos son por "positiva"
		if(answerCounter > threshold) pc++; 
	}
	report << "Muestras Negativas" << endl;
	for(size_t i=0; i<neg.size(); i++)
	{
		int answerCounter = 0;
		for(auto j=models.begin(); j != models.end(); j++)
		{
			auto r = TestSample(report, i, *j, neg[i]);		
			// cuenta los modelos que votan por "negativa"
			if(r == 0) answerCounter++;
		}
		// si mas de la mitad de los votos son por "negativa"
		if(answerCounter > threshold) nc++;
	}

	// informa resultado
	ReportMetric(cout, pc, nc, (int)pos.size(), (int)neg.size());
	ReportMetric(report, pc, nc, (int)pos.size(), (int)neg.size());

	report.close();
}

// Procesa los argumentos para obtener la configuracion
void ParseTrainOptions(vector<string>::const_iterator optBegin, vector<string>::const_iterator optEnd, bool* showProgress, bool* showMerges, bool* skipSearch, bool* noRandom, int* customSeed)
{
	assert(showProgress != NULL);
	assert(showMerges != NULL);
	assert(skipSearch != NULL);
	assert(noRandom != NULL);
	assert(customSeed != NULL);

	*showProgress = true;
	*showMerges = false;
	*skipSearch = false;
	*noRandom = false;
	*customSeed = -1;

	for_each(optBegin, optEnd, [skipSearch, noRandom, showMerges, customSeed](string opt) 
	{
		if(opt == "--skip-search")
		{
			*skipSearch = true;		
			cout << "Omitir la busqueda" << endl;
		}
		else if(opt == "--no-random")
		{
			*noRandom = true;
			cout << "No realizar mezcla en orden aleatorio" << endl;
		} 
		else if(opt == "-v")
		{
			*showMerges = true;
			cout << "Mostrar mezclas" << endl;
		}
		else if(boost::starts_with(opt, "--seed="))
		{
			*customSeed = lexical_cast<int>(opt.substr(7));
			cout << "Semilla personalizada: " << (*customSeed) << endl;
		}
	});
}

int main(int argc, char* argv[])
{
	Testing::AllTesting(); return 0;
	try
	{
		if(argc < 2)
		{
			cout << "Para obtener ayuda ejecute \"FastOil help\"" << endl;
			return 1;
		}

		vector<string> arguments;
		for(int i=1; i<argc; i++)
		{
			arguments.push_back(argv[i]);
		}

		bool trainSingle = arguments[0] == "train_single";
		bool trainMultiple = arguments[0] == "train_multiple";
		bool testSingle = arguments[0] == "test_single";
		bool testMultiple = arguments[0] == "test_multiple";
		bool help = arguments[0] == "help";

		if(help)
		{
			cout
				<< "Construye modelos por el algoritmo Order Independent Language (OIL)" << endl
				<< "\tFastOIL {help|train_single|train_multiple|test_single|test_multiple} <options>" << endl
				<< "Options:" << endl
				<< endl
				<< "help" <<endl
				<< "\tMuestra este mensaje de ayuda" << endl
				<< endl
				<< "train_single <samples> <model> [--skip-search] [--no-random] [--seed=N] [-v]" << endl
				<< "\tEntrena a partir de las muestras del archivo <samples> un solo" << endl
				<< "\tmodelo y lo guarda en el archivo <model>" << endl
				<< endl
				<< "train_multiple <samples> <models-manifest> <count> [--skip-search] [--no-random] [--seed=N] [-v]" << endl
				<< "\tEntrena a partir de las muestras del archivo <samples> varios modelos" << endl
				<< "\tque pueden ser utilizados como un comite de expertos" << endl
				<< endl
				<< "test_single <samples> <model> <report>" << endl
				<< "\tEvalua el modelo desde el archivo <model> en el conjunto de" << endl
				<< "\tmuestras <samples>" << endl
				<< endl
				<< "test_multiple <samples> <models-manifest> <report>" << endl
				<< "\tEvalua multiples modelos indicados en el archivo de manifiesto" << endl
				<< "\t<models-manifest> con las muestras en el archivo <samples>." << endl
				<< "\tEscribe los resultados en el archivo <report>" << endl
				<< endl
				<< ">> Shared options:" << endl
				<< "\tLa opcion --skip-search hace que el algoritmo omita la busqueda" << endl
				<< "\tlocal explicita de la mejor opcion para la mezcla de estados" << endl
				<< "\tpermitiendo una ejecucion mas rapida y una recombinacion menos" << endl
				<< "\tdeterminista de los estados disponibles." << endl
				<< endl
				<< "\tLa opcion --no-random permite realizar la mezcla de estados en" << endl
				<< "\ten modo determinista" << endl
				<< endl
				<< "\tLa opcion --seed=N le permite establecer N como la semilla de" << endl
				<< "\tgeneracion de numeros aleatorios. De esta manera puede generar" << endl
				<< "\tmodelos con mezcla de estados en orden aleatorio y conservar" << endl
				<< "\tdeterminismo de los resultados de experimentacion" << endl
				<< endl
				<< "\tLa opcion -v muestra la mezcla de estados realizada" << endl
				<< endl
				;
		} 
		else if(trainSingle || trainMultiple)
		{
			if(argc < 4) 
			{
				cout << "Numero de argumentos incorrecto" << endl;
				return 1;
			}
			string samplesFilename = arguments[1];
			string modelFilename = arguments[2];
			bool showProgress, showMerges, skipSearch, noRandom;
			int customSeed;
			ParseTrainOptions(arguments.begin()+3, arguments.end(), &showProgress, &showMerges, &skipSearch, &noRandom, &customSeed);
			auto t = customSeed == -1 ? time(NULL) : customSeed;	
			srand((unsigned)t);
			if(trainSingle) 
			{
				cout << "Entrenar modelo" << endl;				
				TrainSingle(samplesFilename, modelFilename, showProgress, showMerges, skipSearch, noRandom);
			}
			if (trainMultiple) 
			{
				cout << "Entrenar multiples modelos" << endl;
				int count = lexical_cast<int>(arguments[3]);
				TrainMultiple(samplesFilename, modelFilename, count, showProgress, showMerges, skipSearch, noRandom, customSeed);
			}
		} 
		else if(testSingle || testMultiple)
		{
			if(argc < 5)
			{
				cout << "Numero de argumentos incorrecto" << endl;
				return 1;
			}
			string samplesFilename = arguments[1];
			string modelFilename = arguments[2];
			string reportFilename = arguments[3];
			if(testSingle) TestSingle(samplesFilename, modelFilename, reportFilename);
			if(testMultiple) TestMultiple(samplesFilename, modelFilename, reportFilename);
		} 
		else 
		{
			cout << "Opcion invalida: '" << arguments[1] << "'" << endl;
			return 1;
		}
	} 
	catch (const exception& e) 
	{
		string msg = e.what();
		cout << "Unexpected error: " << msg << endl;
		throw e;
	}
	return 0;
}

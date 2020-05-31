// FeatureGenerator.cpp : Defines the entry point for the console application.
//

#include <tchar.h>
#include <iostream>
#include <cassert>
#include <fstream>
#include <map>
#include <ctime>

#include "GeneratorReg.h"
#include "FeatureGeneratorFactory.h"

#include "FeaGenConfigParser.h"
#include "QDJUFile.h"
#include "RankingStuff.h"
#include "ContentReader.h"
#include "DocStructure.h"
#include "StringUtil.h"

using namespace std;
using namespace arc;

#define	SMALL_UNIT	500
#define	LARGER_UNIT	5000
#define	LINE_UNIT	20000

#define COLLECTION_SIZE	1000000000
#define AVDL_BODY		1447
#define AVDL_TITLE		8
#define AVDL_URL		8
#define AVDL_ANCHOR		6880

//-------------------------------------------------------------------------------
// Finalizers
//-------------------------------------------------------------------------------
void Finalize(IFeatureGenerator**& rgFeatureGenerator, int iGeneratorCount)
{
	// Free the spaces
	for (int i = 0; i < iGeneratorCount; i++)
	{
		if (rgFeatureGenerator[i] != NULL)
		{
			delete rgFeatureGenerator[i];
			rgFeatureGenerator[i] = NULL;
		}
	}
	delete[] rgFeatureGenerator;
	rgFeatureGenerator = NULL;
}

void Finalize(HANDLE& hContent, IFeatureGenerator**& rgFeatureGenerator, int iGeneratorCount)
{
	// Close content
	if (hContent != NULL)
	{
		CloseContent(hContent);
		hContent = NULL;
	}
	Finalize(rgFeatureGenerator, iGeneratorCount);
}

//--------------------------------------------------------------------------------------
// Some sub functions
//--------------------------------------------------------------------------------------
bool CreateFeatureGenerators(
	IFeatureGenerator**& rgFeatureGenerator,
	const FeaGenConfig& feaGenConfig,
	string& strErrorInfo)
{
	strErrorInfo.clear();
	rgFeatureGenerator = new IFeatureGenerator*[feaGenConfig.FeatureGeneratorCount];
	for (int i = 0; i < feaGenConfig.FeatureGeneratorCount; i++)
	{
		rgFeatureGenerator[i] = NULL;
	}
	for (int i = 0; i < feaGenConfig.FeatureGeneratorCount; i++)
	{
		GeneratorInfo generatorInfo = feaGenConfig.FeatureGeneratorInfos[i];

		
		size_t nPos;
		string strSplitFeatureName;
		nPos=generatorInfo.GeneratorName.find("__");
		strSplitFeatureName= generatorInfo.GeneratorName.substr(0,nPos);
		IFeatureGenerator* pGenerator = CFeatureGeneratorFactory::CreateFeatureGenerator(
			strSplitFeatureName);
		

//		IFeatureGenerator* pGenerator = CFeatureGeneratorFactory::CreateFeatureGenerator(
//			generatorInfo.GeneratorName);

		if (pGenerator == NULL)
		{
			strErrorInfo = "Generator named " + generatorInfo.GeneratorName
				+ " doesn't exist!";
			return false;
		}
		if (!pGenerator->Initialize(
				feaGenConfig.FeatureGeneratorInfos[i].ExternalFileNames))
		{
			strErrorInfo = "Intialize generator named " + generatorInfo.GeneratorName
				+ " failed!";
			return false;
		}
		rgFeatureGenerator[i] = pGenerator;
	}

	return true;
}

// Prepare msn content files and msn feature file if need
bool PrepareSourceFiles(
	HANDLE& hContent, 
	CMSNFeatureFile& msnFeatureFile,
	const FeaGenConfig& feaGenConfig,
	string& strErrorInfo)
{
	// If need source document, initialize the handle
	HRESULT hr;
	hContent = NULL;
	if (feaGenConfig.NeedMSNContentFile)
	{
		hr = OpenContentPath(
			feaGenConfig.MSNContentFileDir.c_str(),
			hContent);
		if (!SUCCEEDED(hr))
		{
			strErrorInfo = "Open content dir error!";
			return false;
		}
	}

	// If need msn feature files
	if (feaGenConfig.NeedMSNFeatureFile)
	{
		if (!msnFeatureFile.Init(feaGenConfig.MSNFeatureFileName))
		{
			strErrorInfo = "Open msn feature files error!";
			return false;
		}
		if (!msnFeatureFile.ReadHeader())
		{
			strErrorInfo = "Read msn feature file header error!";
			return false;
		}
		if (msnFeatureFile.FeatureNames.size() <= 0)
		{
			strErrorInfo = "Read msn feature file header error!";
			return false;
		}
	}

	return true;
}

// Dump features
bool DumpFeatures(
    const string& strDstDir,
	map<string, ofstream*>& mapFiles,
	map<string, ofstream*>& mapDespFiles,
	const string& strGeneratorName,
	int iFeatureDim,
	const vector<float>& vecParas,
	const vector<float>& vecFeatureValues,
	string& strErrorInfo,
	bool fDespDumped,
	int iTotalParaCount,
	bool fReachTheLastPara)
{
	for (int iFeatureIdx = 0; iFeatureIdx < (int)vecFeatureValues.size(); iFeatureIdx++)
	{
		// Generate current tag
		string strTag = strGeneratorName + "_" + StringUtil::I2Str(iFeatureIdx);
		float flFeatureValue = vecFeatureValues.at(iFeatureIdx);

		// Dump description if necessary
		if (!fDespDumped)
		{
			string strDespFileName = strDstDir + "\\" + strTag + ".desp.txt";
			map<string, ofstream*>::iterator itDespFile = mapDespFiles.find(strTag);
			string strFeatureName = strTag;
			for (int iParaIdx = 0; iParaIdx < (int)vecParas.size(); iParaIdx++)
			{
				strFeatureName += ("_" + StringUtil::F2Str(vecParas.at(iParaIdx)));
			}
			if (itDespFile != mapDespFiles.end())
			{
				(*(*itDespFile).second) << strFeatureName << endl;
			}
			else
			{
				ofstream* pDespFile = new ofstream();
				pDespFile->open(strDespFileName.c_str());
				if (!pDespFile->good())
				{
					strErrorInfo = "Open file " + strDespFileName + " error!";
					return false;
				}
				(*pDespFile) << strFeatureName << endl;
				mapDespFiles[strTag] = pDespFile;
			}
		}

		// Dump feature value
		string strDumpFileName = strDstDir + "\\" + strTag + ".txt";
		map<string, ofstream*>::iterator itFile = mapFiles.find(strTag);
		if (itFile != mapFiles.end())
		{
			(*(*itFile).second) << flFeatureValue;
			if (fReachTheLastPara)
			{
				(*(*itFile).second) << endl;
			}
			else
			{
				(*(*itFile).second) << " ";
			}
		}
		else
		{
			ofstream* pDumpFile = new ofstream();
			pDumpFile->open(strDumpFileName.c_str());
			if (!pDumpFile->good())
			{
				strErrorInfo = "Open file " + strDumpFileName + " error!";
				return false;
			}
			(*pDumpFile) << iFeatureDim << " " << iTotalParaCount << endl
				<< flFeatureValue;
			if (fReachTheLastPara)
			{
				*pDumpFile << endl;
			}
			else
			{
				*pDumpFile << " ";
			}
			mapFiles[strTag] = pDumpFile;
		}
	}
	return true;
}

// Close files
void CloseFiles(map<string, ofstream*>& mapFiles, map<string, ofstream*>& mapDespFiles)
{
	for (map<string, ofstream*>::iterator itFile = mapFiles.begin(); 
		itFile != mapFiles.end(); itFile++)
	{
		(*itFile).second->close();
		delete (*itFile).second;
	}

	for (map<string, ofstream*>::iterator itFile = mapDespFiles.begin(); 
		itFile != mapDespFiles.end(); itFile++)
	{
		(*itFile).second->close();
		delete (*itFile).second;
	}

}


// Generate and write back the features
bool GenerateFeatures(
	IFeatureGenerator* const* rgFeatureGenerator,
	const HANDLE& hContent, 
	CMSNFeatureFile& msnFeatureFile,
	const FeaGenConfig& feaGenConfig,
	string& strErrorInfo)
{
	// Get the feature dimension
	int iFeaDim = 0;
	if (!QDJUFile::GetQDJUFileLineCount(feaGenConfig.IndexFileName, iFeaDim))
	{
		printf("%s\n", feaGenConfig.IndexFileName.c_str());
		strErrorInfo = "Query-doc index file error!";
		return false;
	}

	// Initialize the query-doc index file
	QDJUFile indexFile;
	if (!indexFile.Init(feaGenConfig.IndexFileName))
	{
		strErrorInfo = "Query-doc index file error!";
		return false;
	}
	
	map<string, ofstream*> mapFiles; // The dump files
	map<string, ofstream*> mapDespFiles;
	bool fDespDumped = false;
	int iPrevQueryID = -1;
	int iCurQueryID;
	string strCurQueryID;
	int iOffsetInQuery = -1;
	string strUrl;
	string strStuffFileName;
	SimQueryRankingStuff queryStuff;
	bool fFirstQuery = true; // We clear the query stuff for non-first query
	SOURCE_DOCUMENT sourceDoc;
	vector<int> vecMsnFeature;
	vector<float> vecFeatureValues;
	HRESULT hr;
	int iHandledLineCount = 0;
	while (indexFile.ReadLine())
	{
		iCurQueryID = indexFile.GetQueryId();
		strUrl = indexFile.GetUrl();
		
		// Initialize the feature generators with query info when meets a new query
		if (iPrevQueryID != iCurQueryID)
		{
			if (feaGenConfig.NeedStuffFile)
			{
				strCurQueryID = StringUtil::I2Str(iCurQueryID);
				strStuffFileName = feaGenConfig.StuffFileDir + "\\" 
					+ strCurQueryID + ".bin";
				if (!fFirstQuery)
				{
					queryStuff.Clear(); // Clear the content for the previous query
				}
				if (!queryStuff.LoadBinary(strStuffFileName))
				{
					CloseFiles(mapFiles, mapDespFiles);
					strErrorInfo = "Reading stuff file for query " 
						+ strCurQueryID + " error!";
					return false;
				}
				if (fFirstQuery)
				{
					fFirstQuery = false;
				}
				// TODO (Guomao): Get correct information from stuff
				// Now the N and Avdl information from the stuff is empty
				// We assign them fixed values
				queryStuff.m_collection.m_nN = COLLECTION_SIZE;
				queryStuff.m_collection.m_vecAVFLs[FLD_BODY]   = AVDL_BODY;
				queryStuff.m_collection.m_vecAVFLs[FLD_TITLE]  = AVDL_TITLE;
				queryStuff.m_collection.m_vecAVFLs[FLD_URL]    = AVDL_URL;
				queryStuff.m_collection.m_vecAVFLs[FLD_ANCHOR] = AVDL_ANCHOR;
			}
			// Intialize the feature generators
			for (int i = 0; i < feaGenConfig.FeatureGeneratorCount; i++)
			{
				rgFeatureGenerator[i]->InitializePerQuery(
					feaGenConfig.NeedStuffFile ? &queryStuff : NULL);
			}
			iPrevQueryID = iCurQueryID;
			iOffsetInQuery = 0;
		}

		// Get the source document
		if (feaGenConfig.NeedMSNContentFile)
		{	
			hr = GetDocByURL(hContent, strUrl.c_str(), &sourceDoc);
			if (!SUCCEEDED(hr))
			{
				CloseFiles(mapFiles, mapDespFiles);
				strErrorInfo = "Open document " + strUrl + " error!";
				return false;
			}
		}

		// Get the msn feature
		if (feaGenConfig.NeedMSNFeatureFile)
		{
			if (!msnFeatureFile.ReadFeatureLine()
				|| msnFeatureFile.GetQueryId() != iCurQueryID)
			{
				CloseFiles(mapFiles, mapDespFiles);
				strErrorInfo = "Reading " + strUrl + " from msn feature file error!";
				return false;
			}
		}

		// Generate feature based on doc stuff
		SimDoc docStuff;
		if (feaGenConfig.NeedStuffFile)
		{
			docStuff = (*queryStuff.m_vecDocs.at(iOffsetInQuery));
		}

		// Dump features
		for (int i = 0; i < feaGenConfig.FeatureGeneratorCount; i++)
		{
			//string strGeneratorName = feaGenConfig.FeatureGeneratorInfos[i].GeneratorName;
			string strGeneratorName = feaGenConfig.FeatureGeneratorInfos[i].GeneratorName;
			int iParaGroupCount =
				(int)feaGenConfig.FeatureGeneratorInfos[i].ParameterGroup.size();
			rgFeatureGenerator[i]->SetExtraData(iHandledLineCount);
			if (iParaGroupCount == 0)
			{
				// No parameters
				vector<float> vecParas;
				// Clear the feature value vector
				vecFeatureValues.clear();
				int iFeatureCount = rgFeatureGenerator[i]->GenerateFeature(
					strUrl,
					feaGenConfig.NeedMSNContentFile ? &sourceDoc : NULL,
					feaGenConfig.NeedStuffFile ? &docStuff : NULL,
					feaGenConfig.NeedMSNFeatureFile ? &msnFeatureFile : NULL,
					vecParas,
					vecFeatureValues);
				if (iFeatureCount == -1)
				{
					CloseFiles(mapFiles, mapDespFiles);
					strErrorInfo = "Can't generate feature " 
						+ feaGenConfig.FeatureGeneratorInfos[i].GeneratorName
						+ " for " + strUrl;
					return false;
				}
				assert(iFeatureCount == (int)vecFeatureValues.size());
				if (!DumpFeatures(
					feaGenConfig.DstDataHome,
					mapFiles,
					mapDespFiles,
					strGeneratorName, 
					iFeaDim,
					vecParas, 
					vecFeatureValues, 
					strErrorInfo,
					fDespDumped,
					iParaGroupCount,
					true))
				{
					CloseFiles(mapFiles, mapDespFiles);
					return false;
				}
			}
			else
			{
				// Has parameters
				for (int iParaGroupIdx = 0; iParaGroupIdx < iParaGroupCount;
					iParaGroupIdx++)
				{
					vector<float> vecParas = 
						feaGenConfig.FeatureGeneratorInfos[i].ParameterGroup.at(iParaGroupIdx);
					// Clear the feature value vector
					vecFeatureValues.clear();
					int iFeatureCount = rgFeatureGenerator[i]->GenerateFeature(
						strUrl,
						feaGenConfig.NeedMSNContentFile ? &sourceDoc : NULL,
						feaGenConfig.NeedStuffFile ? &docStuff : NULL,
						feaGenConfig.NeedMSNFeatureFile ? &msnFeatureFile : NULL,
						vecParas,
						vecFeatureValues);
					if (iFeatureCount == -1)
					{
						CloseFiles(mapFiles, mapDespFiles);
						strErrorInfo = "Can't generate feature " 
							+ feaGenConfig.FeatureGeneratorInfos[i].GeneratorName
							+ " for " + strUrl;
						return false;
					}
					assert(iFeatureCount == (int)vecFeatureValues.size());
					if (!DumpFeatures(
						feaGenConfig.DstDataHome,
						mapFiles,
						mapDespFiles,
						strGeneratorName, 
						iFeaDim,
						vecParas, 
						vecFeatureValues, 
						strErrorInfo,
						fDespDumped,
						iParaGroupCount,
						iParaGroupIdx == (iParaGroupCount - 1)))
					{
						CloseFiles(mapFiles, mapDespFiles);
						return false;
					}
				}// for
			}
		}
		// Till now, we have dumped the description of each feature file
		if (!fDespDumped)
		{
			fDespDumped = true;
		}
		iOffsetInQuery++;

		iHandledLineCount++;
		if (iHandledLineCount % LARGER_UNIT == 0)
		{
			cout << "+";
		}
		else if (iHandledLineCount % SMALL_UNIT == 0)
		{
			cout << ".";
		}
		if (iHandledLineCount % LINE_UNIT == 0)
		{
			cout << " (" << iHandledLineCount << "/" 
				<< iFeaDim << ")" << endl;
		}
	}
	
	assert(iHandledLineCount == iFeaDim);
	cout << endl;
	cout << iHandledLineCount << " lines finished." << endl;
	
	// Dump feature file list
	ofstream featureFileListFile(feaGenConfig.FeatureFileListFileName.c_str());
	for (map<string, ofstream*>::const_iterator itFeatureFile = mapFiles.begin();
		itFeatureFile != mapFiles.end(); itFeatureFile++)
	{
		string strTag = (*itFeatureFile).first;
		string strFeatureFileName = feaGenConfig.DstDataHome + "\\" + strTag + ".txt";
		featureFileListFile << strFeatureFileName << endl;
	}
	featureFileListFile.close();

	CloseFiles(mapFiles, mapDespFiles);

	return true;
}


//--------------------------------------------------------------------------------------
// main
//--------------------------------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
	if (argc != 2)
	{
		wcout << argv[0] << L" <config_file>" << endl;
		return -1;
	}
	
	time_t startTime;
	time_t endTime;

	time(&startTime);
	cout << "[1] Checking feature generators ..." << endl;
	// Check feature generators (if more than one generator names are same)
	string strErrorInfo;
	if (!CFeatureGeneratorFactory::CheckFeatureGenerators(strErrorInfo))
	{
		cout << strErrorInfo << endl;
		return -1;
	}

	// Parse config file
	cout << "[2] Parsing config file ..." << endl;
	wstring wstrConfigFileName = argv[1];
	CFeaGenConfigParser feaGenConfigParser;
	FeaGenConfig feaGenConfig;
	if (!feaGenConfigParser.ParseConfigFile(wstrConfigFileName, feaGenConfig))
	{
		wcout << L"config file error!" << endl;
		return -1;
	}
	cout << feaGenConfig.FeatureGeneratorCount << " generators." << endl;
	
	// Create the generators
	cout << "[3] Creating generators ..." << endl;
	IFeatureGenerator** rgFeatureGenerator = NULL;
	if (!CreateFeatureGenerators(rgFeatureGenerator, feaGenConfig, strErrorInfo))
	{
		Finalize(rgFeatureGenerator, feaGenConfig.FeatureGeneratorCount);
		cout << strErrorInfo << endl;
		return -1;
	}
	
	// Prepare MSN content and feature files if needed
	cout << "[4] Preparing msn content file and feature file if needed ..." << endl;
	HANDLE hContent = NULL;
	CMSNFeatureFile msnFeatureFile;
	if (!PrepareSourceFiles(hContent, msnFeatureFile, feaGenConfig, strErrorInfo))
	{
		Finalize(hContent, rgFeatureGenerator, feaGenConfig.FeatureGeneratorCount);
		cout << strErrorInfo << endl;
		return -1;
	}

	// Generate features
	cout << "[5] Generating and dumping features ..." << endl;
	if (!GenerateFeatures(
			rgFeatureGenerator, 
			hContent, 
			msnFeatureFile, 
			feaGenConfig, 
			strErrorInfo))
	{
		Finalize(hContent, rgFeatureGenerator, feaGenConfig.FeatureGeneratorCount);
		cout << strErrorInfo << endl;
		return -1;
	}
	
	time(&endTime);
	double dSeconds = difftime(endTime, startTime);
	cout << endl;
	cout << "Total time elapsed: " << (int)dSeconds << " (s)." << endl;

	return 0;
}


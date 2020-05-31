#include "SampleGenerator.h"
using namespace std;
IMPLEMENT_DYNAMICCREATE(SampleGenerator)

bool SampleGenerator::Initialize(
	const vector<string>& /* vecExternalFileNames */)
{
	bool fRet = true;
	
	// TODO: add solid codes

	return fRet;
}

bool SampleGenerator::InitializePerQuery(
	  const SimQueryRankingStuff* /* pQueryRankingStuff */)
{
	bool fRet = true;
	
	// TODO: add solid codes

	return fRet;
}

int SampleGenerator::GenerateFeature(
	const string& /* strUrl */, 
	const SOURCE_DOCUMENT* /* pSourceDoc */, 
	const SimDoc* /* pDocStuff */, 
	const CMSNFeatureFile* /* pMSNFeature */, 
	const vector<float>& vecParas,
	vector<float>& vecFeatureValues)
{
	int iRet = 0;
	vecFeatureValues.clear();
		
	// TODO: add solid codes
	vecFeatureValues = vecParas;
	iRet = (int)vecFeatureValues.size();

	return iRet;
}
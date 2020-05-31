#pragma once

#include <string>
#include <map>
#include <vector>
#include "RankingStuff.h"
#include "DocStructure.h"
#include "MSNFeatureFile2.h"
class IFeatureGenerator
{
public:
	virtual ~IFeatureGenerator() {}

public:
	// Loading external files (pagerank table file, etc.)
	virtual bool Initialize(
		const std::vector<std::string>& vecExternalFileNames
	) = 0;

	// Set query string and its DFs
	virtual bool InitializePerQuery(
		  const SimQueryRankingStuff* pQueryRankingStuff
	) = 0;
	
	////////////////////////////////////////////////////////////
	// Generate the features
	// Returns:
	//		-1	- something wrong!
	//		the count of the feature, otherwise
	virtual int GenerateFeature(
		const std::string& strUrl,
		const SOURCE_DOCUMENT* pSourceDoc,
		const SimDoc* pDocStuff,
		const CMSNFeatureFile* pMSNFeature,
		const std::vector<float>& vecParas,
		std::vector<float>& vecFeatureValues
	) = 0;
	
	////////////////////////////////////////////////////////////
	// Give extra data to the generator, e.g. the line idx of the QDJU file
	virtual void SetExtraData(int) {}
};
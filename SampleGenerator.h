#pragma once

#include "IFeatureGenerator.h"
#include "Support.h"

class SampleGenerator : public IFeatureGenerator
{
	DECLARE_DYMANICCREATE

public:
	virtual ~SampleGenerator() {}

public:
	// Loading external files (pagerank table file, etc.)
	bool Initialize(
		const std::vector<std::string>& vecExternalFileNames
	);

	// Set query string and its DFs
	bool InitializePerQuery(
		  const SimQueryRankingStuff* pQueryRankingStuff
	);

	// Generate the features
	virtual int GenerateFeature(
		const std::string& strUrl,
		const SOURCE_DOCUMENT* pSourceDoc,
		const SimDoc* pDocStuff,
		const CMSNFeatureFile* pMSNFeature, 
		const std::vector<float>& vecParas,
		std::vector<float>& vecFeatureValues
	);
};
#pragma once

#include "IFeatureGenerator.h"

typedef IFeatureGenerator* (*PFNCREATEOBJECT)();

#define RESERVEENTRY_OF_GENERATORTABLE	"__RESERVEENTRY"

#define DEFINE_FEATUREGENERATORS \
	std::map<std::string, FeatureGeneratorDesp> featureGenerators;

#define DEFINE_FEATUREGENERATOR(creator_name, generator_name, class_name) \
	FeatureGeneratorDesp class_name::featureGeneratorDesp( \
										#creator_name, \
										#generator_name, \
										#class_name, \
										&class_name::CreateObject); \

#define DECLARE_DYMANICCREATE \
public: \
	static IFeatureGenerator* CreateObject(); \
	static FeatureGeneratorDesp featureGeneratorDesp;

#define IMPLEMENT_DYNAMICCREATE(class_name) \
	IFeatureGenerator* class_name::CreateObject()  \
	{ \
		return new class_name; \
	} \

struct FeatureGeneratorDesp
{
	std::string m_strCreatorName;
	std::string m_strClassName;
	PFNCREATEOBJECT m_pfnCreateObject;

	FeatureGeneratorDesp() {};

	FeatureGeneratorDesp(
		const std::string& strCreatorName,
		const std::string& strGeneratorName,
		const std::string& strClassName,
		PFNCREATEOBJECT pfnCreateObject
	);
};

#pragma once

#include "myAbstractGenerator.h"

namespace generators {
namespace generator {

class BehavioralGenerator : public MyAbstractGenerator
{
public:
	BehavioralGenerator(QString const &templateDirPath
			, QString const &outputDirPath
			, QString const &pathToQReal
			, qReal::LogicalModelAssistInterface const &logicalModel
			, qReal::ErrorReporterInterface &errorReporter
			, QString const &metamodelLanguageName
			, QString const &languageName
			, QString const &toGeneratePropertyName
			, QString const &programNamePropertyName
			, QString const &generatorMetamodelName
			);
	virtual ~BehavioralGenerator();

	/// Starts generation.
	void generate();
};

}
}


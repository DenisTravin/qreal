#pragma once

#include <QtCore/QObject>

#include "../../../qrgui/toolPluginInterface/usedInterfaces/logicalModelAssistInterface.h"
#include "../../../qrgui/toolPluginInterface/usedInterfaces/errorReporterInterface.h"

namespace generators {
namespace generator {

class Generator : public QObject
{
	Q_OBJECT

public:
	Generator();
	virtual ~Generator();

	/// Initializes generator with logical model reference and means to report errors.
	void init(qReal::LogicalModelAssistInterface const &logicalModel
			, qReal::ErrorReporterInterface &errorReporter
			);

	/// Starts generation process.
	void generate(qReal::Id const &metamodel);

	QString generatorModelFullName();
	QString generatorGeneratorModelName();
	QString generatorNormalizerGeneratorModelName();

private:
	bool isCorrectedName(QString const &name);

	/// Logical model, from which generator takes all information.
	qReal::LogicalModelAssistInterface const *mLogicalModel;

	/// Error reporter object, used to show generation errors in output window.
	qReal::ErrorReporterInterface *mErrorReporter;

	QString mGeneratorModelFullName;
	QString mGeneratorModelName;
	QString mGeneratorNormalizerGeneratorModelName;
};

}
}


#include "qrealApplication.h"

#include <QMouseEvent>
#include <QDebug>
#include <QWidget>

#include "../qrutils/usabilityStatistics/usabilityStatistics.h"

QRealApplication::QRealApplication(int &argc, char **argv)
	: QApplication(argc, argv)
{
}

bool QRealApplication::notify(QObject *obj, QEvent *e)
{
	QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent*>(e);

	QWidget *widget = dynamic_cast<QWidget*>(obj);
	QPoint pos;
	if (mouseEvent && widget && mouseEvent->type() == QMouseEvent::MouseButtonPress) {
		pos = widget->pos();
		for (; widget; widget = dynamic_cast<QWidget*>(widget->parent())) {
			pos += widget->pos();
		}
		utils::UsabilityStatistics::reportMouseClick(mouseEvent->pos() + pos);
	}
	return QApplication::notify(obj, e);
}

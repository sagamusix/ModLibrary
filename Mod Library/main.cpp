#include "modlibrary.h"
#include <QtWidgets/QApplication>
#include <QSettings>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QCoreApplication::setOrganizationName("Mod Library");
	QCoreApplication::setOrganizationDomain("");
	QCoreApplication::setApplicationName("Mod Library");
	QSettings::setDefaultFormat(QSettings::IniFormat);

	ModLibrary w;
	w.show();
	return a.exec();
}

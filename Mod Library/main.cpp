#include "modlibrary.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	ModLibrary w;
	w.show();
	return a.exec();
}

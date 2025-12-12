#include "Application.h"

int main(int argc, char * argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    Application app(argc, argv);
	return app.exec();
}

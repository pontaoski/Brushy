#include <QApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	QQmlApplicationEngine engine;
	engine.load(QUrl("qrc:/main.qml"));
	Q_ASSERT(engine.rootObjects().count() == 1);
	return app.exec();
}

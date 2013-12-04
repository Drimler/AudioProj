#include "audiocfg.H"
#include "MyAudioBase.H"
#include "MyAudioInput.H"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Audio Device Test");

    MyAudioCfg w;
    w.show();
//    MyAudioIO s;
//    s.show();
//    MyGUI g;
//    g.show();

    return app.exec();
}

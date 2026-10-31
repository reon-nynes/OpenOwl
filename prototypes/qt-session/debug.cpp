#include "wire.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QProcess>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <QJsonArray>
#include <QFontDatabase>

class SessionClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap state READ state NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QVariantMap topology READ topology NOTIFY topologyChanged)
public:
    QLocalSocket socket;
    QTimer retry;
    QJsonObject snapshot;
    QJsonObject structure;
    QString message="Connecting to session";
    SessionClient() {
        connect(&socket,&QLocalSocket::connected,this,[this]{message="Connected";socket.write(line({{"op","subscribe"}}));emit changed();});
        connect(&socket,&QLocalSocket::disconnected,this,[this]{message="Disconnected — retrying";emit changed();});
        connect(&socket,&QLocalSocket::readyRead,this,[this]{
            while(socket.canReadLine()) {
                auto next=QJsonDocument::fromJson(socket.readLine()).object();
                if(!next["ok"].toBool()){message=next["error"].toString();emit changed();continue;}
                if(!next.contains("scenes"))continue;
                auto newScenes=next["scenes"].toArray();const auto oldScenes=snapshot["scenes"].toArray();
                for(qsizetype i=0;i<newScenes.size();++i){auto scene=newScenes[i].toObject();if(!scene.contains("programImage"))for(const auto &old:oldScenes)if(old.toObject()["id"]==scene["id"])for(const auto &key:{"programImage","liveImage","timelineImage"})scene[key]=old.toObject()[key];newScenes[i]=scene;}
                next["scenes"]=newScenes;
                QJsonArray sceneIds;QJsonObject nodeIds,effectIds;
                for(const auto &value:newScenes){const auto s=value.toObject();const QString sceneId=s["id"].toString();sceneIds.append(sceneId);QJsonArray ids;
                    for(const auto &nodeValue:s["nodes"].toArray()){const auto n=nodeValue.toObject();const QString id=n["id"].toString();ids.append(id);QJsonArray effects;for(const auto &e:n["effects"].toArray())effects.append(e.toObject()["id"]);effectIds[sceneId+"/"+id]=effects;}nodeIds[sceneId]=ids;
                }
                const QJsonObject nextStructure{{"scenes",sceneIds},{"nodes",nodeIds},{"effects",effectIds}};
                snapshot=next;message="Connected";
                if(nextStructure!=structure){structure=nextStructure;emit topologyChanged();}
                emit changed();
            }
        });
        retry.setInterval(700);connect(&retry,&QTimer::timeout,this,[this]{if(socket.state()==QLocalSocket::UnconnectedState)socket.connectToServer(SessionName);});retry.start();socket.connectToServer(SessionName);
        QTimer::singleShot(500,this,[this]{if(socket.state()!=QLocalSocket::ConnectedState)QProcess::startDetached(QCoreApplication::applicationDirPath()+"/owl-session-spike.exe",{},QDir::currentPath());});
    }
    QVariantMap state()const{return snapshot.toVariantMap();}
    QString status()const{return message;}
    QVariantMap topology()const{return structure.toVariantMap();}
    Q_INVOKABLE void send(const QVariantMap &command){if(socket.state()==QLocalSocket::ConnectedState)socket.write(line(QJsonObject::fromVariantMap(command)));}
signals:void changed();void topologyChanged();
};
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);app.setApplicationName("OpenOwl Architecture Spike");
    for(const auto &font:{"segoeui.ttf","seguisb.ttf","consola.ttf","seguisym.ttf"})
        QFontDatabase::addApplicationFont(qEnvironmentVariable("WINDIR")+"/Fonts/"+font);
    const auto args=app.arguments();const bool capture=args.contains("--capture");
    if(capture)QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
    QQuickStyle::setStyle("Basic");
    QDir().mkpath("artifacts");
    qInstallMessageHandler([](QtMsgType,const QMessageLogContext &,const QString &message){static QMutex mutex;QMutexLocker guard(&mutex);QFile log("artifacts/debug.log");if(log.open(QIODevice::Append))log.write(message.toUtf8()+"\n");});
    SessionClient client;QQmlApplicationEngine engine;engine.rootContext()->setContextProperty("client",&client);engine.load(QUrl("qrc:/Main.qml"));
    if(engine.rootObjects().isEmpty())return 2;
    auto *root=engine.rootObjects().first();
    if(args.contains("--studio"))root->setProperty("studioMode",true);
    if(args.contains("--node"))root->setProperty("selectedNode",args.value(args.indexOf("--node")+1));
    if(args.contains("--width"))root->setProperty("width",args.value(args.indexOf("--width")+1).toInt());
    if(args.contains("--height"))root->setProperty("height",args.value(args.indexOf("--height")+1).toInt());
    if(capture)QTimer::singleShot(2500,&app,[&]{
        auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        auto *viewport=window->findChild<QObject*>("programViewport");auto *item=viewport->findChild<QObject*>("viewportImage");
        qInfo()<<"Program image"<<item->property("status")<<item->property("width")<<item->property("height")<<item->property("source").toUrl().toString().size();
        const auto image=window->grabWindow();const QString path=args.value(args.indexOf("--capture")+1);const bool saved=image.save(path);
        QFile report(path+".json");if(report.open(QIODevice::WriteOnly))report.write(QJsonDocument(QJsonObject{{"ok",saved},{"width",image.width()},{"height",image.height()},{"sessionId",client.snapshot["sessionId"]},{"frames",client.snapshot["frames"]},{"nodeDelegatesCreated",window->property("createdNodeDelegates").toInt()},{"programImageStatus",item->property("status").toInt()},{"studio",window->property("studioMode").toBool()}}).toJson());
        app.exit(saved?0:3);
    });
    return app.exec();
}
#include "debug.moc"

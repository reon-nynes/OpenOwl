#pragma once
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QElapsedTimer>

inline const QString SessionName = QStringLiteral("OpenOwl.QtSession.Throwaway.v1");
inline QByteArray line(const QJsonObject &object) {
    return QJsonDocument(object).toJson(QJsonDocument::Compact) + '\n';
}
inline QJsonObject request(const QJsonObject &command) {
    QLocalSocket socket;
    socket.connectToServer(SessionName);
    if (!socket.waitForConnected(1500)) return {{"ok",false},{"error","Session is not running. Launch the debug UI first."}};
    socket.write(line(command)); socket.flush();
    QElapsedTimer deadline; deadline.start();
    while (!socket.canReadLine() && deadline.elapsed() < 3000) socket.waitForReadyRead(100);
    if (!socket.canReadLine()) return {{"ok",false},{"error","Session did not respond."}};
    return QJsonDocument::fromJson(socket.readLine()).object();
}

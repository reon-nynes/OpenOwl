// Disposable CLI and MCP stdio bridge. The engine outlives either client.
#include "wire.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <iostream>
static void print(const QJsonObject &value){std::cout<<QJsonDocument(value).toJson(QJsonDocument::Compact).constData()<<std::endl;}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);const auto args=app.arguments();
    if(args.contains("--mcp")) {
        std::string input;
        while(std::getline(std::cin,input)) {
            const auto call=QJsonDocument::fromJson(QByteArray::fromStdString(input)).object();
            if(!call.contains("id"))continue;
            const auto id=call["id"];const auto method=call["method"].toString();QJsonObject result;
            if(method=="initialize")result={{"protocolVersion",call["params"].toObject()["protocolVersion"].toString("2025-11-25")},{"capabilities",QJsonObject{{"tools",QJsonObject{}}}},{"serverInfo",QJsonObject{{"name","openowl-session-spike"},{"version","0.0.0-throwaway"}}}};
            else if(method=="ping")result={};
            else if(method=="tools/list") {
                const QJsonObject stateTool{{"name","session_state"},{"description","Inspect the existing running Qt architecture-spike session."},{"inputSchema",QJsonObject{{"type","object"},{"properties",QJsonObject{}}}}};
                const QJsonObject commandSchema{{"type","object"},{"properties",QJsonObject{{"op",QJsonObject{{"type","string"}}}}},{"required",QJsonArray{"op"}},{"additionalProperties",true}};
                const QJsonObject commandTool{{"name","session_command"},{"description","Send a disposable command. Operations: property.set, keyframe.set, keyframe.clear, timeline.seek, timeline.play, timeline.activate, output.set, scene.take, node.add, node.move, effect.add, effect.set, reset. Inspect session_state for IDs."},{"inputSchema",commandSchema}};
                result={{"tools",QJsonArray{stateTool,commandTool}}};
            }
            else if(method=="tools/call") {
                const auto params=call["params"].toObject();const auto name=params["name"].toString();
                const auto reply=(name=="session_state"?request({{"op","state"}}):(name=="session_command"?request(params["arguments"].toObject()):QJsonObject{{"ok",false},{"error","Unknown tool."}}));
                result={{"content",QJsonArray{QJsonObject{{"type","text"},{"text",QString::fromUtf8(QJsonDocument(reply).toJson(QJsonDocument::Compact))}}}},{"isError",!reply["ok"].toBool()}};
            } else {print({{"jsonrpc","2.0"},{"id",id},{"error",QJsonObject{{"code",-32601},{"message","Method not found"}}}});continue;}
            print({{"jsonrpc","2.0"},{"id",id},{"result",result}});
        }
        return 0;
    }
    QByteArray input="{\"op\":\"state\"}";
    if(args.contains("--json"))input=args.value(args.indexOf("--json")+1).toUtf8();
    if(args.contains("--file")){QFile file(args.value(args.indexOf("--file")+1));if(!file.open(QIODevice::ReadOnly)){print({{"ok",false},{"error","Cannot read command file."}});return 2;}input=file.readAll();}
    QJsonParseError parse;const auto doc=QJsonDocument::fromJson(input,&parse);
    if(parse.error!=QJsonParseError::NoError||!doc.isObject()){print({{"ok",false},{"error","Expected a JSON command object."}});return 2;}
    const auto reply=request(doc.object());print(reply);return reply["ok"].toBool()?0:1;
}

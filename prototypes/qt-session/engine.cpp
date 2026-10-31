// THROWAWAY architecture experiment. Qt CPU fixtures, not an Owl shader port.
#include "wire.h"
#include <QCoreApplication>
#include <QLocalServer>
#include <QTimer>
#include <QImage>
#include <QPainter>
#include <QBuffer>
#include <QJsonArray>
#include <QFile>
#include <QUuid>
#include <QMap>
#include <QSet>
#include <QDir>
#include <cmath>
#include <algorithm>
#include <d3d11.h>
#include <wrl/client.h>
#include <SpoutDX.h>
using Microsoft::WRL::ComPtr;

struct Values {
    QMap<QString,double> base;
    QMap<QString,QMap<double,double>> keys;
    double at(const QString &property, double t) const {
        const auto it = keys.constFind(property);
        if (it == keys.cend() || it->isEmpty()) return base.value(property);
        const auto &track = it.value();
        auto upper = track.upperBound(t);
        if (upper == track.cbegin()) return upper.value();
        if (upper == track.cend()) return std::prev(upper).value();
        auto lower = std::prev(upper);
        const double weight = (t-lower.key())/(upper.key()-lower.key());
        return lower.value()*(1-weight)+upper.value()*weight;
    }
    QJsonObject json() const { QJsonObject out; for(auto it=base.cbegin();it!=base.cend();++it)out[it.key()]=it.value(); return out; }
};
struct Effect { QString id,kind; Values live,timeline; };
struct Node { QString id,parent,name,kind; QColor color; Values live,timeline; QVector<Effect> effects; };
struct Scene {
    QString id,name;
    QVector<Node> nodes;
    double cursor=0,length=8,weight=0,target=0,origin=0,fadeElapsed=0,duration=1.2;
    bool playing=false;
    QImage program,liveImage,timelineImage;
};
static double mixed(const Values &live,const Values &timeline,const QString &prop,double t,double weight) {
    return live.base.value(prop)*(1-weight)+timeline.at(prop,t)*weight;
}
static QString encoded(const QImage &image) {
    QByteArray bytes; QBuffer buffer(&bytes); buffer.open(QIODevice::WriteOnly); image.save(&buffer,"PNG");
    return QStringLiteral("data:image/png;base64,")+QString::fromLatin1(bytes.toBase64());
}
static Values properties(double x,double y,double scale=1,double opacity=1) {
    return {{{"x",x},{"y",y},{"scale",scale},{"rotation",0},{"opacity",opacity}}, {}};
}
class Session : public QObject {
    Q_OBJECT
public:
    QLocalServer server;
    QSet<QLocalSocket*> subscribers;
    QVector<Scene> scenes;
    QElapsedTimer clock;
    QTimer timer;
    QString sessionId=QUuid::createUuid().toString(QUuid::WithoutBraces), active="stage", lastAction="Session started", outputError;
    quint64 frames=0,sent=0,revision=0;
    double lastTime=0;
    bool output=false,spoutOpen=false;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11Texture2D> texture;
    spoutDX sender;
    Session() {
        reset(); clock.start();
        connect(&server,&QLocalServer::newConnection,this,[this]{
            while(server.hasPendingConnections()) {
                auto *socket=server.nextPendingConnection();
                connect(socket,&QLocalSocket::readyRead,this,[this,socket]{
                    while(socket->canReadLine()) {
                        QJsonParseError parse;
                        const auto doc=QJsonDocument::fromJson(socket->readLine(),&parse);
                        if(parse.error!=QJsonParseError::NoError||!doc.isObject()) {socket->write(line({{"ok",false},{"error","Expected a JSON object."}}));continue;}
                        const auto cmd=doc.object();
                        if(cmd["op"]=="subscribe") {subscribers.insert(socket);socket->write(line(state(true)));}
                        else socket->write(line(dispatch(cmd)));
                    }
                });
                connect(socket,&QLocalSocket::disconnected,this,[this,socket]{subscribers.remove(socket);socket->deleteLater();});
            }
        });
        if(!server.listen(SessionName)) qFatal("A spike session is already running, or the local socket is unavailable.");
        timer.setTimerType(Qt::PreciseTimer); timer.setInterval(33);
        connect(&timer,&QTimer::timeout,this,&Session::tick); timer.start();
        tick();
    }
    ~Session() override {sender.ReleaseSender();sender.CloseDirectX11();}
    void reset() {
        scenes.clear();
        for(int i=0;i<2;++i) {
            Scene s; s.id=i?"gallery":"stage";s.name=i?"Gallery":"Stage";
            Node root{"root","", "Scene root", "folder",{},properties(0,0),properties(0,0),{}};
            Node back{"background","root","Backdrop","layer",QColor(i?"#263b48":"#293044"),properties(320,180,3),properties(320,180,3),{}};
            Node folder{"group","root","Subject folder","folder",{},properties(0,0),properties(25,-15),{}};
            Effect fade{"folder-opacity","Opacity",{{{"amount",1}},{}},{{{"amount",0.5}}, {}}};
            folder.effects.append(fade);
            Node left{"subject","group","Amber panel","layer",QColor("#f0ad66"),properties(220,175),properties(400,140,0.8),{}};
            left.timeline.keys["x"]={{0,160},{4,460},{8,160}};
            left.timeline.keys["rotation"]={{0,-16},{4,28},{8,-16}};
            Node right{"accent","group","Cyan panel","layer",QColor("#63c9cf"),properties(365,220,0.8),properties(260,230,1.2),{}};
            s.nodes={root,back,folder,left,right}; scenes.append(s);
        }
        active="stage";output=false;sender.ReleaseSender();lastAction="Demo reset";++revision;
    }
    Scene *scene(const QString &id) {for(auto &s:scenes)if(s.id==id)return &s;return nullptr;}
    Node *node(Scene &s,const QString &id) {for(auto &n:s.nodes)if(n.id==id)return &n;return nullptr;}
    QImage render(Scene &s,double weight) {
        auto visit=[&](auto &&self,Node &n)->QImage {
            QImage image(640,360,QImage::Format_ARGB32_Premultiplied);image.fill(Qt::transparent);
            QPainter painter(&image);painter.setRenderHint(QPainter::Antialiasing);
            if(n.kind=="folder") {
                // Stored children are back-to-front. Each child's effects resolve inside out.
                for(auto &child:s.nodes)if(child.parent==n.id)painter.drawImage(0,0,self(self,child));
            } else {
                painter.setPen(Qt::NoPen);painter.setBrush(n.color);
                painter.drawRoundedRect(QRectF(-85,-65,170,130).translated(320,180),12,12);
                painter.setPen(QPen(QColor(255,255,255,100),2));
                painter.drawLine(QPointF(250,180),QPointF(390,180));
                painter.drawLine(QPointF(320,125),QPointF(320,235));
            }
            painter.end();
            // Diagnostic effects only. Each operates on the folder's composite.
            for(auto &effect:n.effects) {
                const double amount=mixed(effect.live,effect.timeline,"amount",s.cursor,weight);
                if(effect.kind=="Opacity") {
                    QImage next(image.size(),image.format());next.fill(Qt::transparent);
                    QPainter p(&next);p.setOpacity(std::clamp(amount,0.0,1.0));p.drawImage(0,0,image);p.end();image=next;
                }
            }
            QImage transformed(image.size(),image.format());transformed.fill(Qt::transparent);
            QPainter final(&transformed);final.setRenderHint(QPainter::SmoothPixmapTransform);
            final.setOpacity(std::clamp(mixed(n.live,n.timeline,"opacity",s.cursor,weight),0.0,1.0));
            const double x=mixed(n.live,n.timeline,"x",s.cursor,weight),y=mixed(n.live,n.timeline,"y",s.cursor,weight);
            if(n.kind=="layer") {final.translate(x,y);final.rotate(mixed(n.live,n.timeline,"rotation",s.cursor,weight));final.scale(mixed(n.live,n.timeline,"scale",s.cursor,weight),mixed(n.live,n.timeline,"scale",s.cursor,weight));final.drawImage(-320,-180,image);}
            else {final.translate(x,y);final.rotate(mixed(n.live,n.timeline,"rotation",s.cursor,weight));final.scale(mixed(n.live,n.timeline,"scale",s.cursor,weight),mixed(n.live,n.timeline,"scale",s.cursor,weight));final.drawImage(0,0,image);}
            return transformed;
        };
        QImage result=visit(visit,*node(s,"root"));
        // A clock marker makes continuous rendering visible even with a paused cursor.
        QPainter p(&result);p.fillRect(QRectF(std::fmod(clock.elapsed()*0.08,620.0),350,20,4),QColor("#c6ccd5"));
        return result;
    }
    bool openSpout() {
        if(!device) {
            auto hr=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context);
            if(FAILED(hr)){outputError="Cannot create D3D11 device.";return false;}
            D3D11_TEXTURE2D_DESC d{};d.Width=640;d.Height=360;d.MipLevels=1;d.ArraySize=1;d.Format=DXGI_FORMAT_B8G8R8A8_UNORM;d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
            if(FAILED(device->CreateTexture2D(&d,nullptr,&texture))){outputError="Cannot create output texture.";return false;}
        }
        if(!spoutOpen)spoutOpen=sender.OpenDirectX11(device.Get());
        // ReleaseSender clears the SDK name even while the D3D11 device stays open.
        if(spoutOpen&&sender.GetName()[0]==0)sender.SetSenderName("OpenOwl Architecture Spike");
        if(!spoutOpen)outputError="Cannot initialize Spout.";
        return spoutOpen;
    }
    void tick() {
        const double now=clock.nsecsElapsed()/1e9,dt=std::clamp(now-lastTime,0.0,0.25);lastTime=now;
        for(auto &s:scenes) {
            if(s.playing)s.cursor=std::fmod(s.cursor+dt,s.length);
            if(s.weight!=s.target) {
                s.fadeElapsed+=dt;const double u=std::min(1.0,s.fadeElapsed/std::max(0.05,s.duration));const double eased=u*u*(3-2*u);
                s.weight=s.origin+(s.target-s.origin)*eased;if(u>=1)s.weight=s.target;
            }
            s.liveImage=render(s,0);s.timelineImage=render(s,1);s.program=render(s,s.weight);
        }
        ++frames;
        if(output&&openSpout()) {
            const auto &image=scene(active)->program;
            context->UpdateSubresource(texture.Get(),0,nullptr,image.constBits(),image.bytesPerLine(),0);
            if(sender.SendTexture(texture.Get()))++sent;else outputError="Spout send failed.";
        }
        if(frames%3==0&&!subscribers.isEmpty()) {
            const auto bytes=line(state(true));
            for(auto *socket:subscribers)if(socket->bytesToWrite()<256*1024)socket->write(bytes);
        }
    }
    QJsonObject state(bool images=false) {
        QJsonArray list;
        for(auto &s:scenes) {
            QJsonArray nodes;
            for(auto &n:s.nodes) {
                QJsonObject evaluated,keys,resolved;
                for(auto it=n.timeline.base.cbegin();it!=n.timeline.base.cend();++it){evaluated[it.key()]=n.timeline.at(it.key(),s.cursor);keys[it.key()]=int(n.timeline.keys.value(it.key()).size());resolved[it.key()]=mixed(n.live,n.timeline,it.key(),s.cursor,s.weight);}
                QJsonArray effects;
                for(auto &e:n.effects)effects.append(QJsonObject{{"id",e.id},{"kind",e.kind},{"live",e.live.base.value("amount")},{"timeline",e.timeline.at("amount",s.cursor)},{"keys",int(e.timeline.keys.value("amount").size())}});
                int depth=0;QString parent=n.parent;while(!parent.isEmpty()){++depth;auto *p=node(s,parent);parent=p?p->parent:QString();}
                nodes.append(QJsonObject{{"id",n.id},{"parent",n.parent},{"name",n.name},{"kind",n.kind},{"depth",depth},{"live",n.live.json()},{"timeline",n.timeline.json()},{"evaluated",evaluated},{"resolved",resolved},{"keys",keys},{"effects",effects}});
            }
            QJsonArray orderedNodes;
            auto appendNode=[&](auto &&self,const QString &id)->void {
                for(const auto &entry:nodes)if(entry.toObject()["id"]==id){orderedNodes.append(entry);break;}
                for(auto it=s.nodes.crbegin();it!=s.nodes.crend();++it)if(it->parent==id)self(self,it->id);
            };
            appendNode(appendNode,"root");
            QJsonObject item{{"id",s.id},{"name",s.name},{"cursor",s.cursor},{"length",s.length},{"playing",s.playing},{"activation",s.weight},{"target",s.target},{"duration",s.duration},{"nodes",orderedNodes}};
            if(images){item["programImage"]=encoded(s.program);item["liveImage"]=encoded(s.liveImage);item["timelineImage"]=encoded(s.timelineImage);}
            list.append(item);
        }
        return {{"ok",true},{"sessionId",sessionId},{"pid",double(QCoreApplication::applicationPid())},{"frames",double(frames)},{"revision",double(revision)},{"spoutFrames",double(sent)},{"outputEnabled",output},{"spoutName",spoutOpen&&sender.GetName()[0]?QString::fromUtf8(sender.GetName()):QString("OpenOwl Architecture Spike")},{"outputError",outputError},{"activeScene",active},{"lastAction",lastAction},{"scenes",list}};
    }
    QJsonObject dispatch(const QJsonObject &cmd) {
        const QString op=cmd["op"].toString("state");
        if(op=="state")return state();
        if(op=="stop"){QTimer::singleShot(150,qApp,&QCoreApplication::quit);return {{"ok",true},{"message","Session stopping"}};}
        if(op=="reset"){reset();return state();}
        if(op=="output.set") {output=cmd["enabled"].toBool();outputError.clear();if(!output)sender.ReleaseSender();else openSpout();}
        else {
            auto *s=scene(cmd["scene"].toString(active));
            if(!s)return {{"ok",false},{"error","Scene does not exist."}};
            if(op=="scene.take")active=s->id;
            else if(op=="timeline.seek")s->cursor=std::clamp(cmd["time"].toDouble(),0.0,s->length);
            else if(op=="timeline.play")s->playing=cmd["playing"].toBool();
            else if(op=="timeline.activate") {s->origin=s->weight;s->target=cmd["enabled"].toBool()?1:0;s->fadeElapsed=0;s->duration=std::clamp(cmd["duration"].toDouble(s->duration),0.05,10.0);}
            else {
                auto *n=node(*s,cmd["node"].toString("subject"));
                if(!n)return {{"ok",false},{"error","Node does not exist."}};
                if(op=="property.set"||op=="keyframe.set"||op=="keyframe.clear") {
                    const auto prop=cmd["property"].toString();
                    if(!n->live.base.contains(prop))return {{"ok",false},{"error","Unknown property."}};
                    const double value=cmd["value"].toDouble();
                    if(!std::isfinite(value))return {{"ok",false},{"error","Property value must be finite."}};
                    if(op=="keyframe.set")n->timeline.keys[prop][s->cursor]=value;
                    else if(op=="keyframe.clear"){n->timeline.base[prop]=n->timeline.at(prop,s->cursor);n->timeline.keys.remove(prop);}
                    else if(cmd["setup"]=="timeline"&&!n->timeline.keys.value(prop).isEmpty())n->timeline.keys[prop][s->cursor]=value;
                    else (cmd["setup"].toString()=="timeline"?n->timeline:n->live).base[prop]=value;
                } else if(op=="effect.set"||op=="effect.keyframe.set"||op=="effect.keyframe.clear") {
                    bool found=false;for(auto &e:n->effects)if(e.id==cmd["effect"].toString()){
                        const double amount=std::clamp(cmd["value"].toDouble(),0.0,1.0);
                        if(op=="effect.keyframe.set"||(cmd["setup"]=="timeline"&&!e.timeline.keys.value("amount").isEmpty()&&op=="effect.set"))e.timeline.keys["amount"][s->cursor]=amount;
                        else if(op=="effect.keyframe.clear"){e.timeline.base["amount"]=e.timeline.at("amount",s->cursor);e.timeline.keys.remove("amount");}
                        else (cmd["setup"]=="timeline"?e.timeline:e.live).base["amount"]=amount;found=true;
                    }
                    if(!found)return {{"ok",false},{"error","Effect does not exist."}};
                } else if(op=="effect.add") {
                    const QString id=QUuid::createUuid().toString(QUuid::WithoutBraces);
                    n->effects.append({id,"Opacity",{{{"amount",1}},{}},{{{"amount",1}},{}}});
                } else if(op=="node.add") {
                    if(n->kind!="folder")return {{"ok",false},{"error","Add children to a folder."}};
                    const QString id=QUuid::createUuid().toString(QUuid::WithoutBraces),parent=n->id;
                    const bool folder=cmd["kind"]=="folder";
                    s->nodes.append({id,parent,cmd["name"].toString(folder?"New folder":"New layer"),folder?"folder":"layer",QColor("#b3a0df"),properties(folder?0:320,folder?0:180),properties(folder?0:320,folder?0:180),{}});
                } else if(op=="node.move") {
                    auto *parent=node(*s,cmd["parent"].toString());
                    if(n->id=="root"||!parent||parent->kind!="folder")return {{"ok",false},{"error","Choose a valid folder; the root cannot move."}};
                    QString p=parent->id;while(!p.isEmpty()){if(p==n->id)return {{"ok",false},{"error","A folder cannot contain itself or an ancestor."}};auto *a=node(*s,p);p=a?a->parent:QString();}n->parent=parent->id;
                } else if(op=="capture") {
                    if(!s->program.save(cmd["path"].toString()))return {{"ok",false},{"error","Cannot save image."}};
                } else return {{"ok",false},{"error","Unknown operation."}};
            }
        }
        ++revision;lastAction=op;return state();
    }
};
// Optional diagnostic mode: a separate process/device receives the real Spout stream.
static int receiveProbe(QCoreApplication &app,const QString &path,const QString &expectedPath) {
    spoutDX receiver;
    if(!receiver.OpenDirectX11())return 2;
    receiver.SetReceiverName("OpenOwl Architecture Spike");
    auto *device=receiver.GetDX11Device();ComPtr<ID3D11DeviceContext> context;device->GetImmediateContext(&context);
    const QImage expected=QImage(expectedPath).convertToFormat(QImage::Format_ARGB32_Premultiplied);
    int samples=0,maxDifference=0,changedPixels=0;QImage captured;
    QTimer timer;
    QObject::connect(&timer,&QTimer::timeout,&app,[&]{
        if(!receiver.ReceiveTexture()||!receiver.GetSenderTexture())return;
        receiver.IsUpdated();if(!receiver.IsFrameNew())return;
        auto *source=receiver.GetSenderTexture();D3D11_TEXTURE2D_DESC d{};source->GetDesc(&d);
        d.BindFlags=d.MiscFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Texture2D> staging;if(FAILED(device->CreateTexture2D(&d,nullptr,&staging))){app.exit(3);return;}
        context->CopyResource(staging.Get(),source);D3D11_MAPPED_SUBRESOURCE mapped{};
        if(FAILED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped))){app.exit(3);return;}
        const auto format=d.Format==DXGI_FORMAT_R8G8B8A8_UNORM?QImage::Format_RGBA8888_Premultiplied:QImage::Format_ARGB32_Premultiplied;
        captured=QImage(static_cast<const uchar*>(mapped.pData),int(d.Width),int(d.Height),int(mapped.RowPitch),format).convertToFormat(QImage::Format_ARGB32_Premultiplied).copy();context->Unmap(staging.Get(),0);
        if(captured.size()!=expected.size()||expected.isNull()){app.exit(4);return;}
        for(int y=0;y<348;++y)for(int x=0;x<640;++x){const auto a=captured.pixel(x,y),b=expected.pixel(x,y);const int diff=std::max({std::abs(qRed(a)-qRed(b)),std::abs(qGreen(a)-qGreen(b)),std::abs(qBlue(a)-qBlue(b)),std::abs(qAlpha(a)-qAlpha(b))});maxDifference=std::max(maxDifference,diff);if(diff>1)++changedPixels;}
        if(++samples<6)return;
        const bool saved=captured.save(path);QFile report(path+".json");if(!report.open(QIODevice::WriteOnly)){app.exit(3);return;}
        report.write(QJsonDocument(QJsonObject{{"ok",saved&&changedPixels==0},{"receiverPid",double(QCoreApplication::applicationPid())},{"samples",samples},{"width",captured.width()},{"height",captured.height()},{"comparedRows",348},{"maxChannelDifference",maxDifference},{"pixelsAboveOneByte",changedPixels},{"limit","Static fixture; last 12 rows excluded because the independent session clock marker moves."}}).toJson());
        app.exit(saved&&changedPixels==0?0:5);
    });
    timer.start(40);QTimer::singleShot(8000,&app,[&]{app.exit(6);});const int result=app.exec();receiver.ReleaseReceiver();receiver.CloseDirectX11();return result;
}
int main(int argc,char **argv){QCoreApplication app(argc,argv);const auto args=app.arguments();if(args.contains("--receive"))return receiveProbe(app,args.value(args.indexOf("--receive")+1),args.value(args.indexOf("--expected")+1));Session session;return app.exec();}
#include "engine.moc"

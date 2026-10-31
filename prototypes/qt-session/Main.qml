import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    visible: true
    width: 1440; height: 960
    minimumWidth: 1060; minimumHeight: 760
    title: "OpenOwl · Qt session architecture spike"
    color: bg
    font.family: "Segoe UI"; font.pixelSize: 13
    palette.window: bg; palette.windowText: ink
    palette.base: panel; palette.text: ink
    palette.button: "#303b4c"; palette.buttonText: ink
    palette.highlight: cyan; palette.highlightedText: "#17212f"
    readonly property color bg: "#171f2c"
    readonly property color panel: "#222d3e"
    readonly property color ink: "#e7ecf4"
    readonly property color muted: "#acb8ca"
    readonly property color cyan: "#63c9cf"
    readonly property color amber: "#f0ad66"
    property string selectedScene: "stage"
    property string selectedNode: "subject"
    property string selectedEffect: "folder-opacity"
    property real fadeDuration: 1.2
    property bool studioMode:false
    property int createdNodeDelegates:0
    readonly property var session: client.state
    readonly property var scenes: session.scenes || []
    readonly property var scene: findScene(selectedScene)
    readonly property var outputScene: findScene(session.activeScene || "stage")
    readonly property var node: findNode()
    readonly property var effect: findEffect()
    function findScene(id) { for(let s of scenes) if(s.id===id) return s; return {nodes:[],cursor:0,length:8,activation:0,target:0,playing:false}; }
    function findNode() { for(let n of scene.nodes) if(n.id===selectedNode) return n; return {live:{},timeline:{},evaluated:{},keys:{},effects:[],name:"Choose a layer",kind:""}; }
    function nodeById(id) { for(let n of scene.nodes)if(n.id===id)return n;return {id:id,name:"",kind:"layer",depth:0}; }
    function effectById(id) { for(let e of node.effects)if(e.id===id)return e;return {id:id,kind:""}; }
    function findEffect() { for(let e of node.effects) if(e.id===selectedEffect)return e;return node.effects.length?node.effects[0]:null; }
    function number(value) { return Number(value || 0).toFixed(2); }
    function command(op, fields) { let c={op:op,scene:selectedScene,node:selectedNode}; if(fields)for(let key in fields)c[key]=fields[key];client.send(c); }

    component Caption: Label { color: window.muted; font.pixelSize: 11; font.letterSpacing: 0.8 }
    component Box: Rectangle { color: window.panel; radius: 6; border.color: "#354155" }
    component Viewport: Rectangle {
        property string imageSource: ""
        color: "#101621"; radius: 3; clip: true
        Canvas {
            anchors.fill: parent
            onPaint: { let p=getContext("2d");for(let y=0;y<height;y+=16)for(let x=0;x<width;x+=16){p.fillStyle=((x/16+y/16)%2)?"#263040":"#202938";p.fillRect(x,y,16,16);} }
            onWidthChanged: requestPaint();onHeightChanged: requestPaint()
        }
        Image { objectName:"viewportImage";anchors.fill: parent; fillMode: Image.PreserveAspectFit; source: parent.imageSource; cache: false }
    }
    component PropertyRow: ColumnLayout {
        required property string propertyName
        required property string label
        required property real low
        required property real high
        spacing: 3
        RowLayout {
            Layout.fillWidth: true
            Label { text: label; color: window.ink; Layout.fillWidth: true }
            Label { text: (window.node.keys[propertyName]||0)+" keys"; color: window.muted; font.pixelSize: 11 }
            Button { text: "+ Key"; implicitHeight: 24; implicitWidth: 54;padding:6;font.pixelSize:11; Accessible.name: "Keyframe "+label; onClicked: window.command("keyframe.set",{property:propertyName,value:window.node.evaluated[propertyName]||0}) }
            Button { text: "Clear"; implicitHeight: 24; implicitWidth: 48;padding:6;font.pixelSize:11; enabled: (window.node.keys[propertyName]||0)>0; Accessible.name: "Clear "+label+" keyframes"; onClicked: window.command("keyframe.clear",{property:propertyName}) }
        }
        RowLayout {
            Layout.fillWidth:true
            Label { text: "L"; color: window.cyan; Layout.preferredWidth: 14 }
            Slider { Layout.fillWidth:true; from:low;to:high;value:window.node.live[propertyName]||0; Accessible.name:"Live "+label; onMoved:window.command("property.set",{setup:"live",property:propertyName,value:value}) }
            Label { text:window.number(window.node.live[propertyName]);color:window.cyan;font.family:"Cascadia Mono";Layout.preferredWidth:50;horizontalAlignment:Text.AlignRight }
        }
        RowLayout {
            Layout.fillWidth:true
            Label { text: "T"; color: window.amber; Layout.preferredWidth: 14 }
            Slider { Layout.fillWidth:true;from:low;to:high;value:window.node.evaluated[propertyName]||0; Accessible.name:"Timeline "+label;onMoved:window.command("property.set",{setup:"timeline",property:propertyName,value:value}) }
            Label { text:window.number(window.node.evaluated[propertyName]);color:window.amber;font.family:"Consolas";Layout.preferredWidth:50;horizontalAlignment:Text.AlignRight }
        }
    }

    ColumnLayout {
        anchors.fill:parent; anchors.margins:18; spacing:12
        RowLayout {
            Layout.fillWidth:true;spacing:16
            Label { text:"OpenOwl";color:ink;font.pixelSize:24;font.weight:Font.DemiBold }
            Caption { text:"QT SESSION / THROWAWAY SPIKE" }
            Item { Layout.fillWidth:true }
            Label { text:client.status; color:cyan }
            Label { text:"PID "+(session.pid||"—")+"   FRAME "+(session.frames||0);color:muted;font.family:"Cascadia Mono" }
            Button { text:"Studio view";checkable:true;checked:window.studioMode;onClicked:window.studioMode=!window.studioMode }
            Button { text:session.outputEnabled?"Spout on":"Spout off";checkable:true;checked:session.outputEnabled||false;onClicked:window.command("output.set",{enabled:!session.outputEnabled}) }
            Button { text:"Reset demo";onClicked:window.command("reset") }
        }
        Label { text:"Explore shared folders and effects, independent live/timeline values, and activation that follows a moving cursor.";color:muted;Layout.fillWidth:true;wrapMode:Text.Wrap }
        RowLayout {
            Layout.fillWidth:true;Layout.fillHeight:true;spacing:12
            Box {
                Layout.preferredWidth:225;Layout.fillHeight:true
                ColumnLayout {
                    anchors.fill:parent;anchors.margins:12;spacing:10
                    Caption { text:"SCENES / STUDIO" }
                    Repeater {
                        model:client.topology.scenes || []
                        delegate:Button {
                            required property string modelData
                            readonly property var entry:window.findScene(modelData)
                            Layout.fillWidth:true;implicitHeight:42
                            text:entry.name+(session.activeScene===modelData?"  · OUTPUT":"")
                            checkable:true;checked:window.selectedScene===modelData
                            onClicked:{window.selectedScene=modelData;window.selectedNode="subject";}
                        }
                    }
                    Button { text:"Send selected scene to output";Layout.fillWidth:true;onClicked:window.command("scene.take") }
                    Rectangle { Layout.fillWidth:true;height:1;color:"#354155" }
                    Caption { text:"SHARED LAYERS & FOLDERS" }
                    ScrollView {
                        id:treeScroll
                        Layout.fillWidth:true;Layout.fillHeight:true;clip:true;contentWidth:availableWidth
                        ColumnLayout {
                            width:treeScroll.availableWidth;spacing:3
                            Repeater {
                                model:client.topology.nodes ? (client.topology.nodes[window.selectedScene]||[]) : []
                                delegate:Button {
                                    required property string modelData
                                    readonly property var entry:window.nodeById(modelData)
                                    Component.onCompleted:window.createdNodeDelegates++
                                    Layout.fillWidth:true;implicitHeight:35
                                    text:"  ".repeat(entry.depth)+(entry.kind==="folder"?"▸ ":"◇ ")+entry.name
                                    contentItem:Label { text:parent.text;color:window.selectedNode===modelData?window.ink:window.muted;verticalAlignment:Text.AlignVCenter;elide:Text.ElideRight }
                                    background:Rectangle { radius:3;color:window.selectedNode===modelData?"#3a4a60":"transparent" }
                                    onClicked:{window.selectedNode=modelData;window.selectedEffect="";}
                                }
                            }
                        }
                    }
                    RowLayout {
                        Button { text:"+ Layer";Layout.fillWidth:true;enabled:window.node.kind==="folder";onClicked:window.command("node.add",{kind:"layer"}) }
                        Button { text:"+ Folder";Layout.fillWidth:true;enabled:window.node.kind==="folder";onClicked:window.command("node.add",{kind:"folder"}) }
                    }
                    Button { text:"Move selected into scene root";Layout.fillWidth:true;enabled:window.selectedNode!=="root";onClicked:window.command("node.move",{parent:"root"}) }
                    Label { text:"Structure is shared by both setups.\nClosing this window leaves the session running.";wrapMode:Text.Wrap;Layout.fillWidth:true;color:muted;font.pixelSize:11 }
                }
            }
            ColumnLayout {
                Layout.fillWidth:true;Layout.fillHeight:true;spacing:9
                RowLayout {
                    Caption { text:"OUTPUT / "+(window.outputScene.name||"") }
                    Item { Layout.fillWidth:true }
                    Label { text:"LIVE  "+Math.round((1-window.outputScene.activation)*100)+"%  /  TIMELINE  "+Math.round(window.outputScene.activation*100)+"%";color:window.outputScene.activation>0?amber:cyan;font.family:"Cascadia Mono";font.pixelSize:12 }
                }
                Item {
                    Layout.fillWidth:true;Layout.fillHeight:true;Layout.minimumHeight:210
                    Viewport { objectName:"programViewport";anchors.fill:parent;visible:!window.studioMode;imageSource:window.outputScene.programImage||"" }
                    GridLayout {
                        anchors.fill:parent;columns:2;columnSpacing:10;visible:window.studioMode
                        Repeater {
                            model:client.topology.scenes || []
                            delegate:ColumnLayout {
                                required property string modelData
                                readonly property var entry:window.findScene(modelData)
                                Layout.fillWidth:true;Layout.fillHeight:true
                                Caption { text:entry.name+(session.activeScene===modelData?" · OUTPUT":"") }
                                Viewport { Layout.fillWidth:true;Layout.fillHeight:true;imageSource:parent.entry.programImage||"" }
                                RowLayout {
                                    Button { text:"Inspect";Layout.fillWidth:true;onClicked:window.selectedScene=modelData }
                                    Button { text:"Take to output";Layout.fillWidth:true;onClicked:client.send({op:"scene.take",scene:modelData}) }
                                }
                            }
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth:true;spacing:10
                    ColumnLayout { Layout.fillWidth:true; Caption { text:"LIVE CONFIGURATION";color:cyan } Viewport { Layout.fillWidth:true;Layout.preferredHeight:150;imageSource:window.scene.liveImage||"" } }
                    ColumnLayout { Layout.fillWidth:true; Caption { text:"TIMELINE AT CURSOR";color:amber } Viewport { Layout.fillWidth:true;Layout.preferredHeight:150;imageSource:window.scene.timelineImage||"" } }
                }
                Box {
                    Layout.fillWidth:true;Layout.preferredHeight:138
                    ColumnLayout {
                        anchors.fill:parent;anchors.margins:12;spacing:7
                        RowLayout {
                            Button { text:window.scene.playing?"Pause":"Play";onClicked:window.command("timeline.play",{playing:!window.scene.playing}) }
                            Label { text:window.number(window.scene.cursor)+" / "+window.number(window.scene.length)+" s";color:ink;font.family:"Cascadia Mono" }
                            Item { Layout.fillWidth:true }
                            Button { text:window.scene.target>0?"Return to live":"Activate timeline";highlighted:true;onClicked:window.command("timeline.activate",{enabled:window.scene.target===0,duration:window.fadeDuration}) }
                        }
                        Slider { Layout.fillWidth:true;from:0;to:window.scene.length;value:window.scene.cursor;Accessible.name:"Timeline cursor";onMoved:window.command("timeline.seek",{time:value}) }
                        RowLayout {
                            Label { text:"Activation duration";color:muted }
                            Slider { Layout.fillWidth:true;from:0.1;to:5;value:window.fadeDuration;Accessible.name:"Activation duration";onMoved:window.fadeDuration=value }
                            Label { text:window.number(window.fadeDuration)+" s";color:ink }
                        }
                    }
                }
            }
            Box {
                Layout.preferredWidth:335;Layout.fillHeight:true
                ScrollView {
                    id:inspector
                    anchors.fill:parent;anchors.margins:12;clip:true;contentWidth:availableWidth
                    ColumnLayout {
                        width:inspector.availableWidth;spacing:12
                        Label { text:window.node.name;color:ink;font.pixelSize:18;font.weight:Font.DemiBold;Layout.fillWidth:true;elide:Text.ElideRight }
                        Caption { text:"CYAN: LIVE  /  AMBER: TIMELINE" }
                        Label { text:"Timeline sliders edit at the cursor. Animated properties write a key; others keep a static value.";color:muted;wrapMode:Text.Wrap;Layout.fillWidth:true;font.pixelSize:11 }
                        PropertyRow { propertyName:"x";label:"Position X";low:-200;high:700;Layout.fillWidth:true }
                        PropertyRow { propertyName:"y";label:"Position Y";low:-200;high:500;Layout.fillWidth:true }
                        PropertyRow { propertyName:"scale";label:"Scale";low:0.1;high:4;Layout.fillWidth:true }
                        PropertyRow { propertyName:"rotation";label:"Rotation";low:-180;high:180;Layout.fillWidth:true }
                        PropertyRow { propertyName:"opacity";label:"Opacity";low:0;high:1;Layout.fillWidth:true }
                        Rectangle { Layout.fillWidth:true;height:1;color:"#354155" }
                        RowLayout { Caption { text:"EFFECT STACK";Layout.fillWidth:true } Button { text:"+ Opacity";implicitHeight:28;onClicked:window.command("effect.add") } }
                        Repeater {
                            model:client.topology.effects ? (client.topology.effects[window.selectedScene+"/"+window.selectedNode]||[]) : []
                            delegate:Button { required property string modelData;text:window.effectById(modelData).kind;Layout.fillWidth:true;implicitHeight:28;onClicked:window.selectedEffect=modelData }
                        }
                        Label { visible:window.effect!==null;text:window.node.kind==="folder"?"Applied to combined folder contents":"Applied to this layer";color:muted;font.pixelSize:11;wrapMode:Text.Wrap;Layout.fillWidth:true }
                        RowLayout {
                            visible:window.effect!==null
                            Label { text:window.effect?(window.effect.keys+" keys"):"";color:muted;Layout.fillWidth:true }
                            Button { text:"+ Key";implicitHeight:24;onClicked:window.command("effect.keyframe.set",{effect:window.effect.id,value:window.effect.timeline}) }
                            Button { text:"Clear";implicitHeight:24;enabled:window.effect!==null&&window.effect.keys>0;onClicked:window.command("effect.keyframe.clear",{effect:window.effect.id}) }
                        }
                        RowLayout {
                            visible:window.effect!==null
                            Label { text:"Live";color:cyan;Layout.preferredWidth:55 }
                            Slider { Layout.fillWidth:true;from:0;to:1;value:window.effect?window.effect.live:1;Accessible.name:"Live effect opacity";onMoved:window.command("effect.set",{effect:window.effect.id,setup:"live",value:value}) }
                            Label { text:window.effect?window.number(window.effect.live):"";color:cyan }
                        }
                        RowLayout {
                            visible:window.effect!==null
                            Label { text:"Timeline";color:amber;Layout.preferredWidth:55 }
                            Slider { Layout.fillWidth:true;from:0;to:1;value:window.effect?window.effect.timeline:1;Accessible.name:"Timeline effect opacity";onMoved:window.command("effect.set",{effect:window.effect.id,setup:"timeline",value:value}) }
                            Label { text:window.effect?window.number(window.effect.timeline):"";color:amber }
                        }
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth:true
            Label { text:session.outputError||("Spout: "+(session.spoutName||"off")+"   ·   Sent "+(session.spoutFrames||0));color:session.outputError?amber:muted;font.pixelSize:11 }
            Item { Layout.fillWidth:true }
            Label { text:"640 × 360 / CPU diagnostic compositor / in-memory state";color:muted;font.pixelSize:11 }
        }
    }
}

import QtQuick 2.12
import svgt 1.0

Item {
    width: 1000
    height: 800

    SvgtImage {
        id: _sv
        property color rightEye: "white"
        property color leftEye: "green"
        
        anchors.fill: parent
        template: "../qml-sample/vectorpaint1.svg"
    }

    MouseArea {
        property bool flag: true
        anchors.fill: parent
        onClicked: {
            _sv.rightEye = flag ? "red" : "blue"
            flag ^= true
        }
    }
}
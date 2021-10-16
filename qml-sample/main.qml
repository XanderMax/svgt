import QtQuick 2.12
import svgt 1.0

Item {
    width: 1000
    height: 800

    SVGTemplate {
        id: _sv
        property color rightEye: "red"
        property color leftEye: "red"
        
        anchors.fill: parent
        template: "../qml-sample/vectorpaint.svg"
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            console.log("sdfsdfsdfsdf")
            _sv.rightEye = "blue";
        }
    }
}
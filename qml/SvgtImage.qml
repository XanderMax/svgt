import QtQuick 2.12
import svgt 1.0

Item {
    id: root
    property alias template: _template.source

    Component.onCompleted: {
        _template.object = root
    }

    Image {
        id: _image
        anchors.fill : parent
        source: _template.destination
    }

    SvgtItemImpl {
        id: _template
        engine: SvgtEngineInstance
    }
}
import QtQuick 2.12
import svgt 1.0

Image {
    id: root
    property alias template: _template.source
    source: _template.destination

    SVGTItem {
        id: _template
        item: root
    }
}
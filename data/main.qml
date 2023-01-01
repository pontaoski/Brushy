import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import cc.blackquill.janet.Brushy 1.0

ApplicationWindow {
    visible: true
    width: 800
    height: 800

    Item {
        Canvassy {
            width: 800
            height: 800
            implicitWidth: 800
            implicitHeight: 800
            subcanvassy: sub
        }
        Subcanvassy {
            id: sub
            width: 800
            height: 800
            implicitWidth: 800
            implicitHeight: 800
        }
    }
}

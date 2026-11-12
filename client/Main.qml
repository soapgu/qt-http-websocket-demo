import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1000; height: 760
    minimumWidth: 720; minimumHeight: 600
    visible: true
    title: "HTTP 提交与 WebSocket 广播"
    property bool readyToClose: false
    Connections {
        target: network
        function onReadyToQuit() {
            window.readyToClose = true
        }
    }
    onClosing: function(close) {
        if (readyToClose) {
            close.accepted = true
            return
        }
        close.accepted = false
        window.contentItem.enabled = false
        network.shutdown()
    }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 20; spacing: 12
        Label { text: "HTTP 提交 · WebSocket 订阅"; font.pixelSize: 24 }
        Label { text: "先连接 WebSocket，再提交 JSON；所有订阅者都会收到广播。" }
        GroupBox {
            title: "HTTP"; Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                RowLayout {
                    TextField { id: base; text: "http://127.0.0.1:8080"; Layout.fillWidth: true; placeholderText: "HTTP 服务地址" }
                    Button { text: "查询状态"; enabled: !network.busy; onClicked: network.request(base.text, false, "") }
                    Button { text: "提交 JSON"; enabled: !network.busy; onClicked: network.request(base.text, true, body.text) }
                }
                ScrollView {
                    Layout.fillWidth: true; Layout.preferredHeight: 90
                    TextArea { id: body; text: '{"message":"你好"}'; wrapMode: TextEdit.Wrap; selectByMouse: true; placeholderText: "JSON 请求体" }
                }
                ScrollView {
                    Layout.fillWidth: true; Layout.preferredHeight: 110
                    TextArea { text: network.response; readOnly: true; wrapMode: TextEdit.Wrap; selectByMouse: true; placeholderText: "HTTP 响应" }
                }
            }
        }
        GroupBox {
            title: "WebSocket 广播订阅"; Layout.fillWidth: true; Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent
                RowLayout {
                    TextField { id: ws; text: "ws://127.0.0.1:8080/ws"; Layout.fillWidth: true }
                    Label { text: network.state }
                    Button { text: "连接"; enabled: network.state === "未连接"; onClicked: network.connectSocket(ws.text) }
                    Button { text: "断开"; enabled: network.state === "已连接" || network.state === "连接中"; onClicked: network.disconnectSocket() }
                }
                ScrollView {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    TextArea { text: network.log; readOnly: true; wrapMode: TextEdit.Wrap; selectByMouse: true; placeholderText: "等待广播消息…" }
                }
            }
        }
    }
}

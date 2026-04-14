#pragma once

namespace BLE {
    enum class Event {
        BT_ADVERTISE_BTN_PRESSED,
        BT_CONNECTION_REQUEST,
        BT_CONNECTION_TIMEOUT,
        BT_CONNECTION_LOST,
        BT_DATA_READY_TO_SEND,
        BT_TX_COMPLETE,
        BT_RX_COMPLETE,
        BT_NO_EVENT
    };
}
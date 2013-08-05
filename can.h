#define TYPE_LIST(X) \
    X(nop, 0) \
\
    X(value_periodic, 1) \
    X(value_explicit, 2) \
    X(set_time, 3) \
    X(set_interval, 4) \
    X(sos_reboot, 5) \
    X(sos_rx_overrun, 6) \
    X(sos_stfu, 7) \
    X(sos_nostfu, 8) \
\
    X(file_read, 0x11) \
    X(file_write, 0x11) \
    X(file_error, 0x12) \
    X(file_tree, 0x13) \
    X(file_header, 0x14) \
\
    X(sensor_error, 0x13) \
\
\
    X(file_checksum, 0x15) \
\
\
    X(get_arm, 0x20) \
    X(set_arm, 0x21) \
\
    X(get_output, 0x22) \
    X(set_output, 0x23) \
\
    X(get_value, 0x24) \
    X(set_value, 0x25) \
\
    X(xfer_cts, 0x80) \
    X(xfer_chunk, 0x81) \
    X(xfer_cancel, 0x82) \
\
    X(modema_at_0, 0x90) \
    X(modema_at_1, 0x91) \
    X(modema_at_2, 0x92) \
    X(modema_at_3, 0x93) \
    X(modema_at_4, 0x94) \
    X(modema_at_5, 0x95) \
    X(modema_at_6, 0x96) \
    X(modema_at_7, 0x97) \
\
    X(get_unread_logs, 0xa0) \
\
    X(invalid, 0xff) \


#define ID_LIST(X) \
    X(any,     0x00) \
    X(ping,    0x01) \
    X(pong,    0x02) \
    X(laser,   0x03) \
    X(gps,     0x04) \
    X(temp,    0x05) \
    X(time,    0x06) \
    X(sd,      0x07) \
    X(arm,     0x08) \
    X(heater,  0x09) \
    X(lidar,   0x0a) \
    X(powershot, 0x0b) \
    X(9dof,    0x0c) \
    X(compass, 0x0d) \
    X(modema,  0x0e) \
    X(modemb,  0x0f) \
    X(accel,   0x10) \
    X(gyro,    0x11) \
    X(invalid, 0x1f) \


enum type {
#define X(name, value) TYPE_ ## name = value, \

    TYPE_LIST(X)
#undef X
};


enum id {
#define X(name, value) ID_ ## name = value, \

    ID_LIST(X)
#undef X
};


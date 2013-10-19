#define TYPE_LIST(X) \
    X(nop, 0) \
\
    X(value_periodic, 1) \
    X(value_explicit, 2) \
    X(value_request, 3) \
    X(set_time, 4) \
    X(set_interval, 5) \
    X(sos_reboot, 6) \
    X(sos_rx_overrun, 7) \
    X(sos_stfu, 8) \
    X(sos_nostfu, 9) \
\
    X(file_read, 0x10) \
    X(file_write, 0x11) \
    X(file_error, 0x12) \
    X(file_tree, 0x13) \
    X(dcim_read, 0x14) \
    X(dcim_header, 0x15) \
    X(file_header, 0x16) \
    X(disk_full, 0x17) \
    X(dcim_len, 0x18) \
    X(file_offer, 0x19) \
\
    X(get_arm, 0x20) \
    X(set_arm, 0x21) \
\
    X(sensor_error, 0x1a) \
    X(file_checksum, 0x1b) \
\
    X(xfer_cts, 0x80) \
    X(xfer_chunk, 0x81) \
    X(xfer_cancel, 0x82) \
\
    X(at_0_write, 0x90) \
    X(at_1_write, 0x91) \
    X(at_2_write, 0x92) \
    X(at_3_write, 0x93) \
    X(at_4_write, 0x94) \
    X(at_5_write, 0x95) \
    X(at_6_write, 0x96) \
    X(at_7_write, 0x97) \
    X(at_send,    0x98) \
\
    X(get_unread_logs, 0xa0) \
\
    X(format_error, 0xb0) \
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
    X(powershot, 0x0b) \
    X(9dof,    0x0c) \
    X(compass, 0x0d) \
    X(modema,  0x0e) \
    X(modemb,  0x0f) \
    X(accel,   0x10) \
    X(gyro,    0x11) \
    X(enviro,  0x12) \
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


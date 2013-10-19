#define TYPE_LIST(X) \
    X(nop, 0x00) \
\
    X(command, 0x01) \
    X(value_request, 0x02) \
    X(value_periodic, 0x03) \
    X(value_explicit, 0x04) \
\
    X(set_time, 0x0a) \
    X(set_interval, 0x0b) \
    X(sos_reboot, 0x0c) \
    X(sos_rx_overrun, 0x0d) \
    X(sos_stfu, 0x0e) \
    X(sos_nostfu, 0x0f) \
    X(sensor_error, 0x10) \
    X(format_error, 0x11) \
\
    X(file_offer, 0x20) \
    X(file_checksum, 0x21) \
\
    X(xfer_cts, 0x30) \
    X(xfer_chunk, 0x31) \
    X(xfer_cancel, 0x32) \
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

// Tags are used for defining the specific
// scope of a command or value_* type message
// i.e. a type of value_periodic with a tag of temp
// or a command with tag get_arm
#define TAG_LIST(X) \
    X(file_read, 0x00) \
    X(file_write, 0x01) \
    X(file_error, 0x02) \
    X(file_tree, 0x03) \
    X(dcim_read, 0x04) \
    X(dcim_header, 0x05) \
    X(file_header, 0x06) \
    X(disk_full, 0x07) \
    X(dcim_len, 0x08) \
\
    X(at_0_write, 0x00) \
    X(at_1_write, 0x01) \
    X(at_2_write, 0x02) \
    X(at_3_write, 0x03) \
    X(at_4_write, 0x04) \
    X(at_5_write, 0x05) \
    X(at_6_write, 0x06) \
    X(at_7_write, 0x07) \
    X(at_send,    0x08) \
\
    X(get_unread_logs, 0x00) \
\
    X(get_arm, 0x00) \
    X(set_arm, 0x01) \
    X(get_stepper, 0x02) \
    X(set_stepper, 0x03) \
\
    X(laser_dist, 0x00) \
\


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


enum tag {
#define X(name, value) TAG_ ## name = value, \

    TAG_LIST(X)
#undef X
};

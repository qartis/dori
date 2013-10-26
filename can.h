#define TYPE_LIST(X) \
    X(nop, 0x00) \
\
    X(value_request, 0x01) \
    X(value_periodic, 0x02) \
    X(value_explicit, 0x03) \
\
    X(set_interval, 0x04) \
    X(sos_reboot, 0x05) \
    X(sos_rx_overrun, 0x06) \
    X(sos_stfu, 0x07) \
    X(sos_nostfu, 0x08) \
    X(sensor_error, 0x09) \
    X(format_error, 0x0a) \
\
    X(file_read, 0x10) \
    X(file_write, 0x11) \
    X(file_offer, 0x12) \
    X(file_checksum, 0x13) \
    X(xfer_cts, 0x14) \
    X(xfer_chunk, 0x15) \
    X(xfer_cancel, 0x16) \
    X(file_header, 0x17) \
    X(file_tree, 0x18) \
    X(file_error, 0x19) \
\
    X(dcim_read, 0x1a) \
    X(dcim_header, 0x1a) \
    X(dcim_len, 0x1b) \
    X(disk_full, 0x1c) \
\
    X(action_modema_powercycle, 0x20) \
    X(action_modemb_powercycle, 0x21) \
\
    X(action_arm_spin, 0x30) \
    X(action_arm_angle, 0x31) \
    X(action_drive, 0x32) \
    X(action_camera_button, 0x33) \
\
    X(at_0_write, 0x41) \
    X(at_1_write, 0x42) \
    X(at_2_write, 0x43) \
    X(at_3_write, 0x44) \
    X(at_4_write, 0x45) \
    X(at_5_write, 0x46) \
    X(at_6_write, 0x47) \
    X(at_7_write, 0x48) \
    X(at_send,    0x49) \
\
    X(invalid, 0xff) \


#define ID_LIST(X) \
    X(any,     0x00) \
    X(imaging, 0x01) \
    X(logger,  0x02) \
    X(arm,     0x03) \
    X(diag,    0x04) \
    X(9dof,    0x05) \
    X(modema,  0x06) \
    X(modemb,  0x07) \
    X(enviro,  0x08) \
    X(dive,    0x09)

#define SENSOR_LIST(X) \
    X(none,     0x00) \
    X(stepper,  0x01) \
    X(arm,      0x02) \
    X(gyro,     0x03) \
    X(gps,      0x04) \
    X(compass,  0x05) \
    X(accel,    0x06) \
    X(time,     0x07) \
    X(laser,    0x08) \
    X(wind,     0x09) \
    X(rain,     0x0a) \
    X(humidity, 0x0b) \
    X(pressure, 0x0c) \
    X(voltage,  0x0d) \
    X(current,  0x0e) \
    X(temp0,    0x0f) \
    X(temp1,    0x10) \
    X(temp2,    0x11) \
    X(temp3,    0x12) \
    X(temp4,    0x13) \
    X(temp5,    0x14) \
    X(temp6,    0x15) \
    X(temp7,    0x16) \
    X(temp8,    0x17) \


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

enum sensor {
#define X(name, value) SENSOR_ ## name = value, \

    SENSOR_LIST(X)
#undef X
};

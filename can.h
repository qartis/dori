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
    X(action_modemb_wipesms,    0x22) \
    X(action_modema_connect,    0x23) \
    X(action_modema_disconnect, 0x24) \
\
    X(action_stepper_angle, 0x30) \
    X(action_arm_angle, 0x31) \
    X(action_drive, 0x32) \
    X(action_camera_button, 0x33) \
    X(action_stepper_setstate, 0x34) \
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
    X(ircam_header, 0x50) \
    X(ircam_read, 0x51) \
    X(ircam_reset, 0x52) \
\
    X(laser_sweep_header, 0x60) \
    X(laser_sweep_begin, 0x61) \
\
    X(set_sms_dest, 0x70) \
    X(set_ip, 0x71) \
\
    X(invalid, 0xff) \


#define ID_LIST(X) \
    X(none,    0x00) \
    X(any,     0x01) \
    X(cam,     0x02) \
    X(arm,     0x03) \
    X(diag,    0x04) \
    X(9dof,    0x05) \
    X(modema,  0x06) \
    X(modemb,  0x07) \
    X(enviro,  0x08) \
    X(drive,   0x09) \
\
    X(invalid, 0xff)

#define SENSOR_LIST(X) \
    X(none,     0x00) \
    X(stepper,  0x01) \
    X(arm,      0x02) \
    X(gyro,     0x03) \
    X(coords,   0x04) \
    X(sats,     0x05) \
    X(compass,  0x06) \
    X(accel,    0x07) \
    X(time,     0x08) \
    X(laser,    0x09) \
    X(wind,     0x0a) \
    X(rain,     0x0b) \
    X(humidity, 0x0c) \
    X(pressure, 0x0d) \
    X(voltage,  0x0e) \
    X(current,  0x0f) \
    X(temp0,    0x10) \
    X(temp1,    0x11) \
    X(temp2,    0x12) \
    X(temp3,    0x13) \
    X(temp4,    0x14) \
    X(temp5,    0x15) \
    X(temp6,    0x16) \
    X(temp7,    0x17) \
    X(temp8,    0x18) \
\
    X(invalid, 0xff)

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

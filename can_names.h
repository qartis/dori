#define X(name, value) const char temp_type_ ## name [] = #name;
TYPE_LIST(X)
#undef X

#define X(name, value) const char temp_id_ ## name [] = #name;
ID_LIST(X)
#undef X

#define X(name, value) const char temp_sensor_ ## name [] = #name;
SENSOR_LIST(X)
#undef X


const char unknown_string [] = "???";

const char *type_names [] = {
    [0 ... 255] = unknown_string,
#define X(name, value) [value] = temp_type_ ##name,
    TYPE_LIST(X)
#undef X
};

const char *id_names [] = {
    [0 ... 255] = unknown_string,
#define X(name, value) [value] = temp_id_ ##name,
    ID_LIST(X)
#undef X
};

const char *sensor_names [] = {
    [0 ... 255] = unknown_string,
#define X(name, value) [value] = temp_sensor_ ##name,
    SENSOR_LIST(X)
#undef X
};




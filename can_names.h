#define X(name, value) char temp_type_ ## name [] = #name;
TYPE_LIST(X)
#undef X

#define X(name, value) char temp_id_ ## name [] = #name;
ID_LIST(X)
#undef X


char unknown_string [] = "???";

char *type_names [] = {
    [0 ... 255] = unknown_string,
#define X(name, value) [value] = temp_type_ ##name,
    TYPE_LIST(X)
#undef X
};


char *id_names [] = {
    [0 ... 255] = unknown_string,
#define X(name, value) [value] = temp_id_ ##name,
    ID_LIST(X)
#undef X
};




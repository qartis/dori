#ifndef ___row__h__
#define ___row__h__
#include <vector>

// A single row of columns
class Row {
public:
    std::vector<char*> cols;
    char col_str[512];
};

#endif

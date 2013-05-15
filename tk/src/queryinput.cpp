#include <FL/Fl.H>
#include <FL/Fl_Input.H>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include "queryinput.h"

static const char *defaultQuery = "select rowid, type, a, b, c, site, time from records;";

QueryInput::QueryInput(int x, int y, int w, int h, const char *label) : Fl_Input(x, y, w, h, label), performQuery(NULL), testQuery(NULL)
{
    value(defaultQuery);
}

int QueryInput::getLimit() {
    const char *pos = strstr(value(), " limit ");
    if(pos) {
        pos += strlen(" limit ");
        int limit = atoi(pos);
        return limit;
    }

    return -1;
}


char* QueryInput::getSearchString() {
    static char buf[256];
    strcpy(buf, value());


    return buf;
}

int QueryInput::handle(int event) {
    int key;

    switch(event) {
    case FL_KEYDOWN:
        key = Fl::event_key();
        if (key == FL_Escape) {
            value(defaultQuery);
            performQuery(parent());
            return 1;
        } else {
            if (key == FL_Enter) {
                if(performQuery) {
                    performQuery(parent());
                }

                return 1;
            }

            if(testQuery) {
                Fl_Input::handle(event);
                testQuery(parent());
                return 1;
            }

        }
    default:
        return Fl_Input::handle(event);
    }
}

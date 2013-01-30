#include "queryinput.h"
#include <ctype.h>

static const char *defaultQuery = "select rowid, * from records;";

TableInput::TableInput(int x, int y, int w, int h, const char *label)
    : Fl_Input(x, y, w, h, label), callback(NULL)
{
    value(defaultQuery);
}

void TableInput::performQuery()
{
    if (callback) {
        callback(parent());
    }
}

int TableInput::getLimit() {
    const char *pos = strstr(value(), " limit ");
    if(pos) {
        pos += strlen(" limit ");
        int limit = atoi(pos);
        return limit;
    }

    return -1;
}


char* TableInput::getSearchString() {
    static char buf[256];
    strcpy(buf, value());


    return buf;
}

int TableInput::handle(int event) {
    int key;

    switch(event) {
    case FL_KEYDOWN:
        key = Fl::event_key();

        if (key == FL_Escape) {
            value(defaultQuery);
            performQuery();
            return 1;
        } else { //if (!isalpha(key)) {
            // let Fl_Input deal with the input first so the callback
            // has the up-to-date textfield input
            //int wasLive = isLiveMode();
            Fl_Input::handle(event);
            //if(!wasLive || !isLiveMode()) {
                performQuery();
            return 1;
        }
        /* fall through */

    default:
        return Fl_Input::handle(event);
    }
}

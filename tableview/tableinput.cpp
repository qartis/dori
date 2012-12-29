#include "tableinput.h"
#include <ctype.h>

static const char *defaultQuery = "select * from records";

TableInput::TableInput(int x, int y, int w, int h, const char *label)
    : Fl_Input(x, y, w, h, label), callback(NULL)
{
    value(defaultQuery);
}

void TableInput::performQuery(void)
{
    if (callback)
        callback(this->parent(), (char *)this->value());
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
        } else if (!isalpha(key)) {
            // let Fl_Input deal with the input first so the callback
            // has the up-to-date textfield input
            Fl_Input::handle(event);
            performQuery();
            return 1;
        }
        /* fall through */

    default:
        return Fl_Input::handle(event);
    }
}

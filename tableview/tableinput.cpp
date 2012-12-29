#include "tableinput.h"
#include <FL/fl_draw.H>
#include <math.h>

static char* defaultQuery = "select * from records;";

TableInput::TableInput(int x, int y, int w, int h, const char *label)
    : callback(NULL), Fl_Input(x, y, w, h, label) {
    }

int TableInput::handle(int event) {
    switch(event) {
    case FL_KEYDOWN: {
        int key = Fl::event_key();
        if(key == ' ' || key == ';' || key == FL_BackSpace) {
            // let Fl_Input deal with the input first so the callback
            // has the up-to-date textfield input
            Fl_Input::handle(event);
            if(callback != NULL) {
                callback(this->parent(), (char *)this->value());
            }
            return 1;
        }
    }
    default:
        return Fl_Input::handle(event);
    }
}

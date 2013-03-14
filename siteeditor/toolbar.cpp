#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Pixmap.H>
#include "siteobject.h"
#include "toolbar.h"

static const char *line_xpm[] = {
    "20 20 2 1",
    "* black",
    "# None",
    "**##################",
    "#**#################",
    "##**################",
    "###**###############",
    "####**##############",
    "#####**#############",
    "######**############",
    "#######**###########",
    "########**##########",
    "#########**#########",
    "##########**########",
    "###########**#######",
    "############**######",
    "#############**#####",
    "##############**####",
    "###############**###",
    "################**##",
    "#################**#",
    "##################**",
    "###################*",

};

static Fl_Pixmap line_pixmap(line_xpm);


static const char *rect_xpm[] = {
    "20 20 2 1",
    "* black",
    "# None",
    "********************",
    "********************",
    "**################**",
    "**################**",
    "**################**",
    "**################**",
    "**################**",
    "**################**",
    "**################**",
    "**################**",
    "**################**",
    "**################**",
    "**################**",
    "**################**",
    "**################**",
    "**################**",
    "**################**",
    "**################**",
    "********************",
    "********************",

};

static Fl_Pixmap rect_pixmap(rect_xpm);

static const char *circle_xpm[] = {
    "20 20 2 1",
    "* black",
    "# None",
    "######********######",
    "####*##########*####",
    "###*############*###",
    "##*##############*##",
    "#*################*#",
    "*##################*",
    "*##################*",
    "*##################*",
    "*##################*",
    "*##################*",
    "*##################*",
    "*##################*",
    "*##################*",
    "*##################*",
    "*##################*",
    "#*################*#",
    "##*##############*##",
    "###*############*###",
    "####*##########*####",
    "#####**********#####",

};

static Fl_Pixmap circle_pixmap(circle_xpm);



static void button_cb(Fl_Widget *widget, void *data) {
    Toolbar *toolbar = (Toolbar *)data;
    if(widget == toolbar->lineButton) {
        toolbar->curSelectedObjType = LINE;
    }
    else if(widget == toolbar->rectButton) {
        toolbar->curSelectedObjType = RECT;
    }
    else if(widget == toolbar->circleButton) {
        toolbar->curSelectedObjType = CIRCLE;
    }
}

Toolbar::Toolbar(int x, int y, int w, int h, const char *label) :
  Fl_Window(x, y, w, h, label), lineButton(NULL), rectButton(NULL), circleButton(NULL), curSelectedObjType(UNDEFINED) {
    lineButton = new Fl_Button(0, 0, w, h / 3);
    lineButton->image(line_pixmap);
    lineButton->type(FL_RADIO_BUTTON);
    lineButton->callback(button_cb, this);

    rectButton = new Fl_Button(0, lineButton->h(), w, h / 3);
    rectButton->image(rect_pixmap);
    rectButton->type(FL_RADIO_BUTTON);
    rectButton->callback(button_cb, this);

    circleButton = new Fl_Button(0, rectButton->y() + rectButton->h(), w, h / 3);
    circleButton->image(circle_pixmap);
    circleButton->type(FL_RADIO_BUTTON);
    circleButton->callback(button_cb, this);

    end();
}

int Toolbar::handle(int event) {
    switch(event) {
    case FL_KEYDOWN: {
        int key = Fl::event_key();
        if(key == (FL_F + 1)) {
            if(!shown()) {
                show();
            }
            else {
                hide();
            }
        }
        return 1;
    }
    default:
        return Fl_Window::handle(event);
    }
}

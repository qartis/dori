#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Color_Chooser.H>
#include <string>
#include <sstream>
#include "colorchooser.h"
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

static const char *poly_xpm[] = {
    "20 20 2 1",
    "* black",
    "# None",
    "####################",
    "#####**#############",
    "#######**###########",
    "#########**#########",
    "###########**#######",
    "#############**#####",
    "#############**#####",
    "###########**#######",
    "#########**#########",
    "#######**###########",
    "#####**#############",
    "###**###############",
    "###**###############",
    "#####**#############",
    "#######**###########",
    "#########**#########",
    "###########**#######",
    "#############**#####",
    "####################",
    "####################",

};

static Fl_Pixmap poly_pixmap(poly_xpm);

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
    else if(widget == toolbar->polyButton) {
        toolbar->curSelectedObjType = POLY;
    }

    if(toolbar->user_data() && toolbar->clickedObjTypeCallback) {
        toolbar->clickedObjTypeCallback(toolbar->user_data());
    }
}

Toolbar::Toolbar(int x, int y, int w, int h, const char *label) : Fl_Window(x, y, w, h, label), lineButton(NULL), rectButton(NULL), circleButton(NULL), colorChooser(NULL), curSelectedObjType(UNDEFINED) {
    lineButton = new Fl_Button(0, 0, w / 3, h / 4);
    lineButton->image(line_pixmap);
    lineButton->type(FL_RADIO_BUTTON);
    lineButton->callback(button_cb, this);

    rectButton = new Fl_Button(0, lineButton->h(), lineButton->w(), lineButton->h());
    rectButton->image(rect_pixmap);
    rectButton->type(FL_RADIO_BUTTON);
    rectButton->callback(button_cb, this);

    circleButton = new Fl_Button(0, rectButton->y() + rectButton->h(), lineButton->w(), lineButton->h());
    circleButton->image(circle_pixmap);
    circleButton->type(FL_RADIO_BUTTON);
    circleButton->callback(button_cb, this);

    polyButton = new Fl_Button(0, circleButton->y() + circleButton->h(), lineButton->w(), lineButton->h());
    polyButton->image(poly_pixmap);
    polyButton->type(FL_RADIO_BUTTON);
    polyButton->callback(button_cb, this);

    colorChooser = new ColorChooser(lineButton->w(), 0, w - lineButton->w(), h);
    colorChooser->mode(0);

    user_data(NULL);
    end();
}

void Toolbar::clearSelectedObjectType() {
    curSelectedObjType = UNDEFINED;
    lineButton->value(0);
    rectButton->value(0);
    circleButton->value(0);
    polyButton->value(0);
    circleButton->value(0);
}

int Toolbar::handle(int event) {
    return Fl_Window::handle(event);
}

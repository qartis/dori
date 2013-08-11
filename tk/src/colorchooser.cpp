#include <FL/Fl.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl_Window.H>
#include "colorchooser.h"

ColorChooser::ColorChooser(int x, int y, int w, int h, const char *label) : Fl_Color_Chooser(x, y, w, h, label) {
    // user_data will represent the SiteEditor instance that spawned
    // the toolbar that owns the ColorChooser
    user_data(NULL);
    mousePushedLastHandle = false;
    setColorCallback = NULL;
}

int ColorChooser::handle(int event) {
    if(Fl::pushed() && Fl::event_inside(0, 0, w(), h())) {
        mousePushedLastHandle = true;
    }
    else {
        if(!Fl::pushed() && mousePushedLastHandle)
        {
            // Let the base class set the new color
            // Then we'll pass the new colour to the callback which 
            // lets the SiteEditor know to change the color of the site object
            Fl_Color_Chooser::handle(event);
            if(setColorCallback && user_data()) {
                setColorCallback(user_data(), r(), g(), b());
            }
        }

        mousePushedLastHandle = false;
    }

    return Fl_Color_Chooser::handle(event);
}

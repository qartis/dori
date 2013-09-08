#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl_Tree.H>
#include <FL/gl.h>
#include <math.h>
#include <map>
#include <vector>
#include <limits.h>
#include <sqlite3.h>
#include <algorithm>
#include <string>
#include <sstream>
#include "siteobject.h"
#include "lineobject.h"
#include "rectobject.h"
#include "circleobject.h"
#include "polyobject.h"
#include "colorchooser.h"
#include "toolbar.h"
#include "siteeditor.h"
#include "siteeditorwindow.h"
#include <unistd.h>

#define TOOLBAR_WIDTH 200

SiteEditorWindow::SiteEditorWindow(int x, int y, int w, int h, const char *label) : Fl_Window(x, y, w, h, label), toolbar(NULL) {
    toolbar = new Toolbar(w - TOOLBAR_WIDTH, 0, TOOLBAR_WIDTH, h);
    siteEditor = new SiteEditor(toolbar, 0, 0, w - TOOLBAR_WIDTH, h);
    end();

    toolbar->user_data(siteEditor);
    toolbar->colorChooser->user_data(siteEditor);
    toolbar->colorChooser->setColorCallback = siteEditor->setColorCallback;
    toolbar->clickedObjTypeCallback = siteEditor->clickedObjTypeCallback;
    toolbar->show();
}

SiteEditorWindow::~SiteEditorWindow() {

}

void SiteEditorWindow::resize(int x, int y, int w, int h) {
    siteEditor->resize(0, 0, w - TOOLBAR_WIDTH, h);
    toolbar->resize(w - TOOLBAR_WIDTH, 0, TOOLBAR_WIDTH, h);
}

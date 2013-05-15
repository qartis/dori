#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/gl.h>
#include <math.h>
#include <map>
#include <vector>
#include <limits.h>
#include <sqlite3.h>
#include <algorithm>
#include "siteobject.h"
#include "lineobject.h"
#include "rectobject.h"
#include "circleobject.h"
#include "toolbar.h"
#include "siteeditor.h"

#define CUSTOM_FONT 69
#define SELECTION_DISTANCE 30

static void window_cb(Fl_Widget *widget, void *data) {
    SiteEditor* siteeditor = (SiteEditor*)widget;
    siteeditor->toolbar->hide();
    siteeditor->hide();
}

float SiteEditor::scaledSelectionDistance() {
    int minX = (SELECTION_DISTANCE / maxScreenWidth) * w();
    int minY = (SELECTION_DISTANCE / maxScreenHeight) * h();
    return minX < minY ? minX : minY;
}

int SiteEditor::sqlite_cb(void *arg, int ncols, char**cols, char **colNames) {
    std::vector<SiteObject*>* objs = (std::vector<SiteObject*>*)arg;

    int rowid = atoi(cols[0]);
    objType type = (objType)(atoi(cols[1]));

    SiteObject *obj;

    switch(type) {
    case LINE:
        obj = new LineObject;
        break;
    case RECT:
        obj = new RectObject;
        break;
    case CIRCLE:
        obj = new CircleObject;
        break;
    default:
        fprintf(stderr, "ERROR: undefined object type\n");
    }

    obj->fromString(cols[2]);
    obj->id = rowid;

    objs->push_back(obj);

    return 0;
}

SiteEditor::SiteEditor(int x, int y, int w, int h, const char *label) : Fl_Double_Window(x, y, w, h, label), toolbar(NULL), curMouseOverElevation(0.0), curState(WAITING), curSelectedObject(NULL), newSiteObject(NULL), db(NULL), maxScreenWidth(w), maxScreenHeight(h) {
    processData("../viewport/poland.xyz", "../siteobjects.db");
    Fl::set_font(CUSTOM_FONT, "OCRB");
}

SiteEditor::~SiteEditor() {
    for(unsigned i = 0; i < siteObjects.size(); i++) {
        delete siteObjects[i];
    }

    sqlite3_close(db);
}

SiteObject* SiteEditor::closestGeom(int mouseX, int mouseY, std::vector<SiteObject*>& dataset, int *distance, bool clearColours) {
    SiteObject *closest = NULL;
    int square, minSquare;
    minSquare = INT_MAX;

    for(unsigned i = 0; i < dataset.size(); i++) {
        int scaledScreenCenterX = dataset[i]->scaledScreenCenterX(w(), maxScreenWidth);
        int scaledScreenCenterY = dataset[i]->scaledScreenCenterY(h(), maxScreenHeight);

        square = ((mouseX - scaledScreenCenterX) * (mouseX - scaledScreenCenterX)) + ((mouseY - scaledScreenCenterY) * (mouseY - scaledScreenCenterY));

        if(square < minSquare) {
            minSquare = square;
            closest = dataset[i];
        }
    }

    if(distance) {
        *distance = sqrt(minSquare);
    }

    return closest;
}

void SiteEditor::processData(const char * filename, const char * db_name) {
    minX = minY = minElevation = INT_MAX;
    maxX = maxY = maxElevation = 0;

    FILE *f = fopen(filename, "r");

    char buf[512];

    if(f == NULL) {
        printf("file not found\n");
        return;
    }

    float curX, curY, curElevation;
    while(fgets(buf, sizeof(buf), f)) {
        if(buf[0] == '#') {
            continue;
        }
        sscanf(buf, "%f %f %f", &curX, &curY, &curElevation);

        if(curX < minX) minX = curX;
        if(curY < minY) minY = curY;
        if(curElevation < minElevation) minElevation = curElevation;

        if(curX > maxX) maxX = curX;
        if(curY > maxY) maxY = curY;
        if(curElevation > maxElevation) maxElevation = curElevation;

        CircleObject *newPoint = new CircleObject;
        newPoint->worldCenterX = curX;
        newPoint->worldCenterY = curY;
        newPoint->screenRadius = 6;
        newPoint->elevation = curElevation;
        newPoint->worldHeight = 1;
        points.push_back(newPoint);
    }

    double scaleColour = 6.0 / (maxElevation - minElevation);

    scaleX = w() / (maxX - minX);
    scaleY = h() / (maxY - minY);

    for(unsigned i = 0; i < points.size(); i++) {
        double r, g, b;
        CircleObject *point = (CircleObject *)points[i];
        Fl_Color_Chooser::hsv2rgb(scaleColour * (points[i]->elevation - minElevation), 1.0, 0.6, r, g, b);

        point->r = r * 255.0;
        point->g = g * 255.0;
        point->b = b * 255.0;

        strcpy(points[i]->special, "filled");
        point->screenCenterX = (int)(scaleX * (point->worldCenterX - minX));
        point->screenCenterY = h() - (int)(scaleY * (point->worldCenterY - minY));
    }


    if(!db) {
        sqlite3_open(db_name, &db);
    }

    const char* query = "SELECT rowid, * FROM objects;";
    int ret = sqlite3_exec(db, query, sqlite_cb, &siteObjects, NULL);
    if (ret != SQLITE_OK)
    {
        fprintf(stderr, "error executing query\n");
    }

    end();

    toolbar = new Toolbar(h(), 0, 200, 200);
    toolbar->show();

    callback(window_cb);

    redraw();
}

int SiteEditor::handle(int event) {
    if(Fl::event_button()) {
        int unscaledMouseX = (Fl::event_x_root() - x()) * (maxScreenWidth / w());
        int unscaledMouseY = (Fl::event_y_root() - y()) * (maxScreenHeight / h());

        if(event == FL_PUSH) {
            int distance = 0;

            curSelectedObject = closestGeom(Fl::event_x_root() - x(), Fl::event_y_root() - y(), siteObjects, &distance, true);

            if(distance > scaledSelectionDistance()) {
                curSelectedObject = NULL;
                redraw();
            }

            if(curSelectedObject && distance < scaledSelectionDistance()) {
                curState = SELECTED;
                redraw();
            }
            else {
                if(toolbar->curSelectedObjType != UNDEFINED) {
                    curState = DRAWING;
                }

                if(toolbar->curSelectedObjType == LINE) {
                    newSiteObject = new LineObject;
                    LineObject *newLineObject = (LineObject*)newSiteObject;

                    newLineObject->screenLeft = unscaledMouseX;//Fl::event_x_root() - x();
                    newLineObject->screenTop = unscaledMouseY;//Fl::event_y_root() - y();
                }
                else if(toolbar->curSelectedObjType == RECT) {
                    newSiteObject = new RectObject;
                    RectObject *newRectObject = (RectObject*)newSiteObject;

                    newRectObject->screenLeft = unscaledMouseX;
                    newRectObject->screenTop =  unscaledMouseY;
                }
                else if(toolbar->curSelectedObjType == CIRCLE) {
                    newSiteObject = new CircleObject;
                    CircleObject *newCircleObject = (CircleObject*)newSiteObject;

                    newCircleObject->screenCenterX = unscaledMouseX;
                    newCircleObject->screenCenterY = unscaledMouseY;
                }

                if(newSiteObject) {
                    newSiteObject->r = toolbar->colorChooser->r() * 255.0;
                    newSiteObject->g = toolbar->colorChooser->g() * 255.0;
                    newSiteObject->b = toolbar->colorChooser->b() * 255.0;
                }

                redraw();
            }
        }
        else if(event == FL_DRAG) {
            if(curState == DRAWING) {
                if(toolbar->curSelectedObjType == LINE) {
                    LineObject *newLineObject = (LineObject*)newSiteObject;
                    newLineObject->screenLengthX = unscaledMouseX - newLineObject->screenLeft;
                    newLineObject->screenLengthY = unscaledMouseY - newLineObject->screenTop;

                    newLineObject->screenCenterX = newLineObject->screenLeft + (newLineObject->screenLengthX / 2);
                    newLineObject->screenCenterY = newLineObject->screenTop + (newLineObject->screenLengthY / 2);
                }
                else if(toolbar->curSelectedObjType == RECT) {
                    RectObject *newRectObject = (RectObject*)newSiteObject;
                    newRectObject->screenWidth = unscaledMouseX - newRectObject->screenLeft;
                    newRectObject->screenHeight = unscaledMouseY - newRectObject->screenTop;

                    newRectObject->screenCenterX = newRectObject->screenLeft + (newRectObject->screenWidth / 2);
                    newRectObject->screenCenterY = newRectObject->screenTop + (newRectObject->screenHeight / 2);
                }
                else if(toolbar->curSelectedObjType == CIRCLE) {
                    CircleObject *newCircleObject = (CircleObject*)newSiteObject;
                    int distX = abs(unscaledMouseX - newCircleObject->screenCenterX);
                    int distY = abs(unscaledMouseY - newCircleObject->screenCenterY);
                    newCircleObject->screenRadius = distX > distY ? distX : distY;
                }

                SiteObject *closest = closestGeom(Fl::event_x_root() - x(), Fl::event_y_root() - y(), (std::vector<SiteObject*> &)(points));
                if(closest) {
                    curMouseOverElevation = closest->elevation;
                }

                redraw();
            }
            else if(curState == SELECTED && curSelectedObject) {
                SiteObject *closest = closestGeom(Fl::event_x_root() - x(), Fl::event_y_root() - y(), points);
                if(closest) {
                    curMouseOverElevation = closest->elevation;
                }

                curSelectedObject->moveCenter(unscaledMouseX, unscaledMouseY);
                redraw();
            }
        }
        else if(event == FL_RELEASE) {
            if(curState == DRAWING) {
                curState = WAITING;

                redraw();

                newSiteObject->calcWorldCoords(scaleX, scaleY, minX, minY);

                char data[512];
                char query[512];
                newSiteObject->toString(data);

                sprintf(query, "INSERT INTO objects (type, data) VALUES (%d, \"%s\");", newSiteObject->type, data);
                int rc = sqlite3_exec(db, query, NULL, NULL, NULL);
                if(SQLITE_OK != rc) {
                    fprintf(stderr, "Couldn't insert object entry into db\n");
                }

                sqlite_int64 rowid = sqlite3_last_insert_rowid(db);
                newSiteObject->id = rowid;
                siteObjects.push_back(newSiteObject);

                newSiteObject = NULL;
            }
            else if(curState == SELECTED && curSelectedObject) {
                curSelectedObject->calcWorldCoords(scaleX, scaleY, minX, minY);

                char data[512];
                char query[512];
                curSelectedObject->toString(data);
                sprintf(query, "UPDATE objects SET data = \"%s\" WHERE rowid = %d", data, curSelectedObject->id);

                int rc = sqlite3_exec(db, query, NULL, NULL, NULL);
                if(SQLITE_OK != rc) {
                    fprintf(stderr, "Couldn't update object entry in db\n");
                }
            }
        }
        else {
            SiteObject *closest = closestGeom(Fl::event_x_root() - x(), Fl::event_y_root() - y(), points);
            if(closest) {
                curMouseOverElevation = closest->elevation;
                redraw(); //TODO: optimize this to only update the text, not redraw everything
            }
        }
    }

    if(event == FL_KEYDOWN) {
        int key = Fl::event_key();

        if(key == FL_Delete && curState == SELECTED) {
            char query[512];
            sprintf(query, "DELETE FROM objects WHERE rowid = %d;", curSelectedObject->id);
            sqlite3_exec(db, query, NULL, NULL, NULL);

            std::vector<SiteObject*>::iterator it = siteObjects.begin();
            for(; it != siteObjects.end(); it++) {
                if((*it)->id == curSelectedObject->id) {
                    curSelectedObject = NULL;
                    siteObjects.erase(it);
                    break;
                }
            }

            curState = WAITING;
            redraw();
            return 1;
        }
        else if(key == (FL_F + 1)) {
            if(!toolbar->shown()) {
                toolbar->show();
            }
            else {
                toolbar->hide();
            }

            return 1;
        }
        else if(key == FL_Escape) {
            curSelectedObject = NULL;
            curState = WAITING;
            toolbar->curSelectedObjType = UNDEFINED;
            toolbar->lineButton->value(0);
            toolbar->rectButton->value(0);
            toolbar->circleButton->value(0);
            redraw();
            return 1;
        }
        else {
            return Fl_Window::handle(event);
        }

    }

    return Fl_Window::handle(event);
}

void SiteEditor::draw() {
    fl_draw_box(FL_FLAT_BOX, 0, 0, w(), h(), fl_rgb_color(25, 25, 25));

    fl_line_style(0,2);

    for(unsigned i = 0; i < points.size(); i++) {
        points[i]->drawScreen(false, w(), maxScreenWidth, h(), maxScreenHeight);
    }

    fl_color(FL_WHITE);
    fl_line_style(FL_SOLID, 5);

    // draw DORI
    fl_draw_box(FL_FLAT_BOX, (293.0 / maxScreenWidth) * w(), (295.0 / maxScreenHeight) * h(), (13.0 / maxScreenWidth) * w(), (11.0 / maxScreenHeight) * h(), FL_BLACK);
    fl_draw_box(FL_FLAT_BOX, (295.0 / maxScreenWidth) * w(), (297.0 / maxScreenHeight) * h(), (10.0 / maxScreenWidth) * w(), (8.0 / maxScreenHeight) * h(), FL_RED);

    char textBuffer[256];
    int textHeight = 0, textWidth = 0;

    for(unsigned i = 0; i < siteObjects.size(); i++) {
        if(curSelectedObject == siteObjects[i]) {
            unsigned oldR, oldG, oldB;
            oldR = curSelectedObject->r;
            oldG = curSelectedObject->g;
            oldB = curSelectedObject->b;

            curSelectedObject->r = 255;
            curSelectedObject->g = 0;
            curSelectedObject->b = 0;

            curSelectedObject->drawScreen(true, w(), maxScreenWidth, h(), maxScreenHeight);

            curSelectedObject->r = oldR;
            curSelectedObject->g = oldG;
            curSelectedObject->b = oldB;
        }
        else
        {
            siteObjects[i]->drawScreen(true, w(), maxScreenWidth, h(), maxScreenHeight);
        }
        /*
        SiteObject *closest = closestGeom(objectScaledX, objectScaledY, points, &unused);
        if(closest) {
            fl_line_style(0);
            sprintf(textBuffer, "%f", closest->elevation);
            fl_color(FL_WHITE);
            fl_font(CUSTOM_FONT, 8);
            fl_measure(textBuffer, textWidth, textHeight, 0);
            fl_draw(textBuffer, objectScaledX - (textWidth / 2.0), objectScaledY);
        }
        */
    }

    if(curState == DRAWING) {
        if(newSiteObject) {
            newSiteObject->drawScreen(true, w(), maxScreenWidth, h(), maxScreenHeight);
        }
    }

    fl_line_style(0);
    sprintf(textBuffer, "CLOSEST PT ELEV %f", curMouseOverElevation);
    fl_color(FL_WHITE);
    fl_font(CUSTOM_FONT, 12);
    fl_measure(textBuffer, textWidth, textHeight, 0);
    fl_draw(textBuffer, 5, textHeight);
}

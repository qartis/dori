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

unsigned long geometrySize(int type) {
    switch(type) {
    case LINE:
        return sizeof(LineObject);
    case RECT:
        return sizeof(RectObject);
    case CIRCLE:
        return sizeof(CircleObject);
    default:
        return sizeof(SiteObject);
    }
}


static void window_cb(Fl_Widget *widget, void *data) {
    SiteEditor* siteeditor = (SiteEditor*)widget;
    siteeditor->toolbar->hide();
    siteeditor->hide();
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

SiteEditor::SiteEditor(int x, int y, int w, int h, const char *label) : Fl_Double_Window(x, y, w, h, label), toolbar(NULL), curMouseOverElevation(0.0), curState(WAITING), curSelectedObject(NULL), newSiteObject(NULL) {
    processData("../viewport/poland.xyz");
    Fl::set_font(CUSTOM_FONT, "OCRB");

    sqlite3_open("../siteobjects.db", &db);

    if(db) {
        const char* query = "SELECT rowid, * FROM objects;";
        int ret = sqlite3_exec(db, query, sqlite_cb, &siteObjects, NULL);
        if (ret != SQLITE_OK)
        {
            fprintf(stderr, "error executing query\n");
        }

        redraw();
        end();

        toolbar = new Toolbar(h, 0, 60, 200);
        toolbar->show();

        callback(window_cb);
    }
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
        if(clearColours) {
            //TODO: change this once we add color picker
            dataset[i]->r = 255;
            dataset[i]->g = 255;
            dataset[i]->b = 255;
        }

        square = ((mouseX - dataset[i]->screenCenterX) * (mouseX - dataset[i]->screenCenterX)) + ((mouseY - dataset[i]->screenCenterY) * (mouseY - dataset[i]->screenCenterY));
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

void SiteEditor::processData(const char * filename) {
    float maxX, maxY, maxElevation;

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

    scaleX = w() / (maxX - minX);
    scaleY = h() / (maxY - minY);

    double scaleColour = 6.0 / (maxElevation - minElevation);

    for(unsigned i = 0; i < points.size(); i++) {
        double r, g, b;
        CircleObject *point = (CircleObject *)points[i];
        Fl_Color_Chooser::hsv2rgb(scaleColour * (points[i]->elevation - minElevation), 1.0, 3.0, r, g, b);

        point->r = r * 255.0;
        point->g = g * 255.0;
        point->b = b * 255.0;

        point->screenCenterX = (int)(scaleX * (point->worldCenterX - minX));
        point->screenCenterY = h() - (int)(scaleY * (point->worldCenterY - minY));
    }

    redraw();
}

int SiteEditor::handle(int event) {
    if(Fl::event_button()) {
        if(event == FL_PUSH) {
            int distance = 0;

            curSelectedObject = closestGeom(Fl::event_x_root() - x(), Fl::event_y_root() - y(), siteObjects, &distance, false);

            if(distance > SELECTION_DISTANCE) {
                curSelectedObject = NULL;
                redraw();
            }

            if(curSelectedObject && distance < SELECTION_DISTANCE) {
                curState = SELECTED;
                //TODO: change this once we add color picker
                curSelectedObject->r = 255;
                curSelectedObject->g = 0;
                curSelectedObject->b = 0;
                redraw();
            }
            else {
                if(toolbar->curSelectedObjType != UNDEFINED) {
                    curState = DRAWING;
                }

                if(toolbar->curSelectedObjType == LINE) {
                    newSiteObject = new LineObject;
                    LineObject *newLineObject = (LineObject*)newSiteObject;
                    //TODO: change this once we add color picker
                    newLineObject->r = rand() % 255;
                    newLineObject->g = rand() % 255;
                    newLineObject->b = rand() % 255;
                    newLineObject->screenLeft = Fl::event_x_root() - x();
                    newLineObject->screenTop = Fl::event_y_root() - y();
                }
                else if(toolbar->curSelectedObjType == RECT) {
                    newSiteObject = new RectObject;
                    RectObject *newRectObject = (RectObject*)newSiteObject;
                    //TODO: change this once we add color picker
                    newRectObject->r = rand() % 255;
                    newRectObject->g = rand() % 255;
                    newRectObject->b = rand() % 255;
                    newRectObject->screenLeft = Fl::event_x_root() - x();
                    newRectObject->screenTop =  Fl::event_y_root() - y();
                }
                else if(toolbar->curSelectedObjType == CIRCLE) {
                    newSiteObject = new CircleObject;
                    CircleObject *newCircleObject = (CircleObject*)newSiteObject;
                    //TODO: change this once we add color picker
                    newCircleObject->r = rand() % 255;
                    newCircleObject->g = rand() % 255;
                    newCircleObject->b = rand() % 255;
                    newCircleObject->screenCenterX = Fl::event_x_root() - x();
                    newCircleObject->screenCenterY = Fl::event_y_root() - y();
                }

                redraw();
            }
        }
        else if(event == FL_DRAG) {
            if(curState == DRAWING) {
                if(toolbar->curSelectedObjType == LINE) {
                    LineObject *newLineObject = (LineObject*)newSiteObject;
                    newLineObject->screenLengthX = (Fl::event_x_root() - x()) - newLineObject->screenLeft;
                    newLineObject->screenLengthY = (Fl::event_y_root() - y()) - newLineObject->screenTop;

                    newLineObject->screenCenterX = newLineObject->screenLeft + (newLineObject->screenLengthX / 2);
                    newLineObject->screenCenterY = newLineObject->screenTop + (newLineObject->screenLengthY / 2);
                }
                else if(toolbar->curSelectedObjType == RECT) {
                    RectObject *newRectObject = (RectObject*)newSiteObject;
                    newRectObject->screenWidth = (Fl::event_x_root() - x()) - newRectObject->screenLeft;
                    newRectObject->screenHeight = (Fl::event_y_root() - y()) - newRectObject->screenTop;

                    newRectObject->screenCenterX = newRectObject->screenLeft + (newRectObject->screenWidth / 2);
                    newRectObject->screenCenterY = newRectObject->screenTop + (newRectObject->screenHeight / 2);
                }
                else if(toolbar->curSelectedObjType == CIRCLE) {
                    CircleObject *newCircleObject = (CircleObject*)newSiteObject;
                    int distX = abs((Fl::event_x_root() - x()) - newCircleObject->screenCenterX);
                    int distY = abs((Fl::event_y_root() - y()) - newCircleObject->screenCenterY);
                    newCircleObject->screenRadius = distX > distY ? distX : distY;
                }


                SiteObject *closest = closestGeom(Fl::event_x_root() - x(), Fl::event_y_root() - y(), (std::vector<SiteObject*> &)(points));
                if(closest) {
                    curMouseOverElevation = closest->elevation;
                }

                redraw();
            }
            else if(curState == SELECTED && curSelectedObject) {
                curSelectedObject->moveCenter((Fl::event_x_root() - x()), (Fl::event_y_root() - y()));

                SiteObject *closest = closestGeom(Fl::event_x_root() - x(), Fl::event_y_root() - y(), points);
                if(closest) {
                    curMouseOverElevation = closest->elevation;
                }

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

                //TODO: change this once we add color picker
                curSelectedObject->r = 255;
                curSelectedObject->g = 255;
                curSelectedObject->b = 255;

                char data[512];
                char query[512];
                curSelectedObject->toString(data);
                sprintf(query, "UPDATE objects SET data = \"%s\" WHERE rowid = %d", data, curSelectedObject->id);

                int rc = sqlite3_exec(db, query, NULL, NULL, NULL);
                if(SQLITE_OK != rc) {
                    fprintf(stderr, "Couldn't update object entry in db\n");
                }

                //TODO: change this once we add color picker
                curSelectedObject->r = 255;
                curSelectedObject->g = 0;
                curSelectedObject->b = 0;
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
    }

    return Fl_Window::handle(event);
}

void SiteEditor::draw() {
    fl_draw_box(FL_FLAT_BOX, 0, 0, w(), h(), fl_rgb_color(25, 25, 25));

    fl_line_style(0,2);

    for(unsigned i = 0; i < points.size(); i++) {
        points[i]->drawScreen(false);
    }

    fl_color(FL_WHITE);
    fl_line_style(FL_SOLID, 5);

    for(unsigned i = 0; i < siteObjects.size(); i++) {
        siteObjects[i]->drawScreen();
    }

    if(curState == DRAWING) {
        if(newSiteObject) {
            newSiteObject->drawScreen();
        }
    }

    fl_line_style(0);

    char textBuffer[256];
    int textHeight = 0, textWidth = 0;
    sprintf(textBuffer, "CLOSEST PT ELEV %f", curMouseOverElevation);
    fl_color(FL_WHITE);
    fl_font(CUSTOM_FONT, 12);
    fl_measure(textBuffer, textWidth, textHeight, 0);
    fl_draw(textBuffer, 5, textHeight);
}

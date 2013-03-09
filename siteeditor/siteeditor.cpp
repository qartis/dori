#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>
#include <math.h>
#include <map>
#include <vector>
#include <limits.h>
#include <sqlite3.h>
#include <algorithm>
#include "geometry.h"
#include "line.h"
#include "rect.h"
#include "circle.h"
#include "toolbar.h"
#include "siteobject.h"
#include "siteeditor.h"

#define CUSTOM_FONT 69
#define SELECTION_DISTANCE 20

unsigned long geometrySize(int type) {
    switch(type) {
    case LINE:
        return sizeof(Line);
    case RECT:
        return sizeof(Rect);
    case CIRCLE:
        return sizeof(Circle);
    default:
        return sizeof(Geometry);
    }
}

SiteEditor::SiteEditor(int x, int y, int w, int h, const char *label)
    : Fl_Double_Window(x, y, w, h, label),
    curMouseOverElevation(0.0),
    curState(WAITING), curSelectedGeom(NULL),
    toolbar(NULL), newGeom(NULL) {

        processData("../viewport/poland.xyz");
        Fl::set_font(CUSTOM_FONT, "OCRB");

        sqlite3_open("../siteobjects.db", &db);
        if(db) {
            sqlite3_stmt* res = NULL;
            const char* query = "SELECT rowid, * FROM objects;";
            int ret = sqlite3_prepare_v2 (db, query, strlen(query), &res, 0);

            if (SQLITE_OK == ret)
            {
                while (SQLITE_ROW == sqlite3_step(res))
                {
                    int rowid = (int)sqlite3_column_int(res, 0);
                    int type = (int)sqlite3_column_int(res, 1);
                    Geometry *newGeom;

                    switch(type) {
                    case LINE:
                        newGeom = new Line;
                        memcpy(newGeom, (Line*)sqlite3_column_blob(res, 2), sizeof(Line));
                        break;
                    case RECT:
                        newGeom = new Rect;
                        memcpy(newGeom, (Rect*)sqlite3_column_blob(res, 2), sizeof(Rect));
                        break;
                    case CIRCLE:
                        newGeom = new Circle;
                        memcpy(newGeom, (Circle*)sqlite3_column_blob(res, 2), sizeof(Circle));
                        break;
                    }

                    newGeom->id = rowid;

                    siteGeoms.push_back(newGeom);
                }
            }
            sqlite3_finalize(res);
            redraw();
        }
        else {
            fprintf(stderr, "couldn't find db\n");
        }


        end();

        toolbar = new Toolbar(h, 0, 60, 200);
        toolbar->show();
    }

SiteEditor::~SiteEditor() {
    for(unsigned i = 0; i < siteGeoms.size(); i++) {
        delete siteGeoms[i];
    }

    sqlite3_close(db);
}

Geometry* SiteEditor::closestGeom(int mouseX, int mouseY, std::vector<Geometry*>& dataset, int *distance, bool clearColours) {
    Geometry *closest = NULL;
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

        Circle *newPoint = new Circle;
        newPoint->worldCenterX = curX;
        newPoint->worldCenterY = curY;
        newPoint->screenRadius = 1;
        newPoint->elevation = curElevation;
        newPoint->worldHeight = 1;
        points.push_back(newPoint);
    }

    scaleX = w() / (maxX - minX);
    scaleY = h() / (maxY - minY);

    double scaleColour = 6.0 / (maxElevation - minElevation);

    for(unsigned i = 0; i < points.size(); i++) {
        double r, g, b;
        Circle *point = (Circle *)points[i];
        Fl_Color_Chooser::hsv2rgb(scaleColour * (points[i]->elevation - minElevation), 1.0, 1.0, r, g, b);

        //TODO: change this once we add color picker
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

            curSelectedGeom = closestGeom(Fl::event_x_root() - x(), Fl::event_y_root() - y(), siteGeoms, &distance, true);

            if(distance > SELECTION_DISTANCE) {
                curSelectedGeom = NULL;
                redraw();
            }

            if(curSelectedGeom && distance < SELECTION_DISTANCE) {
                curState = SELECTED;
                //TODO: change this once we add color picker
                curSelectedGeom->r = 255;
                curSelectedGeom->g = 0;
                curSelectedGeom->b = 0;
                redraw();
            }
            else {
                if(toolbar->curSelectedType != NONE) {
                    curState = DRAWING;
                }

                if(toolbar->curSelectedType == LINE) {
                    newGeom = new Line;
                    Line *newLine = (Line*)newGeom;
                    //TODO: change this once we add color picker
                    newLine->r = 255;
                    newLine->g = 255;
                    newLine->b = 255;
                    newLine->screenLeft = Fl::event_x_root() - x();
                    newLine->screenTop = Fl::event_y_root() - y();
                }
                else if(toolbar->curSelectedType == RECT) {
                    newGeom = new Rect;
                    Rect *newRect = (Rect*)newGeom;
                    //TODO: change this once we add color picker
                    newRect->r = 255;
                    newRect->g = 255;
                    newRect->b = 255;
                    newRect->screenLeft = Fl::event_x_root() - x();
                    newRect->screenTop =  Fl::event_y_root() - y();
                }
                else if(toolbar->curSelectedType == CIRCLE) {
                    newGeom = new Circle;
                    Circle *newCircle = (Circle*)newGeom;
                    //TODO: change this once we add color picker
                    newCircle->r = 255;
                    newCircle->g = 255;
                    newCircle->b = 255;
                    newCircle->screenCenterX = Fl::event_x_root() - x();
                    newCircle->screenCenterY = Fl::event_y_root() - y();
                }

                redraw();
            }
        }
        else if(event == FL_DRAG) {
            if(curState == DRAWING) {
                if(toolbar->curSelectedType == LINE) {
                    Line *newLine = (Line*)newGeom;
                    newLine->screenLengthX = (Fl::event_x_root() - x()) - newLine->screenLeft;
                    newLine->screenLengthY = (Fl::event_y_root() - y()) - newLine->screenTop;

                    newLine->screenCenterX = newLine->screenLeft + (newLine->screenLengthX / 2);
                    newLine->screenCenterY = newLine->screenTop + (newLine->screenLengthY / 2);
                }
                else if(toolbar->curSelectedType == RECT) {
                    Rect *newRect = (Rect*)newGeom;
                    newRect->screenWidth = (Fl::event_x_root() - x()) - newRect->screenLeft;
                    newRect->screenHeight = (Fl::event_y_root() - y()) - newRect->screenTop;

                    newRect->screenCenterX = newRect->screenLeft + (newRect->screenWidth / 2);
                    newRect->screenCenterY = newRect->screenTop + (newRect->screenHeight / 2);
                }
                else if(toolbar->curSelectedType == CIRCLE) {
                    Circle *newCircle = (Circle*)newGeom;
                    int distX = abs((Fl::event_x_root() - x()) - newCircle->screenCenterX);
                    int distY = abs((Fl::event_y_root() - y()) - newCircle->screenCenterY);
                    newCircle->screenRadius = distX > distY ? distX : distY;
                }


                Geometry *closest = closestGeom(Fl::event_x_root() - x(), Fl::event_y_root() - y(), (std::vector<Geometry*> &)(points));
                if(closest) {
                    curMouseOverElevation = closest->elevation;
                }

                redraw();
            }
            else if(curState == SELECTED) {
                if(curSelectedGeom) {
                    SiteObject::moveCenter(curSelectedGeom, (Fl::event_x_root() - x()), (Fl::event_y_root() - y()));

                    Geometry *closest = closestGeom(Fl::event_x_root() - x(), Fl::event_y_root() - y(), points);
                    if(closest) {
                        curMouseOverElevation = closest->elevation;
                    }

                    redraw();
                }
            }
        }
        else if(event == FL_RELEASE) {
            if(curState == DRAWING) {
                curState = WAITING;
                redraw();

                unsigned long blobSize = geometrySize(newGeom->type);
                sqlite3_stmt* preparedStatement = NULL;
                const char* unused;

                char query[512];
                sprintf(query, "INSERT INTO objects (type, data) VALUES (%d, ?);", newGeom->type);
                sqlite3_prepare_v2(db, query, -1, &preparedStatement, &unused);
                sqlite3_bind_blob (preparedStatement, 1, newGeom, blobSize, SQLITE_STATIC);
                sqlite3_step (preparedStatement);
                sqlite3_finalize (preparedStatement);

                sqlite_int64 rowid = sqlite3_last_insert_rowid(db);
                newGeom->id = rowid;

                siteGeoms.push_back(newGeom);

                newGeom = NULL;
            }
            else if(curState == SELECTED) {
                sqlite3_blob *blob;

                int rc = sqlite3_blob_open(db, "main", "objects", "data", curSelectedGeom->id, 1, &blob);
                if(SQLITE_OK != rc) {
                    fprintf(stderr, "Couldn't get blob handle (%i): %s\n", rc, sqlite3_errmsg(db));
                    exit(1);
                }

                //TODO: change this once we add color picker
                curSelectedGeom->r = 255;
                curSelectedGeom->g = 255;
                curSelectedGeom->b = 255;

                if(SQLITE_OK != (rc = sqlite3_blob_write(blob, curSelectedGeom, geometrySize(curSelectedGeom->type), 0))) {
                    fprintf(stderr, "Error writing to blob handle\n");
                    exit(1);
                }

                sqlite3_blob_close(blob);

                //TODO: change this once we add color picker
                curSelectedGeom->r = 255;
                curSelectedGeom->g = 0;
                curSelectedGeom->b = 0;
            }
        }
        else {
            Geometry *closest = closestGeom(Fl::event_x_root() - x(), Fl::event_y_root() - y(), points);
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
            sprintf(query, "DELETE FROM objects WHERE rowid = %d;", curSelectedGeom->id);
            sqlite3_exec(db, query, NULL, NULL, NULL);

            std::vector<Geometry*>::iterator it = siteGeoms.begin();
            for(; it != siteGeoms.end(); it++) {
                if((*it)->id == curSelectedGeom->id) {
                    curSelectedGeom = NULL;
                    siteGeoms.erase(it);
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
    fl_draw_box(FL_FLAT_BOX, 0, 0, w(), h(), FL_BLACK);

    for(unsigned i = 0; i < points.size(); i++) {
        SiteObject::draw(points[i], false);
    }

    fl_color(FL_WHITE);
    fl_line_style(FL_SOLID, 5);

    for(unsigned i = 0; i < siteGeoms.size(); i++) {
        SiteObject::draw(siteGeoms[i]);
    }

    if(curState == DRAWING) {
        if(newGeom) {
            SiteObject::draw(newGeom);
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

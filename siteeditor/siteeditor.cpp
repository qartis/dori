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
#include "siteeditor.h"

#define CUSTOM_FONT 69
#define SELECTION_DISTANCE 20

int SiteEditor::sqlite_cb(void *arg, int ncols, char **cols, char **colNames) {
    (void)ncols;
    (void)colNames;
    SiteEditor * siteEditor = (SiteEditor*)arg;

    point newSiteObject;

    newSiteObject.id = atoi(cols[0]);

    newSiteObject.screenX = atoi(cols[1]);
    newSiteObject.screenY = atoi(cols[2]);

    newSiteObject.screenWidth = atoi(cols[3]);
    newSiteObject.screenHeight = atoi(cols[4]);

    newSiteObject.worldX = atof(cols[5]);
    newSiteObject.worldY = atof(cols[6]);

    newSiteObject.worldWidth = atof(cols[7]);
    newSiteObject.worldHeight = atof(cols[8]);

    newSiteObject.elevation = atof(cols[9]);

    newSiteObject.r = atoi(cols[10]);
    newSiteObject.g = atoi(cols[11]);
    newSiteObject.b = atoi(cols[12]);

    newSiteObject.screenCenterX = newSiteObject.screenX + (newSiteObject.screenWidth / 2);
    newSiteObject.screenCenterY = newSiteObject.screenY + (newSiteObject.screenHeight / 2);


    siteEditor->siteObjects.push_back(newSiteObject);

    return 0;
}

SiteEditor::SiteEditor(int x, int y, int w, int h, const char *label)
    : Fl_Double_Window(x, y, w, h, label), newSquareOriginX(0), newSquareOriginY(0),
    newSquareCenterX(0), newSquareCenterY(0) ,
    newSquareWidth(0), newSquareHeight(0),
    newSquareR(255), newSquareG(255), newSquareB(255),
    curMouseOverElevation(0.0),
    curState(WAITING), curSelectedSquare(NULL) {

    processData("../viewport/poland.xyz");
    Fl::set_font(CUSTOM_FONT, "OCRB");

    sqlite3_open("../siteobjects.db", &db);
    if(db) {
        fprintf(stderr, "reading from db\n");
        sqlite3_exec(db, "select rowid, * from objects;", sqlite_cb, this, NULL); 
        redraw();
    }
    else {
        fprintf(stderr, "couldn't find db\n");
    }
}

SiteEditor::~SiteEditor() {
    sqlite3_close(db);
}

point* SiteEditor::closestPoint(int mouseX, int mouseY, std::vector<point>& dataset, int *distance, bool clearColours) {
    point *closest = NULL;
    int square, minSquare;
    minSquare = INT_MAX;

    for(unsigned i = 0; i < dataset.size(); i++) {
        if(clearColours) {
            dataset[i].r = 255;
            dataset[i].g = 255;
            dataset[i].b = 255;
        }

        square = ((mouseX - dataset[i].screenCenterX) * (mouseX - dataset[i].screenCenterX)) + ((mouseY - dataset[i].screenCenterY) * (mouseY - dataset[i].screenCenterY));
        if(square < minSquare) {
            minSquare = square;
            closest = &dataset[i];
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

        point newPoint;
        newPoint.worldX = curX;
        newPoint.worldY = curY;
        newPoint.elevation = curElevation;
        points.push_back(newPoint);
    }

    scaleX = w() / (maxX - minX);
    scaleY = h() / (maxY - minY);

    double scaleColour = 6.0 / (maxElevation - minElevation);

    for(unsigned i = 0; i < points.size(); i++) {
        double r, g, b;
        Fl_Color_Chooser::hsv2rgb(scaleColour * (points[i].elevation - minElevation), 1.0, 1.0, r, g, b);

        points[i].r = r * 255.0;
        points[i].g = g * 255.0;
        points[i].b = b * 255.0;

        points[i].screenX = (int)(scaleX * (points[i].worldX - minX));
        points[i].screenY = h() - (int)(scaleY * (points[i].worldY - minY));

        points[i].screenCenterX = points[i].screenX;
        points[i].screenCenterY = points[i].screenY;

        points[i].screenWidth = points[i].screenHeight = 1;
        points[i].worldWidth = points[i].worldHeight = 1;
    }

    redraw();
}

int SiteEditor::handle(int event) {
    if(Fl::event_button()) {
        if(event == FL_PUSH) {
            int distance = 0;

            curSelectedSquare = closestPoint(Fl::event_x_root() - x(), Fl::event_y_root() - y(), siteObjects, &distance, true);

            if(curSelectedSquare && distance < SELECTION_DISTANCE) {
                curState = SELECTED;
                curSelectedSquare->r = 255;
                curSelectedSquare->g = 0;
                curSelectedSquare->b = 0;
                redraw();
            }
            else {
                curState = DRAWING;
                newSquareOriginX = Fl::event_x_root() - x();
                newSquareOriginY = Fl::event_y_root() - y();
            }
        }
        else if(event == FL_DRAG) {
            if(curState == DRAWING) {
                newSquareWidth = (Fl::event_x_root() - x()) - newSquareOriginX;
                newSquareHeight = (Fl::event_y_root() - y()) - newSquareOriginY;

                newSquareCenterX = newSquareOriginX + (newSquareWidth / 2);
                newSquareCenterY = newSquareOriginY + (newSquareHeight / 2);

                redraw();
            }
            else if(curState == SELECTED) {
                if(curSelectedSquare) {
                    curSelectedSquare->screenX = (Fl::event_x_root() - x()) - (curSelectedSquare->screenWidth / 2);
                    curSelectedSquare->screenY = (Fl::event_y_root() - y()) - (curSelectedSquare->screenHeight / 2);

                    curSelectedSquare->screenCenterX = curSelectedSquare->screenX + (curSelectedSquare->screenWidth / 2);
                    curSelectedSquare->screenCenterY = curSelectedSquare->screenY + (curSelectedSquare->screenHeight / 2);

                    redraw();
                }
            }
        }
        else if(event == FL_RELEASE) {
            if(curState == DRAWING) {
                point newSiteObject;
                newSiteObject.screenX = newSquareOriginX;
                newSiteObject.screenY = newSquareOriginY;

                newSiteObject.screenWidth = newSquareWidth;
                newSiteObject.screenHeight = newSquareHeight;

                newSiteObject.screenCenterX = newSiteObject.screenX + (newSiteObject.screenWidth / 2);
                newSiteObject.screenCenterY = newSiteObject.screenY + (newSiteObject.screenHeight / 2);

                newSiteObject.worldX = (newSiteObject.screenX / scaleX) + minX;
                newSiteObject.worldY = (newSiteObject.screenY / scaleY) + minY;

                newSiteObject.worldWidth = (newSiteObject.worldX / scaleX) + minX;
                newSiteObject.worldHeight = (newSiteObject.worldY / scaleY) + minY;

                newSiteObject.r = 255;
                newSiteObject.g = 255;
                newSiteObject.b = 255;

                point *closest = closestPoint(newSiteObject.screenX, newSiteObject.screenY, points);

                if(closest) {
                    newSiteObject.elevation = closest->elevation;
                }

                char query[512];
                sprintf(query, "INSERT INTO objects VALUES (%d, %d, %d, %d, %f, %f, %f, %f, %f, %u, %u, %u);",
                        newSiteObject.screenX,
                        newSiteObject.screenY,
                        newSiteObject.screenWidth,
                        newSiteObject.screenHeight,
                        newSiteObject.worldX,
                        newSiteObject.worldY,
                        newSiteObject.worldWidth,
                        newSiteObject.worldHeight,
                        newSiteObject.elevation,
                        newSiteObject.r,
                        newSiteObject.g,
                        newSiteObject.b);

                sqlite3_exec(db, query, NULL, NULL, NULL);

                siteObjects.push_back(newSiteObject);

                newSquareWidth = newSquareHeight = 0;

                curState = WAITING;
                redraw();
            }
            else if(curState == SELECTED) {
                curSelectedSquare->screenX = (Fl::event_x_root() - x()) - (curSelectedSquare->screenWidth / 2);
                curSelectedSquare->screenY = (Fl::event_y_root() - y()) - (curSelectedSquare->screenHeight / 2);

                char query[512];
                sprintf(query, "UPDATE objects set screenX = %d, screenY = %d WHERE rowid = %d;",
                        curSelectedSquare->screenX,
                        curSelectedSquare->screenY,
                        curSelectedSquare->id);

                sqlite3_exec(db, query, NULL, NULL, NULL);
            }
        }
        else {
            point *closest = closestPoint(Fl::event_x_root() - x(), Fl::event_y_root() - y(), points);
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
            sprintf(query, "DELETE FROM objects WHERE rowid = %d;", curSelectedSquare->id);
            sqlite3_exec(db, query, NULL, NULL, NULL);

            std::vector<point>::iterator it = siteObjects.begin();

            for(; it != siteObjects.end(); it++) {
                if(it->id == curSelectedSquare->id) {
                    siteObjects.erase(it);
                    break;
                }
            }

            redraw();
            curState = WAITING;
        }
    }

    return Fl_Window::handle(event);
}

void SiteEditor::draw() {
    fl_draw_box(FL_FLAT_BOX, 0, 0, w(), h(), FL_BLACK);

    for(unsigned i = 0; i < points.size(); i++) {
        fl_color(points[i].r, points[i].g, points[i].b);
        fl_circle(points[i].screenX, points[i].screenY, 1);
    }

    fl_color(FL_WHITE);
    fl_line_style(FL_SOLID, 5);

    for(unsigned i = 0; i < siteObjects.size(); i++) {
        fl_color(siteObjects[i].r, siteObjects[i].g, siteObjects[i].b);
        fl_begin_loop();
        fl_vertex(siteObjects[i].screenX, siteObjects[i].screenY);
        fl_vertex(siteObjects[i].screenX + siteObjects[i].screenWidth, siteObjects[i].screenY);
        fl_vertex(siteObjects[i].screenX + siteObjects[i].screenWidth, siteObjects[i].screenY + siteObjects[i].screenHeight);
        fl_vertex(siteObjects[i].screenX, siteObjects[i].screenY + siteObjects[i].screenHeight);
        fl_vertex(siteObjects[i].screenX, siteObjects[i].screenY);
        fl_end_loop();

        fl_begin_loop();
        fl_vertex(siteObjects[i].screenCenterX - 2, siteObjects[i].screenCenterY - 2);
        fl_vertex(siteObjects[i].screenCenterX - 2, siteObjects[i].screenCenterY + 2);
        fl_vertex(siteObjects[i].screenCenterX + 2, siteObjects[i].screenCenterY + 2);
        fl_vertex(siteObjects[i].screenCenterX + 2, siteObjects[i].screenCenterY - 2);
        fl_end_loop();
    }

    if(curState == DRAWING) {
        fl_color(newSquareR, newSquareG, newSquareB);
        fl_begin_loop();
        fl_vertex(newSquareOriginX, newSquareOriginY);
        fl_vertex(newSquareOriginX + newSquareWidth, newSquareOriginY);
        fl_vertex(newSquareOriginX + newSquareWidth, newSquareOriginY + newSquareHeight);
        fl_vertex(newSquareOriginX, newSquareOriginY + newSquareHeight);
        fl_vertex(newSquareOriginX, newSquareOriginY);
        fl_end_loop();

        fl_begin_loop();
        fl_vertex(newSquareCenterX - 2, newSquareCenterY - 2);
        fl_vertex(newSquareCenterX - 2, newSquareCenterY + 2);
        fl_vertex(newSquareCenterX + 2, newSquareCenterY + 2);
        fl_vertex(newSquareCenterX + 2, newSquareCenterY - 2);
        fl_end_loop();
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

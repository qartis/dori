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
//#include "circleobject.h"
#include "colorchooser.h"
#include "toolbar.h"
#include "siteeditor.h"
#include <unistd.h>

#define CUSTOM_FONT 69
#define SELECTION_DISTANCE 30

static void window_cb(Fl_Widget *widget, void *data) {
    (void)data;
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
    (void)ncols;
    (void)colNames;
    std::vector<SiteObject*>* objs = (std::vector<SiteObject*>*)arg;

    // rowid, siteid, type
    int rowid = atoi(cols[0]);
    int siteid = atoi(cols[1]);
    SiteObjectType type = (SiteObjectType)(atoi(cols[2]));

    SiteObject *obj;

    switch(type) {
    case LINE:
        //obj = new LineObject;
        //break;
    case RECT:
        obj = new RectObject;
        break;
    case CIRCLE:
        //obj = new CircleObject;
        //break;
    default:
        fprintf(stderr, "ERROR: undefined object type\n");
    }

    obj->fromString(cols[3]);
    obj->id = rowid;

    objs->push_back(obj);

    return 0;
}

static void setColorCallback(void *data, float r, float g, float b) {
    SiteEditor *siteeditor = (SiteEditor*)data;

    siteeditor->beginDatabaseTransaction();

    for(unsigned i = 0; i < siteeditor->selectedObjects.size(); i++) {
        siteeditor->selectedObjects[i]->setRGB(r * 255.0, g * 255.0, b * 255.0);

        char data[512];
        char query[512];

        siteeditor->selectedObjects[i]->toString(data);
        sprintf(query, "UPDATE objects SET data = \"%s\" WHERE rowid = %d", data, siteeditor->selectedObjects[i]->id);

        int rc = sqlite3_exec(siteeditor->db, query, NULL, NULL, NULL);
        if(SQLITE_OK != rc) {
            fprintf(stderr, "Couldn't update object entry in db\n");
        }
    }

    // redraw() first so the screen is up to date while the db updates
    siteeditor->redraw();
    siteeditor->endDatabaseTransaction();
}

static void clickedObjTypeCallback(void *data) {
    SiteEditor *siteeditor = (SiteEditor*)data;
    siteeditor->clearSelectedObjects();
}

SiteEditor::SiteEditor(int x, int y, int w, int h, const char *label) : Fl_Double_Window(x, y, w, h, label), toolbar(NULL), db(NULL), curMouseOverElevation(0.0), curState(WAITING), newSiteObject(NULL), maxScreenWidth(w), maxScreenHeight(h), siteMeterExtents(MAX_METER_EXTENTS) {
    //Fl::set_font(CUSTOM_FONT, "OCRB");
    end();
    toolbar = new Toolbar(h, 0, 200, 200);

    toolbar->colorChooser->user_data(this);
    toolbar->colorChooser->setColorCallback = setColorCallback;

    toolbar->user_data(this);
    toolbar->clickedObjTypeCallback = clickedObjTypeCallback;

    toolbar->show();

    loadSiteObjects("data/siteobjects.db");
}

SiteEditor::~SiteEditor() {
    for(unsigned i = 0; i < siteObjects.size(); i++) {
        delete siteObjects[i];
    }

    sqlite3_close(db);
}

inline int SiteEditor::doriScreenX() {
    return w() / 2;
}

inline int SiteEditor::doriScreenY() {
    return h() / 2;
}


SiteObject* SiteEditor::findClosestObject(int mouseX, int mouseY, int maxDistance, std::vector<SiteObject*>& dataset) {
    SiteObject *closest = NULL;
    int square, minSquare;
    int squareMaxDist = maxDistance * maxDistance;
    minSquare = INT_MAX;

    for(unsigned i = 0; i < dataset.size(); i++) {
        float worldOffsetCenterX = dataset[i]->getWorldOffsetCenterX();
        float worldOffsetCenterY = dataset[i]->getWorldOffsetCenterY();

        int screenCenterX = doriScreenX() + SiteObject::worldToScreen(worldOffsetCenterX, siteMeterExtents, w());
        int screenCenterY = doriScreenY() - SiteObject::worldToScreen(worldOffsetCenterY, siteMeterExtents, h());

        int diffX = (mouseX - screenCenterX);
        int diffY = (mouseY - screenCenterY);

        square = (diffX * diffX) + (diffY * diffY);

        if(square < squareMaxDist && square < minSquare) {
            minSquare = square;
            closest = dataset[i];
        }
    }

    return closest;
}

void SiteEditor::findObjectsInBoundingBox(float worldStartMouseX, float worldStartMouseY, float worldCurMouseX, float worldCurMouseY, std::vector<SiteObject*> &inputData, std::vector<SiteObject*> &outputData) {
    outputData.clear();

    for(unsigned i = 0; i < inputData.size(); i++) {
        float centerX = inputData[i]->getWorldOffsetCenterX();
        float centerY = inputData[i]->getWorldOffsetCenterY();

        // Check that the center of the object is within the bounding box
        // of the mouse start coordinates, and current coordinates
        if(centerX >= worldStartMouseX && centerX <= worldCurMouseX &&
           centerY <= worldStartMouseY && centerY >= worldCurMouseY) {
            outputData.push_back(inputData[i]);
            inputData[i]->select();
        }
        else if(centerX <= worldStartMouseX && centerX >= worldCurMouseX &&
           centerY >= worldStartMouseY && centerY <= worldCurMouseY) {
            outputData.push_back(inputData[i]);
            inputData[i]->select();
        }
    }
}


void SiteEditor::loadSiteObjects(const char * db_name) {
    scaleX = w() / (maxX - minX);
    scaleY = h() / (maxY - minY);

    sqlite3_open(db_name, &db);
    if(!db) {
        perror("processData()");
        exit(1);
    }

    const char* query = "SELECT rowid, * FROM objects;";
    int ret = sqlite3_exec(db, query, sqlite_cb, &siteObjects, NULL);
    if (ret != SQLITE_OK)
    {
        fprintf(stderr, "error executing query\n");
    }

    end();

    callback(window_cb);

    redraw();
}

void SiteEditor::createNewObject(SiteObjectType type, int mouseX, int mouseY) {
    if(toolbar->curSelectedObjType == RECT) {
        newSiteObject = new RectObject;
        RectObject *newRectObject = (RectObject*)newSiteObject;

        int mouseDistX = mouseX - doriScreenX();
        int mouseDistY = doriScreenY() - mouseY;

        newRectObject->worldOffsetX = SiteObject::screenToWorld(mouseDistX, siteMeterExtents, w());
        newRectObject->worldOffsetY = SiteObject::screenToWorld(mouseDistY, siteMeterExtents, h());
    }
    else if(toolbar->curSelectedObjType == LINE) {
        newSiteObject = new LineObject;
        LineObject *newLineObject = (LineObject*)newSiteObject;

        int mouseDistX = mouseX - doriScreenX();
        int mouseDistY = doriScreenY() - mouseY;

        newLineObject->worldOffsetX = SiteObject::screenToWorld(mouseDistX, siteMeterExtents, w());
        newLineObject->worldOffsetY = SiteObject::screenToWorld(mouseDistY, siteMeterExtents, h());
    }
}

void SiteEditor::sizeObject(SiteObjectType type, int mouseX, int mouseY) {
    if(toolbar->curSelectedObjType == RECT) {
        RectObject *newRectObject = (RectObject*)newSiteObject;

        int rectOriginX = doriScreenX() + SiteObject::worldToScreen(newRectObject->worldOffsetX, siteMeterExtents, w());
        int rectOriginY = doriScreenY() - SiteObject::worldToScreen(newRectObject->worldOffsetY, siteMeterExtents, h());

        int mouseDistX = mouseX - rectOriginX;
        int mouseDistY = rectOriginY - mouseY;

        newRectObject->worldWidth  = SiteObject::screenToWorld(mouseDistX, siteMeterExtents, w());
        newRectObject->worldLength = SiteObject::screenToWorld(mouseDistY, siteMeterExtents, h());
    }
    else if(toolbar->curSelectedObjType == LINE) {
        LineObject *newLineObject = (LineObject*)newSiteObject; int doriX = w() / 2;
        int doriY = h() / 2;

        int lineOriginX = doriX + SiteObject::worldToScreen(newLineObject->worldOffsetX, siteMeterExtents, w());
        int lineOriginY = doriY - SiteObject::worldToScreen(newLineObject->worldOffsetY, siteMeterExtents, h());

        int mouseDistX = mouseX - lineOriginX;
        int mouseDistY = lineOriginY - mouseY;

        newLineObject->worldWidth  = SiteObject::screenToWorld(mouseDistX, siteMeterExtents, w());
        newLineObject->worldLength = SiteObject::screenToWorld(mouseDistY, siteMeterExtents, h());
    }
}

void SiteEditor::clearSelectedObjects() {
    for(unsigned i = 0; i < selectedObjects.size(); i++) {
        selectedObjects[i]->unselect();
    }
    selectedObjects.clear();
}

void SiteEditor::selectAllObjects() {
    clearSelectedObjects();

    for(unsigned i = 0; i < siteObjects.size(); i++) {
        siteObjects[i]->select();
        selectedObjects.push_back(siteObjects[i]);
    }
}

bool SiteEditor::isObjectSelected(SiteObject *obj) {
    return std::find(selectedObjects.begin(), selectedObjects.end(), obj) != selectedObjects.end();
}


void SiteEditor::beginDatabaseTransaction() {
    int err = sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    if(err != SQLITE_OK) {
        printf("sqlite error: %s\n", sqlite3_errmsg(db));
    }
}

void SiteEditor::endDatabaseTransaction() {
    int err = sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
    if(err != SQLITE_OK) {
        printf("sqlite error: %s\n", sqlite3_errmsg(db));
    }
}

void SiteEditor::commitSelectedObjectsToDatabase() {
    beginDatabaseTransaction();

    for(unsigned i = 0; i < selectedObjects.size(); i++) {
        selectedObjects[i]->unselect();

        char data[512];
        char query[512];

        selectedObjects[i]->toString(data);
        sprintf(query, "UPDATE objects SET data = \"%s\" WHERE rowid = %d", data, selectedObjects[i]->id);

        int rc = sqlite3_exec(db, query, NULL, NULL, NULL);
        if(SQLITE_OK != rc) {
            fprintf(stderr, "Couldn't update object entry in db\n");
        }

        selectedObjects[i]->select();
    }

    endDatabaseTransaction();
}


int SiteEditor::handle(int event) {
    curMouseX = (Fl::event_x_root() - x());
    curMouseY = (Fl::event_y_root() - y());

    if(Fl::event_button()) {
        if(event == FL_PUSH) {
            int distance = 0;

            if(toolbar->curSelectedObjType == UNDEFINED) {
                SiteObject* curSelectedObject = findClosestObject(curMouseX, curMouseY, scaledSelectionDistance(), siteObjects);

                selectionStartMouseX = curMouseX;
                selectionStartMouseY = curMouseY;

                if(curSelectedObject == NULL) {
                    clearSelectedObjects();
                    curState = SELECTING;
                }
                else {
                    // Check if the currently selected object is already in the list of selected objects
                    // If it isn't, then this should be a new selection, so clear out the list */
                    if(!isObjectSelected(curSelectedObject)) {
                        clearSelectedObjects();
                        selectedObjects.push_back(curSelectedObject);
                        curSelectedObject->select();
                        curState = SELECTED;
                        redraw();
                    }
                }
                return 1;
            }
            else { // if we're drawing
                clearSelectedObjects();
                curState = DRAWING;
                createNewObject(toolbar->curSelectedObjType, curMouseX, curMouseY);

                if(newSiteObject) {
                    newSiteObject->r = toolbar->colorChooser->r() * 255.0;
                    newSiteObject->g = toolbar->colorChooser->g() * 255.0;
                    newSiteObject->b = toolbar->colorChooser->b() * 255.0;
                }

                redraw();
                return 1;
            }
        }
        else if(event == FL_DRAG) {
            if(curState == DRAWING) {
                sizeObject(toolbar->curSelectedObjType, curMouseX, curMouseY);
                redraw();
                return 1;
            }
            else if(curState == SELECTED) {
                int mouseDistX = curMouseX - selectionStartMouseX;
                int mouseDistY = selectionStartMouseY - curMouseY;

                for(unsigned i = 0; i < selectedObjects.size(); i++) {
                    float originX = selectedObjects[i]->getWorldOffsetCenterX();
                    float originY = selectedObjects[i]->getWorldOffsetCenterY();

                    float offsetX = SiteObject::screenToWorld(mouseDistX, siteMeterExtents, w());
                    float offsetY = SiteObject::screenToWorld(mouseDistY, siteMeterExtents, h());

                    selectedObjects[i]->setWorldOffsetCenterX(originX + offsetX);
                    selectedObjects[i]->setWorldOffsetCenterY(originY + offsetY);

                    selectionStartMouseX = curMouseX;
                    selectionStartMouseY = curMouseY;
                }

                redraw();
                return 1;
            }
            else { // we're not drawing and we're not moving objects
                   // so we must be drawing a selection box
                curState = SELECTING;
                redraw();
                return 1;
            }
        }
        else if(event == FL_RELEASE) {
            if(curState == DRAWING) {
                curState = WAITING;
                redraw();

                char data[512];
                char query[512];
                newSiteObject->toString(data);

                sprintf(query, "INSERT INTO objects (siteid, type, data) VALUES (%d, %d, \"%s\");", 1, newSiteObject->type, data);
                int rc = sqlite3_exec(db, query, NULL, NULL, NULL);
                if(SQLITE_OK != rc) {
                    fprintf(stderr, "Couldn't insert object entry into db\n");
                }

                sqlite_int64 rowid = sqlite3_last_insert_rowid(db);
                newSiteObject->id = rowid;
                siteObjects.push_back(newSiteObject);

                newSiteObject = NULL;
                return 1;
            }
            else if(curState == SELECTED) {
                commitSelectedObjectsToDatabase();
                redraw();
                return 1;
            }
            else { // done drawing the selection box
                float worldStartMouseX = SiteObject::screenToWorld(selectionStartMouseX - doriScreenX(), siteMeterExtents, w());
                float worldStartMouseY = SiteObject::screenToWorld(doriScreenY() - selectionStartMouseY, siteMeterExtents, h());

                float worldCurMouseX = SiteObject::screenToWorld(curMouseX - doriScreenX(), siteMeterExtents, w());
                float worldCurMouseY = SiteObject::screenToWorld(doriScreenY() - curMouseY, siteMeterExtents, h());

                findObjectsInBoundingBox(worldStartMouseX, worldStartMouseY, worldCurMouseX, worldCurMouseY, siteObjects, selectedObjects);
                if(selectedObjects.size() > 0) {
                    curState = SELECTED;
                }
                else {
                    curState = WAITING;
                }
                redraw();
                return 1;
            }
        }
    }

    if(event == FL_KEYDOWN) {
        int key = Fl::event_key();

        if(key == FL_Delete && curState == SELECTED) {
            char query[512];
            beginDatabaseTransaction();

            // Loop through all selected objects, and remove
            // all site objects with matching IDs
            std::vector<SiteObject*>::iterator itSelected = selectedObjects.begin();
            for(; itSelected != selectedObjects.end(); itSelected++) {
                sprintf(query, "DELETE FROM objects WHERE rowid = %d;", (*itSelected)->id);
                sqlite3_exec(db, query, NULL, NULL, NULL);

                std::vector<SiteObject*>::iterator itAllObjects = siteObjects.begin();
                for(; itAllObjects != siteObjects.end(); itAllObjects++) {
                    if((*itAllObjects)->id == (*itSelected)->id) {
                        itAllObjects = siteObjects.erase(itAllObjects);
                        break;
                    }
                }
            }
            selectedObjects.clear();
            curState = WAITING;
            endDatabaseTransaction();
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
            clearSelectedObjects();
            curState = WAITING;
            toolbar->curSelectedObjType = UNDEFINED;
            toolbar->lineButton->value(0);
            toolbar->rectButton->value(0);
            toolbar->circleButton->value(0);
            redraw();
            return 1;
        }
        else if(key == 'a' && Fl::event_ctrl()) {
            selectAllObjects();
            redraw();
            curState = SELECTED;
            return 1;
        }
    }

    return Fl_Window::handle(event);
}

void SiteEditor::drawGrid() {
    // horz lines
    int offset = 5;

    fl_color(FL_GRAY);
    fl_line_style(FL_DASH, 1);
    for(int i = offset; i < h(); i+=offset) {
        fl_line(0, i*offset, w(), i*offset);
    }


    // vert lines
    for(int i = offset; i < w(); i+=offset) {
        fl_line(i*offset, 0, i*offset, h());
    }
}

void SiteEditor::draw() {
    fl_draw_box(FL_FLAT_BOX, 0, 0, w(), h(), fl_rgb_color(25, 25, 25));

    drawGrid();

    fl_line_style(0,2);

    fl_color(FL_WHITE);
    fl_line_style(FL_SOLID, 5);

    // draw DORI
    fl_draw_box(FL_FLAT_BOX, (295.0 / maxScreenWidth) * w(), (297.0 / maxScreenHeight) * h(), (10.0 / maxScreenWidth) * w(), (8.0 / maxScreenHeight) * h(), FL_RED);

    for(unsigned i = 0; i < siteObjects.size(); i++) {
        siteObjects[i]->drawScreen(true, w(), h(), siteMeterExtents);
    }

    switch(curState) {
    case DRAWING:
        if(newSiteObject) {
            newSiteObject->drawScreen(true, w(), h(), siteMeterExtents);
        }
        break;
    case SELECTING:
        fl_line_style(1);
        fl_color(255, 255, 255);
        fl_begin_loop();
        fl_vertex(selectionStartMouseX, selectionStartMouseY);
        fl_vertex(curMouseX, selectionStartMouseY);
        fl_vertex(curMouseX, curMouseY);
        fl_vertex(selectionStartMouseX, curMouseY);
        fl_end_loop();
        break;
    case SELECTED:
        break;
    case WAITING:
        break;
    }

    fl_line_style(0);
}

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
#include "colorchooser.h"
#include "toolbar.h"
#include "siteeditor.h"
#include <unistd.h>

#define CUSTOM_FONT 69
#define SELECTION_DISTANCE 20

static void window_cb(Fl_Widget *widget, void *data) {
    (void)data;
    SiteEditor* siteeditor = (SiteEditor*)widget;
    siteeditor->toolbar->hide();
    siteeditor->hide();
}

float SiteEditor::scaledSelectionDistance() {
    int minX = (SELECTION_DISTANCE / maxWindowWidth) * w();
    int minY = (SELECTION_DISTANCE / maxWindowLength) * h();
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

SiteEditor::SiteEditor(int x, int y, int w, int h, const char *label) : Fl_Double_Window(x, y, w, h, label), toolbar(NULL), db(NULL), curMouseOverElevation(0.0), curState(WAITING), newSiteObject(NULL), maxWindowWidth(w), maxWindowLength(h), panStartMouseX(0), panStartMouseY(0), screenPanX(0), screenPanY(0) {
    Fl::set_font(CUSTOM_FONT, "OCRB");
    end();
    toolbar = new Toolbar(h, 0, 200, 200);

    toolbar->colorChooser->user_data(this);
    toolbar->colorChooser->setColorCallback = setColorCallback;

    toolbar->user_data(this);
    toolbar->clickedObjTypeCallback = clickedObjTypeCallback;

    toolbar->show();

    loadSiteObjects("data/siteobjects.db");

    siteMeterExtents = MAX_METER_EXTENTS * 2; // 80m in all directions
    sitePixelExtents = w * 8;
    cellsPerMeter = 4.0;


    // numGridCells =  (site in meters) / (meters / cell)
    //              =  (site in meters) * (cell / meters)
    //              =  cells
    numGridCells = siteMeterExtents * (float)cellsPerMeter;
    pixelsPerCell = ((float)sitePixelExtents / (float)numGridCells);

    numArcs = MAX_METER_EXTENTS;

    screenPanX = -((sitePixelExtents / 2) - (w / 2));
    screenPanY = -((sitePixelExtents / 2) - (h / 2));
}

SiteEditor::~SiteEditor() {
    for(unsigned i = 0; i < siteObjects.size(); i++) {
        delete siteObjects[i];
    }

    sqlite3_close(db);
}

inline int SiteEditor::siteScreenCenterX() {
    return screenPanX + (sitePixelExtents / 2);
}

inline int SiteEditor::siteScreenCenterY() {
    return screenPanY + (sitePixelExtents / 2);
}


SiteObject* SiteEditor::findClosestObject(int mouseX, int mouseY, int maxDistance, std::vector<SiteObject*>& dataset) {
    SiteObject *closest = NULL;
    int square, minSquare;
    int squareMaxDist = maxDistance * maxDistance;
    minSquare = INT_MAX;

    for(unsigned i = 0; i < dataset.size(); i++) {
        float worldOffsetCenterX = dataset[i]->getWorldOffsetCenterX();
        float worldOffsetCenterY = dataset[i]->getWorldOffsetCenterY();

        int screenCenterX = siteScreenCenterX() + (worldOffsetCenterX * cellsPerMeter) * pixelsPerCell;
        int screenCenterY = siteScreenCenterY() - (worldOffsetCenterY * cellsPerMeter) * pixelsPerCell;

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

void SiteEditor::findObjectsInBoundingBox(int mouseStartX, int mouseStartY, int curMouseX, int curMouseY, std::vector<SiteObject*> &inputData, std::vector<SiteObject*> &outputData) {
    outputData.clear();

    float cellsPerPixel = 1.0 / pixelsPerCell;
    float metersPerCell = 1.0 / cellsPerMeter;

    float worldStartMouseX = (mouseStartX - siteScreenCenterX()) * cellsPerPixel * metersPerCell;
    float worldStartMouseY = (siteScreenCenterY() - mouseStartY) * cellsPerPixel * metersPerCell;

    float worldCurMouseX = (curMouseX - siteScreenCenterX()) * cellsPerPixel * metersPerCell;
    float worldCurMouseY = (siteScreenCenterY() - curMouseY) * cellsPerPixel * metersPerCell;

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
    if(type == RECT) {
        newSiteObject = new RectObject;
    }
    else if(type == LINE) {
        newSiteObject = new LineObject;

    }
    else if(type == CIRCLE) {
        newSiteObject = new CircleObject;
    }

    int mouseDistX = mouseX - siteScreenCenterX();
    int mouseDistY = siteScreenCenterY() - mouseY;

    // # cells = mouse dist in pixels * (cells / pixel)
    // meters = # cells * (meters / cell)
    float cellsPerPixel = 1.0 / pixelsPerCell;
    float metersPerCell = 1.0 / (float)cellsPerMeter;

    newSiteObject->worldOffsetX = (mouseDistX * cellsPerPixel) * metersPerCell;
    newSiteObject->worldOffsetY = (mouseDistY * cellsPerPixel) * metersPerCell;
}

void SiteEditor::sizeObject(SiteObjectType type, SiteObject *object, int mouseX, int mouseY) {

    float originX = siteScreenCenterX() + (cellsPerMeter * object->worldOffsetX) * pixelsPerCell;
    float originY = siteScreenCenterY() - (cellsPerMeter * object->worldOffsetY) * pixelsPerCell;

    int mouseDistX = mouseX - originX;
    int mouseDistY = originY - mouseY;

    // # cells = mouse dist in pixels * (cells / pixel)
    // meters = # cells * (meters / cell)
    float cellsPerPixel = 1.0 / pixelsPerCell;
    float metersPerCell = 1.0 / (float)cellsPerMeter;

    if(toolbar->curSelectedObjType == RECT) {
        RectObject *newRectObject = (RectObject*)newSiteObject;

        newRectObject->worldWidth  = (mouseDistX * cellsPerPixel) * metersPerCell;
        newRectObject->worldLength = (mouseDistY * cellsPerPixel) * metersPerCell;
    }
    else if(toolbar->curSelectedObjType == LINE) {
        LineObject *newLineObject = (LineObject*)newSiteObject;

        newLineObject->worldWidth  = (mouseDistX * cellsPerPixel) * metersPerCell;
        newLineObject->worldLength = (mouseDistY * cellsPerPixel) * metersPerCell;
    }
    else if(toolbar->curSelectedObjType == CIRCLE) {
        CircleObject *newCircleObject = (CircleObject*)newSiteObject;

        int maxMouseDist;

        // Take the max mouse distance as the radius
        if(mouseDistX > mouseDistY) {
            maxMouseDist = mouseDistX;
        }
        else {
            maxMouseDist = mouseDistY;
        }

        newCircleObject->worldRadius = (maxMouseDist * cellsPerPixel) * metersPerCell;
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
        if(Fl::event_button() == FL_LEFT_MOUSE) {
            if(event == FL_PUSH) {
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
                    sizeObject(toolbar->curSelectedObjType, newSiteObject, curMouseX, curMouseY);
                    redraw();
                    return 1;
                }
                else if(curState == SELECTED) {
                    int mouseDistX = curMouseX - selectionStartMouseX;
                    int mouseDistY = selectionStartMouseY - curMouseY;

                    for(unsigned i = 0; i < selectedObjects.size(); i++) {
                        float originX = selectedObjects[i]->getWorldOffsetCenterX();
                        float originY = selectedObjects[i]->getWorldOffsetCenterY();

                        float offsetX = (mouseDistX * (1.0 / pixelsPerCell)) * (1.0 / cellsPerMeter);
                        float offsetY = (mouseDistY * (1.0 / pixelsPerCell)) * (1.0 / cellsPerMeter);

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
                    findObjectsInBoundingBox(selectionStartMouseX, selectionStartMouseY, curMouseX, curMouseY, siteObjects, selectedObjects);
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
        else if(Fl::event_button() == FL_MIDDLE_MOUSE) {
            if(event == FL_PUSH) {
                panStartMouseX = curMouseX;
                panStartMouseY = curMouseY;
            }
            else if(event == FL_DRAG) {
                int newPanX = screenPanX + (curMouseX - panStartMouseX);
                int newPanY = screenPanY + (curMouseY - panStartMouseY);

                // Prevent panning off the left side of the screen
                if(newPanX > 0) {
                    newPanX = 0;
                }
                // Prevent panning off of the right side of the screen
                // Subtract w() because panning is measured from the
                // left side of the screen, so we want to stop panning
                // to the right with one screen length remaining
                else if(newPanX < -(sitePixelExtents - w())) {
                    newPanX = -(sitePixelExtents - w());
                }
                if(newPanY > 0) {
                    newPanY = 0;
                }
                else if(newPanY < -(sitePixelExtents - h())) {
                    newPanY = -(sitePixelExtents - h());
                }

                screenPanX =  newPanX;
                screenPanY = newPanY;
                panStartMouseX = curMouseX;
                panStartMouseY = curMouseY;
            }
        }
        else if(event == FL_MOUSEWHEEL) {
            if(Fl::event_dy() > 0 && numGridCells < (siteMeterExtents * cellsPerMeter)) {
                numGridCells += cellsPerMeter * 2;
            }
            else if(numGridCells > cellsPerMeter * 2) {
                numGridCells -= cellsPerMeter * 2;
            }

            pixelsPerCell = ((float)sitePixelExtents / (float)numGridCells);

            redraw();
            return 1;
        }

        redraw();
    }

    // Redraw on move so that we can update the mouse distance
    // TODO: Change this to damage instead of redrawing the whole thing
    if(event == FL_MOVE) {
        redraw();
        return 1;
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
            curState = SELECTED;
            toolbar->curSelectedObjType = UNDEFINED;
            toolbar->curSelectedObjType = UNDEFINED;
            toolbar->lineButton->value(0);
            toolbar->rectButton->value(0);
            toolbar->circleButton->value(0);
            redraw();
            return 1;
        }
        else if(key == ' ') {
            screenPanX = -((sitePixelExtents / 2) - (w() / 2));
            screenPanY = -((sitePixelExtents / 2) - (h() / 2));
            redraw();
            return 1;
        }
    }

    return Fl_Window::handle(event);
}

void SiteEditor::drawGrid(float pixelsPerCell, int numCells) {
    if(pixelsPerCell < 5) {
        pixelsPerCell = 5;
    }

    char textBuffer[32];
    int textWidth, textHeight;
    fl_font(CUSTOM_FONT, 15);

    // We want to draw the meter markers in distance away from DORI
    // The number of gridlines specify the number of quarter meter indications
    // Therefore our first number should be gridlines / 8
    // And we should decrement by 1 for every two gridlines
    // until we hit the middle line, then we increase again
    int index = numCells / (cellsPerMeter * 2);
    int diff = -1;

    fl_color(FL_GRAY);

    // horz lines
    fl_line_style(0, 1);
    for(int i = 0; i < numCells - 1; i++) {
        fl_line(screenPanX, screenPanY + (i * pixelsPerCell), sitePixelExtents, screenPanY + (i * pixelsPerCell));
    }

    // vert lines
    for(int i = 0; i < numCells - 1; i++) {
        fl_line(screenPanX + (i * pixelsPerCell), screenPanY, screenPanX + (i * pixelsPerCell), sitePixelExtents);
    }

    // reset index
    index = numCells / (cellsPerMeter * 2);
    diff = -1;

    // horz labels
    fl_color(FL_DARK1);
    fl_line_style(0, 1);
    for(int i = 0; i < numCells - 1; i++) {
        if(i % cellsPerMeter == 0) {
            sprintf(textBuffer, "%d", index);
            fl_measure(textBuffer, textWidth, textHeight, 0);

            if(index == 0) {
                diff = 1;
            }
            else {
                fl_color(FL_BLUE);
                fl_draw(textBuffer, w() - textWidth, screenPanY + (i * pixelsPerCell) + (textHeight / 2.0));
            }

            index += diff;
        }
    }


    // reset index
    index = numCells / (cellsPerMeter * 2);
    diff = -1;

    // vert labels
    for(int i = 0; i < numCells - 1; i++) {
        if(i % cellsPerMeter == 0) {
            sprintf(textBuffer, "%d", index);
            fl_measure(textBuffer, textWidth, textHeight, 0);

            if(index == 0) {
                diff = 1;
            }
            else {
                fl_color(FL_BLUE);
                fl_draw(textBuffer, screenPanX + (i * pixelsPerCell) - (textWidth / 2.0), h() - textHeight);
            }

            index += diff;
        }
    }

    fl_line_style(0);
}

void SiteEditor::drawArcs() {
    // height of line's second point
    // is (1/2) *  width * tan(outside angle converted to radians)
    // outside angle = (180.0 - inside angle) / 2
    float outsideAngle = -180.0 / 2.0;
    outsideAngle *= (M_PI / 180.0); // convert to radians

    fl_color(FL_BLUE);
    fl_line_style(FL_SOLID, 2);

    int radius = 2 * cellsPerMeter * pixelsPerCell;
    int textHeight, textWidth;
    char textBuffer[32];

    for(int i = 0; i < numArcs; i++) {
        int arcOriginX = -(siteScreenCenterX() - (i * (radius / 2.0)));
        int arcOriginY = -(siteScreenCenterY() - (i * (radius / 2.0)));

        printf("arc %d, origin: %d %d, pan: %d %d\n", i, arcOriginX, arcOriginY, screenPanX, screenPanY);
        printf("drawing arc %d\n", i);
        fl_arc(siteScreenCenterX() - (i * (radius / 2.0)), siteScreenCenterY() - (i * (radius / 2.0)), i * radius, i * radius, 360.0, 360.0);

        if(i > 0) {
            sprintf(textBuffer, "%d", i);
            fl_measure(textBuffer, textWidth, textHeight, 0);
            fl_draw(textBuffer, siteScreenCenterX() - textWidth / 2.0, siteScreenCenterY() - (i * (radius / 2)));
        }
    }
}

void SiteEditor::drawDori() {
    // Draw dori based on the units of cells per meter, and pixels per cell
    // Basically we want to always draw DORI at 1 meter by 1 meter
    // So we offset the drawing half a meter away from the center
    // And set the wheels to be 1/4 of a meter on each side
    int doriOffset = (cellsPerMeter * pixelsPerCell) / 2;
    int doriWheelWidth = (doriOffset / 2);

    // chassis
    fl_draw_box(FL_FLAT_BOX, siteScreenCenterX() - doriOffset, siteScreenCenterY() - doriOffset, doriOffset * 2, doriOffset * 2, FL_BLUE);

    // wheels
    fl_draw_box(FL_FLAT_BOX, siteScreenCenterX() - doriOffset, siteScreenCenterY() - doriOffset, doriWheelWidth, doriOffset * 2, FL_BLACK);
    fl_draw_box(FL_FLAT_BOX, siteScreenCenterX() + (doriOffset / 2), siteScreenCenterY() - doriOffset, doriWheelWidth, doriOffset * 2, FL_BLACK);
}


void SiteEditor::draw() {
    fl_draw_box(FL_FLAT_BOX, 0, 0, w(), h(), FL_DARK2);

    drawDori();

    drawGrid(pixelsPerCell, numGridCells);

    //drawArcs();

    for(unsigned i = 0; i < siteObjects.size(); i++) {
        siteObjects[i]->drawScreen(true, cellsPerMeter, pixelsPerCell, siteScreenCenterX(), siteScreenCenterY());
    }

    switch(curState) {
    case DRAWING:
        if(newSiteObject) {
            newSiteObject->drawScreen(true, cellsPerMeter, pixelsPerCell, siteScreenCenterX(), siteScreenCenterY());
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

    char textBuffer[128];
    int textWidth, textHeight;

    fl_font(CUSTOM_FONT, 15);

    // Count the number of grid cells away from the center I am in pixels:
    float numCellsDistX = ((float)curMouseX - (float)(siteScreenCenterX())) / pixelsPerCell;
    float numCellsDistY = ((float)curMouseY - (float)(siteScreenCenterY())) / pixelsPerCell;

    // Now do (meters / cell) * cells, to get # of meters away
    float worldGridCellDistX = (1.0 / (float)cellsPerMeter) * numCellsDistX;
    float worldGridCellDistY = (1.0 / (float)cellsPerMeter) * numCellsDistY;

    float worldDist = sqrt((worldGridCellDistX * worldGridCellDistX) + (worldGridCellDistY * worldGridCellDistY));

    sprintf(textBuffer, "%.3fm", worldDist);
    fl_measure(textBuffer, textWidth, textHeight, 0);
    fl_draw_box(FL_FLAT_BOX, 5.0, 5.0, textWidth, textHeight, FL_BLACK);

    fl_color(FL_WHITE);
    fl_draw(textBuffer, 5.0, textHeight);

    // Drawing line to mouse cursor
    fl_line_style(FL_SOLID, 1);
    fl_color(FL_BLACK);
    fl_begin_loop();
    fl_vertex(siteScreenCenterX(), siteScreenCenterY());
    fl_vertex(curMouseX, curMouseY);
    fl_end_loop();
}


#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Color_Chooser.H>
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
#include <unistd.h>

#define CUSTOM_FONT 69
#define SELECTION_DISTANCE_PIXELS 13
#define CELLS_PER_METER 4.0
#define MIN_PIXELS_PER_CELL 16
#define MAX_PIXELS_PER_CELL 320
#define ZOOM_PIXEL_AMOUNT 4
#define ARROW_SCREEN_OFFSET 15.0f

#define SITE_ID 1

#define BG_COLOUR FL_DARK2
#define GRID_COLOUR FL_DARK1
#define LABEL_COLOUR FL_BLACK
#define CROSSHAIR_COLOUR FL_BLACK

static void window_cb(Fl_Widget *widget, void *data) {
    (void)data;
    SiteEditor* siteeditor = (SiteEditor*)widget;
    siteeditor->toolbar->hide();
    siteeditor->hide();
}

float SiteEditor::screenToWorld(float screenVal) {
    float cellsPerPixel = 1.0 / pixelsPerCell;
    float metersPerCell = 1.0 / CELLS_PER_METER;
    return SiteObject::screenToWorld(screenVal, cellsPerPixel, metersPerCell);
}

float SiteEditor::worldToScreen(float worldVal) {
    return SiteObject::worldToScreen(worldVal, pixelsPerCell, CELLS_PER_METER);
}

int SiteEditor::sqlite_cb(void *arg, int ncols, char**cols, char **colNames) {
    (void)ncols;
    (void)colNames;
    std::vector<SiteObject*>* objs = (std::vector<SiteObject*>*)arg;

    // rowid, siteid, type
    int rowid = atoi(cols[0]);
    int siteid = atoi(cols[1]);
    (void)siteid;

    SiteObjectType type = (SiteObjectType)(atoi(cols[2]));
    (void)type;

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
    case POLY:
        obj = new PolyObject;
        break;
    default:
        fprintf(stderr, "ERROR: undefined object type\n");
    }

    obj->fromString(cols[3]);
    obj->id = rowid;

    objs->push_back(obj);

    return 0;
}

void SiteEditor::setColorCallback(void *data, float r, float g, float b) {
    SiteEditor *siteeditor = (SiteEditor*)data;

    siteeditor->beginDatabaseTransaction();

    for(unsigned i = 0; i < siteeditor->selectedObjects.size(); i++) {
        siteeditor->selectedObjects[i]->setRGB(r * 255.0, g * 255.0, b * 255.0);

        std::ostringstream ss;

        ss << "UPDATE objects SET data = \"";
        ss << siteeditor->selectedObjects[i]->toString();
        ss << "\" WHERE rowid = " << siteeditor->selectedObjects[i]->id;

        int rc = sqlite3_exec(siteeditor->db, ss.str().c_str(), NULL, NULL, NULL);
        if(SQLITE_OK != rc) {
            fprintf(stderr, "Couldn't update object entry in db\n");
        }
    }

    // redraw() first so the screen is up to date while the db updates
    siteeditor->redraw();
    siteeditor->endDatabaseTransaction();
}

void SiteEditor::clickedObjTypeCallback(void *data) {
    SiteEditor *siteeditor = (SiteEditor*)data;
    siteeditor->clearSelectedObjects();
    siteeditor->curState = SiteEditor::WAITING;
}

SiteEditor::SiteEditor(int x, int y, int w, int h, const char *label) : Fl_Double_Window(x, y, w, h, label), toolbar(NULL), db(NULL), curMouseOverElevation(0.0), curState(WAITING), newSiteObject(NULL), maxWindowWidth(w), maxWindowLength(h), panStartWorldX(0), panStartWorldY(0), showArcs(false), showGrid(true), curWorldDistance(0.0), arrowScreenX(-1), arrowScreenY(-1) {
    Fl::set_font(CUSTOM_FONT, "OCRB");
    loadSiteObjects("data/siteobjects.db");

    siteMeterExtents = SITE_METER_EXTENTS * 2; // 80m in all directions
    sitePixelExtents = w * 8;

    numGridCells = siteMeterExtents * (float)CELLS_PER_METER;
    pixelsPerCell = MIN_PIXELS_PER_CELL;

    // one arc per meter
    numArcs = SITE_METER_EXTENTS;

    panToWorldCenter();
    end();
}

SiteEditor::~SiteEditor() {
    for(unsigned i = 0; i < siteObjects.size(); i++) {
        delete siteObjects[i];
    }

    sqlite3_close(db);
}


/*
   float SiteEditor::getAbsoluteWorldMouseX() {
   worldPanX + screenToWorld(mouseX);
   }

   float SiteEditor::getAbsoluteWorldMouseY() {
   worldPanY + screenToWorld(mouseY);
   }
   */

void SiteEditor::panToWorldCenter() {
    worldPanX = (siteMeterExtents / 2.0) - (screenToWorld(w()) / 2.0);
    worldPanY = (siteMeterExtents / 2.0) - (screenToWorld(h()) / 2.0);
}

SiteObject* SiteEditor::findClosestObject(int mouseX, int mouseY, int maxDistance, std::vector<SiteObject*>& dataset) {
    SiteObject *closest = NULL;
    float square, minSquare;
    minSquare = INT_MAX;

    float mouseWorldX = worldPanX + screenToWorld(mouseX);
    float mouseWorldY = worldPanY + screenToWorld(mouseY);

    // Convert the max selection distance from pixels to meters
    // then square it, so we can compare distance calculations
    float maxDistanceMeters = screenToWorld(maxDistance);
    maxDistanceMeters *= maxDistanceMeters;

    // distance of mouse cursor to center of site (in world coords)
    float mouseOffsetX = mouseWorldX - (siteMeterExtents / 2.0);
    float mouseOffsetY = (siteMeterExtents / 2.0) - mouseWorldY;

    for(unsigned i = 0; i < dataset.size(); i++) {
        // distance of site object to center of site (in world coords)
        float worldOffsetCenterX = dataset[i]->getWorldOffsetCenterX();
        float worldOffsetCenterY = dataset[i]->getWorldOffsetCenterY();

        // difference between mouse world and object world coords
        float diffX = (mouseOffsetX - worldOffsetCenterX);
        float diffY = (mouseOffsetY - worldOffsetCenterY);

        square = (diffX * diffX) + (diffY * diffY);


        if(square < maxDistanceMeters && square < minSquare) {
            minSquare = square;
            closest = dataset[i];
        }
    }

    return closest;
}

void SiteEditor::findObjectsInBoundingBox(int mouseStartX, int mouseStartY, int curMouseX, int curMouseY, std::vector<SiteObject*> &inputData, std::vector<SiteObject*> &outputData) {
    outputData.clear();

    // Convert the mouse coordinates to world coordinates here
    // to figure out the meter offset of the mouse cursor from center of the site
    // (currently center of the world)
    // This is better than converting the world site object coords to screen coords
    // in the loop below
    float worldStartMouseX = screenToWorld(mouseStartX) + worldPanX - (siteMeterExtents / 2.0);
    float worldStartMouseY = (siteMeterExtents / 2.0) - (screenToWorld(mouseStartY) + worldPanY);

    float worldCurMouseX = screenToWorld(curMouseX) + worldPanX - (siteMeterExtents / 2.0);
    float worldCurMouseY = (siteMeterExtents / 2.0) - (screenToWorld(curMouseY) + worldPanY);

    // set the 'start' variables to the minimums
    // and the 'cur' variables to the maximums
    if(worldCurMouseX < worldStartMouseX) {
        std::swap(worldCurMouseX, worldStartMouseX);
    }
    if(worldCurMouseY < worldStartMouseY) {
        std::swap(worldCurMouseY, worldStartMouseY);
    }

    for(unsigned i = 0; i < inputData.size(); i++) {
        float centerX = inputData[i]->getWorldOffsetCenterX();
        float centerY = inputData[i]->getWorldOffsetCenterY();

        // Check that the center of the object is within the bounding box
        // of the mouse start coordinates, and current coordinates
        if(centerX >= worldStartMouseX && centerX <= worldCurMouseX &&
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

void SiteEditor::createNewObject(SiteObjectType type, float mouseWorldX, float mouseWorldY) {
    float mouseDistX = mouseWorldX - (siteMeterExtents / 2.0);
    float mouseDistY = (siteMeterExtents / 2.0) - mouseWorldY;

    if(type == RECT) {
        newSiteObject = new RectObject;
    }
    else if(type == LINE) {
        newSiteObject = new LineObject;
    }
    else if(type == CIRCLE) {
        newSiteObject = new CircleObject;
    }
    else if(type == POLY) {
        newSiteObject = new PolyObject;
    }

    float r = toolbar->colorChooser->r() * 255.0;
    float g = toolbar->colorChooser->g() * 255.0;
    float b = toolbar->colorChooser->b() * 255.0;

    newSiteObject->init(mouseDistX, mouseDistY, r, g, b);
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

        std::ostringstream ss;

        ss << "UPDATE objects SET data = \"";
        ss << selectedObjects[i]->toString();
        ss << "\" WHERE rowid = " << selectedObjects[i]->id;

        int rc = sqlite3_exec(db, ss.str().c_str(), NULL, NULL, NULL);
        if(SQLITE_OK != rc) {
            fprintf(stderr, "Couldn't update object entry in db\n");
        }

        selectedObjects[i]->select();
    }

    endDatabaseTransaction();
}

void SiteEditor::processNewSiteObject() {
    if(newSiteObject == NULL)
        return;

    std::ostringstream ss;

    ss << "INSERT INTO objects (siteid, type, data) VALUES (";
    ss << SITE_ID << ", " << newSiteObject->type << ", ";
    ss << "\"" << newSiteObject->toString() << "\");";

    int rc = sqlite3_exec(db, ss.str().c_str(), NULL, NULL, NULL);
    if(SQLITE_OK != rc) {
        fprintf(stderr, "Couldn't insert object entry into db\n");
    }

    sqlite_int64 rowid = sqlite3_last_insert_rowid(db);
    newSiteObject->id = rowid;
    siteObjects.push_back(newSiteObject);
    newSiteObject = NULL;
}

void SiteEditor::resize(int X, int Y, int W, int H) {
    Fl_Window::resize(X, Y, W, H);
    enforceSiteBounds();
    calcArcInfo();
}

inline float SiteEditor::screenCenterWorldX() {
    return worldPanX + screenToWorld(w() / 2);
}

inline float SiteEditor::screenCenterWorldY() {
    return worldPanY + screenToWorld(h() / 2);
}

// Snap the pan back within bounds if necessary
void SiteEditor::enforceSiteBounds() {
    if(worldPanX < 0) {
        worldPanX = 0;
    }
    else if(worldPanX + screenToWorld(w()) > siteMeterExtents) {
        worldPanX = siteMeterExtents - screenToWorld(w());
    }

    if(worldPanY < 0) {
        worldPanY = 0;
    }
    else if(worldPanY + screenToWorld(h()) > siteMeterExtents) {
        worldPanY = siteMeterExtents - screenToWorld(h());
    }
}

int SiteEditor::handle(int event) {
    // only update the mouse position if we have focus
    if(Fl::belowmouse() == this) {
        curMouseX = Fl::event_x();
        curMouseY = Fl::event_y();
    }

    float centerX = screenCenterWorldX();
    float centerY = screenCenterWorldY();

    float doriX = (siteMeterExtents / 2.0);
    float doriY = (siteMeterExtents / 2.0);

    if(event == FL_MOVE || event == FL_DRAG) {
        handleMouseMovement(curMouseX, curMouseY);
    }

    if(Fl::belowmouse() == this &&
       Fl::event_button() == FL_LEFT_MOUSE) {
        if(handleLeftMouse(event, curMouseX, curMouseY, clickedMouseX, clickedMouseY)) {
            return 1;
        }
    }

    if(Fl::event_button() == FL_RIGHT_MOUSE || Fl::event_button() == FL_MIDDLE_MOUSE) {
        if(event == FL_PUSH) {
            panStartWorldX = screenToWorld(curMouseX);
            panStartWorldY = screenToWorld(curMouseY);
            return 1;
        }
        else if(event == FL_DRAG) {
            handlePan();

            calcArrowScreenPosition(centerX,
                                    centerY,
                                    doriX,
                                    doriY,
                                    arrowScreenX,
                                    arrowScreenY);

            arrowAngleDegrees = calcArrowAngleDegrees(arrowScreenX,
                                                      arrowScreenY,
                                                      centerX,
                                                      centerY,
                                                      doriX,
                                                      doriY);

            redraw();
            return 1;
        }
    }
    else if(event == FL_MOUSEWHEEL) {
        if(handleMouseWheel(centerX,
                         centerY,
                         doriX,
                         doriY,
                         arrowScreenX,
                         arrowScreenY))
        {
            return 1;
        }

    }

    // FL_SHORTCUT is used to sent keyboard events
    // to widgets that don't have focus
    if(event == FL_KEYDOWN || event == FL_SHORTCUT) {
        int key = Fl::event_key();
        if(handleKeyPress(key, centerX, centerY, doriX, doriY)) {
            return 1;
        }
    }

    return Fl_Double_Window::handle(event);
}

int SiteEditor::handleLeftMouse(int event,
                                int curMouseX,
                                int curMouseY,
                                int& clickedMouseX,
                                int& clickedMouseY)
{
    if(event == FL_PUSH) {
        clickedMouseX = curMouseX;
        clickedMouseY = curMouseY;

        if(toolbar->curSelectedObjType == UNDEFINED) {
            SiteObject* curSelectedObject = findClosestObject(curMouseX, curMouseY, SELECTION_DISTANCE_PIXELS, siteObjects);
            if(curSelectedObject == NULL) {
                clearSelectedObjects();
                curState = SELECTING;
                redraw();
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
        else {
            // if cur selected object is not undefined
            // either we're starting a drawing, finishing a drawing
            // or adding to a polygon drawing

            // Starting to draw a site object
            if(curState == WAITING) {
                float curMouseWorldX = worldPanX + screenToWorld(curMouseX);
                float curMouseWorldY = worldPanY + screenToWorld(curMouseY);

                createNewObject(toolbar->curSelectedObjType, curMouseWorldX, curMouseWorldY);
                curState = DRAWING;

                redraw();
                return 1;
            }
            else if(curState == DRAWING) {
                // Finishing or adding to a drawing

                // 'Confirm' the object
                // For non poly objects, this will set the size of the object
                // For poly, this will add the current mouse position to the list of points
                newSiteObject->confirm();

                // reset to WAITING state if we're not drawing a poly object
                if(toolbar->curSelectedObjType != POLY) {
                    // add the new (not POLY type) site object to the database and list of site objects
                    processNewSiteObject();
                    curState = WAITING;
                }

                redraw();
                return 1;
            }
        }
    }
    else if(event == FL_DRAG) {
        // Move all selected objects
        if(curState == SELECTED) {
            int mouseDistX = curMouseX - clickedMouseX;
            int mouseDistY = clickedMouseY - curMouseY;

            for(unsigned i = 0; i < selectedObjects.size(); i++) {
                float mouseWorldDistX = screenToWorld(mouseDistX);
                float mouseWorldDistY = screenToWorld(mouseDistY);

                selectedObjects[i]->updateWorldOffsetCenterX(mouseWorldDistX);
                selectedObjects[i]->updateWorldOffsetCenterY(mouseWorldDistY);

                clickedMouseX = curMouseX;
                clickedMouseY = curMouseY;
            }

            redraw();
            return 1;
        }
        else if(curState != DRAWING) {
            // so we must be drawing a selection box
            curState = SELECTING;
            redraw();
            return 1;
        }
    }
    else if(event == FL_RELEASE) {
        if(curState == SELECTED) {
            commitSelectedObjectsToDatabase();
            redraw();
            return 1;
        }
        // done drawing the selection box
        else if(curState == SELECTING) {
            findObjectsInBoundingBox(clickedMouseX, clickedMouseY, curMouseX, curMouseY, siteObjects, selectedObjects);
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

    return 0;
}

void SiteEditor::handleMouseMovement(int curMouseX, int curMouseY) {
    // Get the number of cells it takes to get from the edge of the screen to the mouse
    // which is (cells per pixel) * mouse pixels
    float mouseCellsX = (1.0 / pixelsPerCell) * curMouseX;
    float mouseCellsY = (1.0 / pixelsPerCell) * curMouseY;

    // Calculate how many meters it takes to cover this amount of cells
    // (number of cells) * meters per cell
    // then add the world pan value to get the absolute world position
    float worldMouseX = mouseCellsX * (1.0 / CELLS_PER_METER) + worldPanX;
    float worldMouseY = mouseCellsY * (1.0 / CELLS_PER_METER) + worldPanY;

    // Subtract the location of the center of the site to get the distance
    float distX = worldMouseX - (siteMeterExtents / 2.0);
    float distY = worldMouseY - (siteMeterExtents / 2.0);

    curWorldDistance = sqrt((distX * distX) + (distY * distY));

    float worldAbsoluteMouseX = worldPanX + screenToWorld(curMouseX);
    float worldAbsoluteMouseY = worldPanY + screenToWorld(curMouseY);

    float worldAbsoluteClickedX = worldPanX + screenToWorld(clickedMouseX);
    float worldAbsoluteClickedY = worldPanY + screenToWorld(clickedMouseY);

    if(newSiteObject && curState == DRAWING) {
        newSiteObject->updateSize(worldAbsoluteMouseX - worldAbsoluteClickedX, worldAbsoluteClickedY - worldAbsoluteMouseY);
    }

    redraw();
}

void SiteEditor::handlePan() {
    float curMouseWorldX = screenToWorld(curMouseX);
    float curMouseWorldY = screenToWorld(curMouseY);

    worldPanX -= (curMouseWorldX - panStartWorldX);
    worldPanY -= (curMouseWorldY - panStartWorldY);

    // snap the pan position back to within site bounds
    enforceSiteBounds();

    panStartWorldX = curMouseWorldX;
    panStartWorldY = curMouseWorldY;

    calcArcInfo();
}

int SiteEditor::handleKeyPress(int key, int centerX, int centerY, int doriX, int doriY) {
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
    else if(key == FL_Escape) {
        if(newSiteObject) {
            newSiteObject->cancel();

            if(toolbar->curSelectedObjType == POLY &&
               (((PolyObject*)newSiteObject)->numPoints() >= 2))
            {
                processNewSiteObject();
            }
            else {
                delete newSiteObject;
                newSiteObject = NULL;
            }
        }
        else {
            toolbar->clearSelectedObjectType();
        }

        clearSelectedObjects();
        curState = WAITING;
        redraw();
        return 1;
    }
    else if(key == 'a' && Fl::event_ctrl()) {
        selectAllObjects();
        curState = SELECTED;
        toolbar->clearSelectedObjectType();
        redraw();
        return 1;
    }
    else if(key == 'd') {
        // If you press 'd', reset focus back to center of site
        panToWorldCenter();

        calcArrowScreenPosition(centerX,
                                centerY,
                                doriX,
                                doriY,
                                arrowScreenX,
                                arrowScreenY);

        arrowAngleDegrees = calcArrowAngleDegrees(arrowScreenX,
                                                  arrowScreenY,
                                                  centerX,
                                                  centerY,
                                                  doriX,
                                                  doriY);

        calcArcInfo();
        redraw();
        return 1;
    }
    else if(key == 'a') {
        showArcs = !showArcs;
        redraw();
        return 1;
    }
    else if(key == 'g') {
        showGrid = !showGrid;
        redraw();
        return 1;
    }

    return 0;
}

int SiteEditor::handleMouseWheel(float centerX,
                                 float centerY,
                                 float doriX,
                                 float doriY,
                                 float arrowScreenX,
                                 float arrowScreenY)
{
    // add or subtract pixels per cell (perform zooming)
    // depending on the direction of the mouse scroll
    float zoomAmount = ZOOM_PIXEL_AMOUNT * Fl::event_dy();

    // Store the old world center, before zooming
    // so that after we zoom, we can pan back to our old coordinate,
    // and simulate zooming into the center of the screen
    float oldCenterX = screenCenterWorldX();
    float oldCenterY = screenCenterWorldY();

    pixelsPerCell -= zoomAmount;

    float newCenterX = screenCenterWorldX();
    float newCenterY = screenCenterWorldY();

    if(pixelsPerCell < MIN_PIXELS_PER_CELL) {
        pixelsPerCell = MIN_PIXELS_PER_CELL;
    }
    else if(pixelsPerCell > MAX_PIXELS_PER_CELL) {
        pixelsPerCell = MAX_PIXELS_PER_CELL;
    }
    else {
        worldPanX -= newCenterX - oldCenterX;
        worldPanY -= newCenterY - oldCenterY;
    }

    enforceSiteBounds();

    calcArrowScreenPosition(centerX,
                            centerY,
                            doriX,
                            doriY,
                            arrowScreenX,
                            arrowScreenY);

    arrowAngleDegrees = calcArrowAngleDegrees(arrowScreenX,
                                              arrowScreenY,
                                              centerX,
                                              centerY,
                                              doriX,
                                              doriY);

    calcArcInfo();

    redraw();
    return 1;
}

void SiteEditor::drawGrid() {
    char textBuffer[32];
    int textWidth, textHeight;
    fl_font(CUSTOM_FONT, 15);

    fl_color(FL_DARK1);
    fl_line_style(0, 1);

    float screenPanX = worldToScreen(worldPanX);
    float screenPanY = worldToScreen(worldPanY);

    // horizontal lines
    for(int i = 0; i < numGridCells; i++) {
        float offsetY = (pixelsPerCell * i) - screenPanY;
        if(offsetY > 0 && offsetY < h()) {
            fl_line(0, offsetY, w(), offsetY);
        }
    }

    // vertical lines
    for(int i = 0; i < numGridCells; i++) {
        float offsetX = (pixelsPerCell * i) - screenPanX;
        if(offsetX > 0 && offsetX < w()) {
            fl_line(offsetX, 0, offsetX, h());
        }
    }

    int index = siteMeterExtents / 2;
    int direction = -1;

    // horizontal meter labels
    for(int i = 0; i < numGridCells; i++) {
        if((i % (int)CELLS_PER_METER == 0)) {
            if(index == 0) {
                direction = 1;
            }
            else {
                float offsetY = (pixelsPerCell * i) - screenPanY;

                // Only draw labels that are on screen
                // and give a textheight buffer so the horizontal labels don't
                // collide with the vertical labels
                sprintf(textBuffer, "%d", index);
                fl_measure(textBuffer, textWidth, textHeight, 0);

                if(offsetY > 0 && offsetY < h() - textHeight) {
                    fl_color(FL_BLACK);
                    fl_draw(textBuffer, w() - textWidth - 5, offsetY + (textHeight / 2.0));
                }
            }

            index += direction;
        }
    }

    // reset index and direction
    index = siteMeterExtents / 2;
    direction = -1;

    // vertical meter labels
    for(int i = 0; i < numGridCells; i++) {
        if((i % (int)CELLS_PER_METER == 0)) {
            if(index == 0) {
                direction = 1;
            }
            else {
                float offsetX = (pixelsPerCell * i) - screenPanX;

                // Only draw labels that are on screen
                // and give a textwidth buffer so the horizontal labels don't
                // collide with the vertical labels
                sprintf(textBuffer, "%d", index);
                fl_measure(textBuffer, textWidth, textHeight, 0);

                if(offsetX > 0 && offsetX < w() - textWidth) {
                    fl_color(FL_BLACK);
                    fl_draw(textBuffer, offsetX - (textWidth / 2.0) - 5, h() - textHeight - 5);
                }
            }

            index += direction;
        }
    }
}

void SiteEditor::calcArcInfo() {
    // height of line's second point
    // is (1/2) *  width * tan(outside angle converted to radians)
    // outside angle = (180.0 - 360.0) / 2
    float outsideAngle = -180.0 / 2.0;
    outsideAngle *= (M_PI / 180.0); // convert to radians

    fl_color(GRID_COLOUR);
    fl_line_style(FL_SOLID, 2);

    float centerX = (siteMeterExtents / 2.0);
    float centerY = (siteMeterExtents / 2.0);

    for(int i = 0; i < numArcs; i++) {
        ArcInfo *info;

        // Make a new arc info object if the vector doesn't contain
        // information for the arc at this radius
        if(arcInfo.size() <= (unsigned)i) {
            ArcInfo newArcInfo;
            arcInfo.push_back(newArcInfo);
        }

        info = &arcInfo[i];

        float radius = (i+1) * 1.0f;

        // approximate the circle as a square:
        // we make the squares smaller by 75% to under approximate the circle
        // so that we won't hide the arcs near the edge of the screen
        float squareLeft = centerX - (radius * 0.75);
        float squareRight = centerX + (radius * 0.75);
        float squareTop = centerY - (radius * 0.75);
        float squareBottom = centerY + (radius * 0.75);

        // Don't draw the circle if the screen is completely inside of it
        if(squareLeft < worldPanX &&
           squareRight > worldPanX + screenToWorld(w()) &&
           squareTop < worldPanY &&
           squareBottom > worldPanY + screenToWorld(h()))
        {
            info->isOnScreen = false;
            continue;
        }

        // Expand the square back to regular size to check if it's outside the screen
        // If we leave it at the reduced 75%, then some rings are hidden when
        // we pan near the edge of the site
        squareLeft = centerX - radius;
        squareRight = centerX + radius;
        squareTop = centerY - radius;
        squareBottom = centerY + radius;

        // Don't draw the circle (represented as a square)
        // if it doesn't intersect with the screen at all
        if(squareRight < worldPanX ||
            squareBottom < worldPanY ||
            squareLeft > worldPanX + screenToWorld(w()) ||
            squareTop > worldPanY + screenToWorld(h()))
        {
            info->isOnScreen = false;
            continue;
        }

        info->isOnScreen = true;
        info->screenRadius = worldToScreen(radius);
    }
}

void SiteEditor::drawArcs() {

    int textHeight, textWidth;
    char textBuffer[32];

    fl_font(CUSTOM_FONT, 15);

    float doriX = worldToScreen((siteMeterExtents / 2.0) - worldPanX);
    float doriY = worldToScreen((siteMeterExtents / 2.0) - worldPanY);

    int count = 0;
    for(unsigned i = 0; i < arcInfo.size(); i++) {
        if(!arcInfo[i].isOnScreen) {
            continue;
        }

        count++;
        fl_color(LABEL_COLOUR);
        fl_circle(doriX, doriY, arcInfo[i].screenRadius);

        sprintf(textBuffer, "%d", (int)(i+1));
        fl_measure(textBuffer, textWidth, textHeight, 0);

        // top text
        fl_draw(textBuffer, doriX - (textWidth / 2.0), doriY - arcInfo[i].screenRadius - 2);

        // left text
        fl_draw(textBuffer, doriX - arcInfo[i].screenRadius - textWidth, doriY);

        // bottom text
        fl_draw(textBuffer, doriX - (textWidth / 2.0), doriY + arcInfo[i].screenRadius + textHeight);

        // right text
        fl_draw(textBuffer, doriX + arcInfo[i].screenRadius + 2, doriY);
    }
}


void SiteEditor::drawDori() {
    // Draw dori based on the units of cells per meter, and pixels per cell
    // Basically we want to always draw DORI at 1 meter by 1 meter
    // So we offset the drawing half a meter away from the center
    // And set the wheels to be 1/4 of a meter on each side
    int doriOffset = (CELLS_PER_METER) / 2 * pixelsPerCell;
    int doriWheelWidth = (doriOffset / 2);

    float doriX = worldToScreen((siteMeterExtents / 2.0) - worldPanX);
    float doriY = worldToScreen((siteMeterExtents / 2.0) - worldPanY);

    // chassis
    fl_draw_box(FL_FLAT_BOX, doriX - doriOffset, doriY - doriOffset, doriOffset * 2, doriOffset * 2, FL_BLUE);

    // wheels
    fl_draw_box(FL_FLAT_BOX, doriX - doriOffset, doriY - doriOffset, doriWheelWidth, doriOffset * 2, FL_BLACK);
    fl_draw_box(FL_FLAT_BOX, doriX + (doriOffset / 2), doriY - doriOffset, doriWheelWidth, doriOffset * 2, FL_BLACK);
}


void SiteEditor::drawCrosshair() {
    // Drawing crosshair to mouse cursor
    fl_line_style(FL_SOLID, 1);
    fl_color(CROSSHAIR_COLOUR);

    // horizontal lines
    fl_line(0, curMouseY, curMouseX - 5, curMouseY);
    fl_line(curMouseX + 5, curMouseY, w(), curMouseY);

    // vertical lines
    fl_line(curMouseX, 0, curMouseX, curMouseY - 5);
    fl_line(curMouseX, h(), curMouseX, curMouseY + 5);
}

void SiteEditor::drawDistanceText() {
    char textBuffer[128];
    int textWidth, textHeight;

    fl_font(CUSTOM_FONT, 15);

    sprintf(textBuffer, "%.3fm", curWorldDistance);
    fl_measure(textBuffer, textWidth, textHeight, 0);
    fl_draw_box(FL_FLAT_BOX, 5.0, 5.0, textWidth, textHeight, FL_BLACK);

    fl_color(FL_WHITE);
    fl_draw(textBuffer, 5.0, textHeight);
}

void SiteEditor::drawSelectionState() {
    // reset line style
    fl_line_style(0);

    switch(curState) {
    case DRAWING:
        if(newSiteObject) {
            newSiteObject->drawScreen(true, CELLS_PER_METER, pixelsPerCell, worldPanX, worldPanY);
        }
        break;
    case SELECTING:
        fl_line_style(1);
        fl_color(255, 255, 255);
        fl_begin_loop();
        fl_vertex(clickedMouseX, clickedMouseY);
        fl_vertex(curMouseX, clickedMouseY);
        fl_vertex(curMouseX, curMouseY);
        fl_vertex(clickedMouseX, curMouseY);
        fl_end_loop();
        break;
    case SELECTED:
        break;
    case WAITING:
        break;
    }
}

void SiteEditor::drawSiteObjects() {
    for(unsigned i = 0; i < siteObjects.size(); i++) {
        siteObjects[i]->drawScreen(true, CELLS_PER_METER, pixelsPerCell, worldPanX, worldPanY);
    }
}

// Draw an arrow that points to DORI's location (currently the center of the site)
void SiteEditor::drawArrow(float arrowScreenX, float arrowScreenY, float arrowAngleDegrees) {
    // Calculate the intersection points between the line
    // and the 4 lines of the screen
    // We do this to determine where to place the arrow that points to DORI


    // If arrow coordinates aren't set, then there's no arrow to draw
    if(arrowScreenX < 0 || arrowScreenY < 0)
    {
        return;
    }

    fl_push_matrix();

    fl_translate(arrowScreenX, arrowScreenY);
    fl_rotate(arrowAngleDegrees);

    fl_line_style(0, 5);
    fl_color(FL_BLACK);

    fl_begin_line();
    fl_vertex(0, -10);
    fl_vertex(10, 0);
    fl_vertex(0, 10);
    fl_end_line();

    fl_pop_matrix();
}

float SiteEditor::calcArrowAngleDegrees(float arrowX,
                                        float arrowY,
                                        float centerX,
                                        float centerY,
                                        float doriX,
                                        float doriY) {
    if(arrowX == -1 || arrowY == -1) {
        return -1;
    }

    float distanceX = doriX - centerX;
    float distanceY = centerY - doriY;

    return atan2(distanceY, distanceX) * (180.0 / M_PI);
}

void SiteEditor::calcArrowScreenPosition(float centerX,
                                         float centerY,
                                         float doriX,
                                         float doriY,
                                         float& arrowX,
                                         float& arrowY) {
    float screenWidthWorld = screenToWorld(w());
    float screenHeightWorld = screenToWorld(h());

    // fail fast if DORI is on screen
    if((doriX >= worldPanX && doriX <= worldPanX + screenWidthWorld) &&
       (doriY >= worldPanY && doriY <= worldPanY + screenHeightWorld))
    {
        arrowX = arrowY = -1;
        return;
    }

    // Distance to DORI from the center
    float distanceX = doriX - centerX;
    float distanceY = centerY - doriY;

    // Points that represent the line from the center of the screen to DORI
    float x0 = centerX;
    float y0 = centerY;
    float x1 = centerX + distanceX;
    float y1 = centerY - distanceY;

    // Calculate the slope for that line
    float slope = (y1  - y0) / (x1 - x0);
    float constant = y0 - (slope * x0);

    // Then calculate the intersects between that line and the four
    // lines that make up the screen
    float rightIntersectX = worldPanX + screenWidthWorld;
    float rightIntersectY = (slope * rightIntersectX) + constant;

    if(doriX > rightIntersectX &&
       rightIntersectY > worldPanY &&
       rightIntersectY < worldPanY + screenHeightWorld)
    {
        // For each arrow X and Y, clamp the arrow within the bounds of the screen by some offset
        arrowX = std::min(worldToScreen(rightIntersectX - worldPanX), w() - ARROW_SCREEN_OFFSET);
        arrowY = std::min(worldToScreen(rightIntersectY - worldPanY), h() - ARROW_SCREEN_OFFSET);
        return;
    }

    float leftIntersectX = worldPanX;
    float leftIntersectY = (slope * leftIntersectX) + constant;

    if(doriX < leftIntersectX &&
       leftIntersectY > worldPanY  &&
       leftIntersectY < worldPanY + screenHeightWorld)
    {
        arrowX = std::max(worldToScreen(leftIntersectX - worldPanX), ARROW_SCREEN_OFFSET);
        arrowY = std::max(worldToScreen(leftIntersectY - worldPanY), ARROW_SCREEN_OFFSET);
        return;
    }

    float topIntersectY = worldPanY;
    float topIntersectX = (topIntersectY - constant) / slope;

    if(doriY < topIntersectY &&
       topIntersectX > worldPanX  &&
       topIntersectX < worldPanX + screenWidthWorld)
    {
        arrowX = std::max(worldToScreen(topIntersectX - worldPanX), ARROW_SCREEN_OFFSET);
        arrowY = std::max(worldToScreen(topIntersectY - worldPanY), ARROW_SCREEN_OFFSET);
        return;
    }

    float bottomIntersectY = worldPanY + screenHeightWorld;
    float bottomIntersectX = (bottomIntersectY - constant) / slope;

    if(doriY > bottomIntersectY &&
       bottomIntersectX > worldPanX  &&
       bottomIntersectX < worldPanX + screenWidthWorld)
    {
        arrowX = std::min(worldToScreen(bottomIntersectX - worldPanX), w() - ARROW_SCREEN_OFFSET);
        arrowY = std::min(worldToScreen(bottomIntersectY - worldPanY), h() - ARROW_SCREEN_OFFSET);
        return;
    }
}

void SiteEditor::draw() {
    fl_draw_box(FL_FLAT_BOX, 0, 0, w(), h(), FL_DARK2);

    drawDori();

    if(showGrid) {
        drawGrid();
    }

    if(showArcs) {
        drawArcs();
    }

    drawSiteObjects();

    drawCrosshair();
    drawDistanceText();

    // handles drawing of the new site object or object selection
    drawSelectionState();

    drawArrow(arrowScreenX, arrowScreenY, arrowAngleDegrees);

    //reset line style
    fl_line_style(0, 1);
}


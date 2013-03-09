class SiteObject {
public:
    static void draw(Geometry* data, bool drawCenterPoint = true) {
        if(data == NULL) {
            fprintf(stderr, "couldn't draw, data was NULL\n");
            return;
        }

        switch(data->type) {
        case LINE: {
            Line *line = (Line *)data;
            fl_color(line->r, line->g, line->b);
            fl_line(line->screenLeft, line->screenTop, line->screenLeft + line->screenLengthX, line->screenTop + line->screenLengthY);

            break;
        }
        case RECT: {
            Rect *rect = (Rect *)data;
            fl_color(rect->r, rect->g, rect->b);
            fl_begin_loop();
            fl_vertex(rect->screenLeft, rect->screenTop);
            fl_vertex(rect->screenLeft + rect->screenWidth, rect->screenTop);
            fl_vertex(rect->screenLeft + rect->screenWidth, rect->screenTop + rect->screenHeight);
            fl_vertex(rect->screenLeft, rect->screenTop + rect->screenHeight);
            fl_vertex(rect->screenLeft, rect->screenTop);
            fl_end_loop();
            break;
        }
        case CIRCLE: {
            Circle *circle = (Circle *)data;
            fl_color(circle->r, circle->g, circle->b);
            fl_circle(circle->screenCenterX, circle->screenCenterY, circle->screenRadius);
            break;
        }
        default:
            fprintf(stderr, "unexpected geom type in draw()\n");
            return;
        }

        // draw the center point
        if(drawCenterPoint) {
            fl_color(255, 0, 0);
            fl_circle(data->screenCenterX, data->screenCenterY, 1);
        }
    }

    static void moveCenter(Geometry *data, int newCenterX, int newCenterY) {
        if(data == NULL) {
            fprintf(stderr, "couldn't do moveCenter(), data was NULL\n");
            return;
        }

        switch(data->type) {
        case LINE: {
            Line *line = (Line *)data;
            line->screenLeft = newCenterX - (line->screenLengthX / 2);
            line->screenTop =  newCenterY - (line->screenLengthY / 2);

            line->screenCenterX = newCenterX;
            line->screenCenterY = newCenterY;
            break;
        }
        case RECT: {
            Rect *rect = (Rect *)data;
            rect->screenLeft = newCenterX - (rect->screenWidth / 2);
            rect->screenTop =  newCenterY - (rect->screenHeight / 2);
            rect->screenCenterX = newCenterX;
            rect->screenCenterY = newCenterY;
            break;
        }
        case CIRCLE: {
            Circle *circle = (Circle *)data;
            circle->screenCenterX = newCenterX;
            circle->screenCenterY = newCenterY;
            break;
        }
        default:
            fprintf(stderr, "incorrect geom type in moveCenter()\n");
            break;
        }
    }
};

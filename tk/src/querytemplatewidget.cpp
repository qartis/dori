#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>
#include <vector>
#include <string>
#include <sqlite3.h>
#include <iostream>
#include "sparkline.h"
#include "querytemplatewidget.h"


#define QUERY_RESULT_LIMIT 5000

#define TO_STR(arg) #arg
#define EXPAND_AND_QUOTE(arg) TO_STR(arg)

QueryTemplateWidget::QueryTemplateWidget(sqlite3 *db, int X,int Y,int W,int H,const char*L) :
Fl_Widget(X,Y,W,H,L) {
    if (db == NULL) {
        std::cout << "db empty" << std::endl;
        return;
    }

    const char* queries[] = {
        "SELECT data FROM can WHERE id = \"diag\" AND sensor = \"temp5\" ORDER BY rowid LIMIT 5000" EXPAND_AND_QUOTE(QUERY_RESULT_LIMIT), 
        "SELECT data FROM can WHERE id = \"diag\" AND sensor = \"voltage\" ORDER BY rowid LIMIT 5000" EXPAND_AND_QUOTE(QUERY_RESULT_LIMIT), 
    };

    const char* titles[] = {
        "DIAG Temp 5",
        "DIAG Voltage",
        "DIAG Current",
    };

    for(unsigned int i = 0; i < sizeof(queries); i++) {
        QueryTemplate *queryTemplate = new QueryTemplate();
        queryTemplate->sparkline = new Fl_Sparkline(0, 0, 0, 0);
        queryTemplate->query = queries[i];
        queryTemplate->sparkline->label(titles[i]);
        queryTemplate->sparkline->align(FL_ALIGN_TOP);

        std::vector<QueryRow*> rowList;
        sqlite3_exec(db, queryTemplate->query, sqlite_cb, &rowList, NULL);

        if(rowList.size() == 0) {
            return;
        }

        if(rowList[0] == NULL || rowList[0]->cols.size() == 0) {
            return;
        }

        float** values = new float*[rowList[0]->cols.size()];
        for(unsigned int c = 0; c < rowList[0]->cols.size(); c++) {
            values[c] = new float[rowList.size()];

            for(unsigned int r = 0; r < rowList.size(); r++) {
                values[c][r] = rowList[r]->cols[c];
            }
        }

        queryTemplate->sparkline->setValues(values[0], rowList.size());
        queryTemplates.push_back(queryTemplate);
    }
}

int QueryTemplateWidget::sqlite_cb(void *arg, int ncols, char **cols, char**)
{
    std::vector<QueryRow*>* vals = (std::vector<QueryRow*>*)arg;

    QueryRow* row = new QueryRow();

    for (int i = 0; i < ncols; i++) {
        if(cols[i] == NULL) {
            continue;
        }

        int val = 0;
        if(sscanf(cols[i], "%x", &val) == 0) {
            continue;
        }

        row->cols.push_back(val);
    }

    vals->push_back(row);

    return 0;
}

void QueryTemplateWidget::draw() {
    int yOffset = 50;
    for(unsigned int i = 0; i < queryTemplates.size(); i++)
    {
        queryTemplates[i]->sparkline->resize(x() + 15, y() + yOffset, w(), 100);
        yOffset += queryTemplates[i]->sparkline->h() + 40;
    }
}

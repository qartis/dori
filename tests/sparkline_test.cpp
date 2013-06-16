#include <assert.h>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>

#include "sparkline.h"

int main()
{
    float values[50000];
    int num_values = 0;
    int tmp;

    FILE *f = fopen("sparkline_test.dat", "r");
    assert(f);

    while (fscanf(f, "%f", &values[num_values]) == 1) {
        num_values++;
    }

    printf("read %d values\n", num_values);

    Fl_Double_Window win(200, 200, "sparkline");
    Fl_Sparkline sparkline(0, 0, win.w(), win.h());
    sparkline.setValues(values, num_values);
    win.resizable(sparkline);
    win.show();
    return(Fl::run());
}

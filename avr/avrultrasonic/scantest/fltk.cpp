#include <fcntl.h>
#include <unistd.h>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <math.h>
#include <stdio.h>
#include <time.h>
#define BG_COLOR   45
#define TICK_COLOR 50
#define CIRC_COLOR 0

#define NUM_READINGS 2048
int readings[NUM_READINGS];
int idx;

void readdata(int fd, void *unused)
{
    static char buf[128] = {0};
    char *nl;
    int i;
    int rc;
    static int len = 0;

    rc = read(fd, (void *)(buf + len), sizeof(buf) - len);
    if (rc < 1) {
        perror("read");
        printf("%d bytes\n", sizeof(buf) - len);
        Fl::remove_fd(0);
        //exit(0);
    } else {
        printf("read %d bytes\n", rc);
    }
    len += rc;

    i = 0;

    for (;;) {
        nl = (char *)memchr(buf + i, '\n', len - i);
        if (nl == NULL) {
            break;
        }

        *nl = '\0';
        fprintf(stderr, "packet: '%s'\n", buf + i);
        float reading;
        if (sscanf(buf + i, "%f cm", &reading)) {
            readings[idx] = reading;
            idx++;
            idx %= NUM_READINGS;
        }
        fflush(stderr);

        i = nl - buf + 1;
    }

    memcpy(buf, buf + i, len - i);
    len = len - i;
}

class MyTimer : public Fl_Box {
    void draw() {
        // COMPUTE NEW COORDS OF LINE
        static long start = time(NULL);
        static int counter = 0;
        long tick = time(NULL) - start;
        char secs[80]; sprintf(secs, "%02ld:%02ld", tick/60, tick%60);
        float pi = 3.14 - (((tick % 60) / 60.0) * 6.28);
        int radius = h() / 2;
        int x1 = (int)(x() + w()/2),
            y1 = (int)(y() + h()/2),
            x2 = (int)(x1 + (sin(pi) * radius)),
            y2 = (int)(y1 + (cos(pi) * radius));

        // TELL BASE WIDGET TO DRAW ITS BACKGROUND
        Fl_Box::draw();

        // DRAW 'SECOND HAND' OVER WIDGET'S BACKGROUND
        fl_color(TICK_COLOR);
        fl_line(x1, y1, x2, y2);
        fl_color(CIRC_COLOR);
        fl_pie(x1-5, y1-5, 10, 10, 0.0, 360.0);

        // DRAW TIMER TEXT STRING
        fl_color(TICK_COLOR);
        fl_font(FL_HELVETICA,16);
        fl_draw(secs, x()+4, y()+h()-4);

        fl_color(FL_BLACK);
        int i;
        for(i=0;i<idx;i++){
            int x = w() / 2 + cos(i * M_PI * 2.0 / idx) * readings[i] * 0.5;
            int y = h() / 2 + sin(i * M_PI * 2.0 / idx) * readings[i] * 0.5;
            fl_rectf(x, y, 5, 5);
        }
    }

    static void Timer_CB(void *userdata) {
        MyTimer *o = (MyTimer*)userdata;
        o->redraw();
        Fl::repeat_timeout(0.25, Timer_CB, userdata);
    }
public:
    // CONSTRUCTOR
    MyTimer(int X,int Y,int W,int H,const char*L=0) : Fl_Box(X,Y,W,H,L) {
        box(FL_FLAT_BOX);
        color(BG_COLOR);
        Fl::add_timeout(0.25, Timer_CB, (void*)this);
    }
};

int main() {
    //int fd = open("/dev/ttyUSB1", O_RDONLY | O_NOCTTY);
    perror("open");

    Fl::add_fd(0, readdata, NULL);

    Fl_Double_Window win(1000, 1000);
    MyTimer tim(0, 0, win.w(), win.h());
    win.show();
    return(Fl::run());
}

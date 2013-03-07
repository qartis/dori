/* 
   Copyright 2006-2009 by the Yury Fedorchenko.
   
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with main.c; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
  
  Please report all bugs and problems to "yuryfdr@users.sourceforge.net".
   
*/

#include "Fl_Custom_Cursor.H"

#include <FL/Fl_Pixmap.H>
#include <FL/fl_draw.H>

#include <FL/Fl.H>
#include <FL/x.H>

#ifndef WIN32

static XcursorImage* create_cursor_image(unsigned char *src,int w,int h,int x,int y)
{
  XcursorImage *xcimage;
  XcursorPixel *dest;

  xcimage = XcursorImageCreate(w,h);

  xcimage->xhot = x;
  xcimage->yhot = y;

  dest = xcimage->pixels;

  for (int j = 0; j < h; j++){
    for (int i = 0; i < w; i++){
        *dest = ((src[0]==255&&src[1]==255&&src[2]==254)?(0x00 << 24):(0xff <<24)) | (src[0] << 16) | (src[1] << 8) | src[2];
        src += 3;
        dest++;
    }
  }
  return xcimage;
}

#else

static HCURSOR CreateAlphaCursor(unsigned char *isrc,int w,int h,int x,int y)
{
  HDC hMemDC;
  DWORD dwWidth, dwHeight;
  BITMAPV5HEADER bi;
  HBITMAP hBitmap;//, hOldBitmap;
  void *lpBits;
  HCURSOR hAlphaCursor = NULL;

  dwWidth = 32; // width of cursor
  dwHeight = 32; // height of cursor

  ZeroMemory(&bi,sizeof(BITMAPV5HEADER));
  bi.bV5Size = sizeof(BITMAPV5HEADER);
  bi.bV5Width = dwWidth;
  bi.bV5Height = dwHeight;
  bi.bV5Planes = 1;
  bi.bV5BitCount = 32;
  bi.bV5Compression = BI_BITFIELDS;
  // The following mask specification specifies a supported 32 BPP
  // alpha format for Windows XP.
  bi.bV5RedMask = 0x00FF0000;
  bi.bV5GreenMask = 0x0000FF00;
  bi.bV5BlueMask = 0x000000FF;
  bi.bV5AlphaMask = 0xFF000000;

  HDC hdc;
  hdc = GetDC(NULL);

  // Create the DIB section with an alpha channel.
  hBitmap = CreateDIBSection(hdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS,(void **)&lpBits, NULL, (DWORD)0);

//  hMemDC = CreateCompatibleDC(hdc);
  ReleaseDC(NULL,hdc);

  DWORD *lpdwPixel;
  lpdwPixel = (DWORD *)lpBits;
  for (DWORD i=0;i<dwWidth;i++)
    for (DWORD j=0;j<dwHeight;j++)
    {
    unsigned char* src = isrc + 3*(j + dwHeight*(dwWidth-i-1));
    *lpdwPixel = ((src[0]==255&&src[1]==255&&src[2]==254)?(0x00 << 24):(0xff <<24)) | (src[0] << 16) | (src[1] << 8) | src[2];
    lpdwPixel++;
    }

  // Create an empty mask bitmap
  HBITMAP hMonoBitmap = CreateBitmap(dwWidth,dwHeight,1,1,NULL);

  ICONINFO ii;
  ii.fIcon = FALSE; // Change fIcon to TRUE to create an alpha icon
  ii.xHotspot = x;
  ii.yHotspot = y;
  ii.hbmMask = hMonoBitmap;
  ii.hbmColor = hBitmap;

  // Create the alpha cursor with the alpha DIB section
  hAlphaCursor = CreateIconIndirect(&ii);

  DeleteObject(hBitmap);
  DeleteObject(hMonoBitmap);

  return hAlphaCursor;
}
#endif

Fl_Custom_Cursor::Fl_Custom_Cursor(Fl_Image *image,int x,int y):cimage(image->copy()),created(false),hx(x),hy(y)
{
}
Fl_Custom_Cursor::Fl_Custom_Cursor(const char * const *data,int x,int y):created(false),hx(x),hy(y)
{
  cimage = new Fl_Pixmap(data);
}

void Fl_Custom_Cursor::create(){
#ifndef WIN32
  fl_open_display();
  Fl::first_window()->make_current();
  int w=cimage->w(),h=cimage->h();
#else
  int w=32,h=32;//win32 unfortunatelly don't support other cursors size 
#endif
  Fl_Offscreen offscr = fl_create_offscreen(w,h);
  fl_begin_offscreen(offscr);
  fl_color(fl_rgb_color(255,255,254));
  fl_rectf(0,0,w,h);
  cimage->draw(0,0);
  unsigned char* pixbuf = fl_read_image(NULL, 0, 0, w, h, 0);
  fl_end_offscreen();
  fl_delete_offscreen(offscr);
#ifndef WIN32
  XcursorImage *xcimage;
  xcimage = create_cursor_image(pixbuf, w, h, hx, hy);
  xcursor = XcursorImageLoadCursor(fl_display, xcimage);
  XcursorImageDestroy (xcimage);
#else
  xcursor = CreateAlphaCursor(pixbuf, w, h, hx, hy);
#endif  
  delete[] pixbuf;
  created = true;
}

void Fl_Custom_Cursor::cursor(Fl_Window* w){
  if(!created)create();
#ifndef WIN32
  //  printf("%p %d\n",fl_display,fl_xid(w->window()));
  XDefineCursor(fl_display,fl_xid(w),xcursor);
#else
  Fl_X::i(w)->cursor=xcursor;  
  SetCursor(xcursor);
#endif  
}


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

#include <FL/Fl_Window.H>
#include <FL/Fl_Image.H>

#if defined( WIN32 ) || defined (__APPLE__)
#include <FL/x.H>
#else
#include <X11/Xcursor/Xcursor.h>
#endif

class Fl_Custom_Cursor{
  public:
  Fl_Custom_Cursor(Fl_Image*,int,int);
  Fl_Custom_Cursor(const char* const * xpm_data,int,int);
  void cursor(Fl_Window*);
  
  private:
  void create();
  Fl_Image* cimage;
  bool created;
  int hx,hy;
#if defined( WIN32 )
  HCURSOR xcursor;
#else
  Cursor xcursor;  
#endif
};

/* 
   Copyright 2006-2008 by the Yury Fedorchenko.
   
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

#include "basic_ball.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define _(a) a

using namespace controls3d;
using namespace mvct;

basic_ball::basic_ball()
    : b_zoom_frame(0),
      arcball(mvct::XYZ(0,0,0),1.0), trcontrol(), zoomcontrol(), dollycontrol(10.0),
      transform(), currenttr(controls3d::NoTransform),projtype(Orthographic),
      fovy(45.0), aspect_ratio(1), near_cp(1.0), far_cp(1000.0),
      umin(-1.0), umax(1.0), vmin(-1.0), vmax(1.0),
      cenx(0), ceny(0), cenz(0), eyex(0), eyey(0), eyez(50.0), upx(0), upy(1), upz(0),
      Rmax(1)
{
    transform.reset();
};

void basic_ball::reshape(){
  double w__=view_width(),h__=view_height();
  cenx = ceny = 0.0;
  cenz = 0.0;
  eyex = eyey = 0.0;
  eyez = 1;//20.0 * Rmax();
  upx = upz = 0.0;
  upy = 1.0;

  aspect_ratio = w__/h__;
  near_cp = -1000.0* Rmax;
  far_cp = 1000.0* Rmax;
  if(Perspective == projtype){
  // Perspective projection
    fovy = 145.0;
    projtype = Perspective;
    near_cp = 1. * Rmax;
    far_cp = 10.0* Rmax;
  }else{
    // Orthographic projection
    if (aspect_ratio >= 1.) {
      vmin = -1*Rmax;
      vmax = 1*Rmax;
      umin = vmin * aspect_ratio;
      umax = vmax * aspect_ratio;
    } else {
      vmin = -1*Rmax/ aspect_ratio;
      vmax = 1*Rmax / aspect_ratio;
      umin = vmin * aspect_ratio;
      umax = vmax * aspect_ratio;
    }
    projtype = Orthographic;
  }
  // GL calls.
  glViewport(0, 0, w__, h__);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if ( projtype == Perspective ) 
    gluPerspective(fovy,aspect_ratio,near_cp,far_cp);
  else if ( projtype == Orthographic)
    glOrtho(umin,umax,vmin,vmax,near_cp,far_cp);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(eyex,eyey,eyez,cenx,ceny,cenz,upx,upy,upz);
  
  dist = sqrt( pow2(cenx-eyex) + pow2(ceny-eyey) + pow2(cenz-eyez) );
}


void basic_ball::handle_rotate(mvct::XY mouse,bool push,bool release){
  if ( currenttr != controls3d::NoTransform && currenttr != controls3d::Rotate ) return;
    currenttr = controls3d::Rotate;
  // Transform coords to lie between -1 to 1
/*    x = ( double(event->x << 1) / get_width() ) - 1.0;
  y = ( -double(event->y << 1) / get_height() ) + 1.0;*/
  double y = ( -2.*mouse.y ) + 1.0;
  double x = ( 2.*mouse.x ) - 1.0;
  static double my;
  static double mx;
  if( mouse.y<0.05 ){
    if(push){
      cursor(CursorRotateZ);
      mx=x;
    }else{
      double angle=M_PI*(x-mx);
      transform.rotate_z(angle);
      transRotate.rotate_z(angle);
      mx=x;
    }
  }else if( mouse.y>0.95 ){
    if(push){
      mx=x;
      cursor(CursorRotateY);
    }else{
      double angle=-M_PI*(x-mx);
      transform.rotate_y(angle);
      transRotate.rotate_y(angle);
      mx=x;
    }
  }else if( mouse.x<0.05 || mouse.x>0.95 ){
    if(push){
      my=y;
      cursor(CursorRotateX);
    }else{
      double angle=M_PI*(y-my);
      transform.rotate_x(angle);
      transRotate.rotate_x(angle);
      my=y;
    }
  }else{
    arcball.mouse(XYZ(x,y,0));
  
    if( push ){
      arcball.begin_drag();
      cursor(CursorRotate);
    } else if ( release ) {
      arcball.end_drag();
      transform.rotate(arcball.quaternion_value());
      transRotate.rotate(arcball.quaternion_value());
      arcball.reset(); currenttr = controls3d::NoTransform;
      cursor();
    }
  }
  if( release ) cursor();
}

void basic_ball::handle_pan(mvct::XY mouse,bool push,bool release){
  if ( currenttr != controls3d::NoTransform && currenttr != controls3d::Pan ) return;
    currenttr = controls3d::Pan;
    // Transform coords to lie between -1 to 1
    double x = ( 2.*mouse.x ) - 1.0;
    double y = ( -2.*mouse.y ) + 1.0;
//    x = ( double(Fl::event_x() << 1) / w() ) - 1.0;
//    y = ( -double(Fl::event_y() << 1) / h() ) + 1.0;
    
    // Adjust the x and y values so that moving mouse by 1 pixel
    // on screen moves point under mouse by 1 pixel
    double tx=0, ty=0;
    if ( projtype == Orthographic ) {
      // Orthographic projection
      tx = umin*(1.0-x)*0.5 + umax*(1.0+x)*0.5;
      ty = vmin*(1.0-y)*0.5 + vmax*(1.0+y)*0.5;
    }
    else if ( projtype == Perspective ) {
      // Perspective projection
      tx = x*dist*tan(mvct::deg2rad(fovy*aspect_ratio*0.5));
      ty = y*dist*tan(mvct::deg2rad(fovy*0.5));
    }
    trcontrol.mouse(XYZ(tx,ty,0));
    
    if( push ){
      trcontrol.begin_drag();
      cursor(CursorMove);
    }else if( release ){
      trcontrol.end_drag();
      transform.translate(trcontrol.trans_value());// Update the combined transformation
      trcontrol.reset(); currenttr = controls3d::NoTransform;
      cursor();
    }

}
void basic_ball::handle_zoom(mvct::XY mouse,bool push,bool release){
  if ( currenttr != controls3d::NoTransform && currenttr != controls3d::Zoom ) return;
  currenttr = controls3d::Zoom;
  // Transform coords to lie between -1 to 1
  double z = ( 2.*mouse.x ) - 1.0;
  
  zoomcontrol.mouse(XYZ(z,z,z));
  
  if ( push ){
    zoomcontrol.begin_drag();
    cursor(CursorZoom);
  }else if ( release ){
    zoomcontrol.end_drag();
    transform.scale(zoomcontrol.zoom_value());
    zoomcontrol.reset(); currenttr = controls3d::NoTransform;
    cursor();
  }

}
void basic_ball::handle_dolly(mvct::XY mouse,bool push,bool release){
  if ( currenttr != controls3d::NoTransform && currenttr != controls3d::Dolly ) return;
  currenttr = controls3d::Dolly;
  // Transform coords to lie between -1 to 1
  //z = ( double(Fl::event_x() << 1) / w() ) - 1.0;
  double z = ( 2.*mouse.x ) - 1.0;
  dollycontrol.mouse(XYZ(0,0,z));// dollycontrol.update();
    
  if ( push ) dollycontrol.begin_drag();
  else if ( release ) {
    dollycontrol.end_drag();
    // Shouldn't reset since the value is directly used for transformations
    // separate from other transformations
    currenttr = controls3d::NoTransform;
  }

}

void basic_ball::views3D(Views3D cv)
{
  if(Nochange!=cv){
    transRotate.reset();
  }
  switch(cv) {
  case Left:
    break;
  case Right:
    transRotate.post_rotate(mvct::Quaternion(M_PI,XYZ(0,1.,0)));
    break;
  case Top:
    transRotate.post_rotate(mvct::Quaternion(M_PI_2,XYZ(1,0,0)));
    break;
  case Bottom:
    transRotate.post_rotate(mvct::Quaternion(-M_PI_2,XYZ(1,0,0)));
    break;
  case Front:
    transRotate.rotate(mvct::Quaternion(M_PI_2,XYZ(0,1,0)));
    break;
  case Back:
    transRotate.rotate(mvct::Quaternion(-M_PI_2,XYZ(0,1,0)));
    break;
  case LeftTopFront:
    transRotate.post_rotate(mvct::Quaternion(M_PI_4,XYZ(0,1,0)));
    transRotate.post_rotate(mvct::Quaternion(asin(1./sqrt(3.)),XYZ(1,0,1)));
    break;
  case RightTopFront:
    transRotate.rotate(mvct::Quaternion(3.*M_PI_4,XYZ(0,1,0))*mvct::Quaternion(-asin(1./sqrt(3.)),XYZ(1,0,-1)));
    break;
  case LeftBottomFront:
    transRotate.rotate(mvct::Quaternion(M_PI_4,XYZ(0,1,0))*mvct::Quaternion(-asin(1./sqrt(3.)),XYZ(1,0,1)));
    break;
  case RightBottomFront:
    transRotate.rotate(mvct::Quaternion(3.*M_PI_4,XYZ(0,1,0))*mvct::Quaternion(asin(1./sqrt(3.)),XYZ(1,0,-1)));
    break;
  case LeftTopBack:
    transRotate.rotate(mvct::Quaternion(-M_PI_4,XYZ(0,1,0))*mvct::Quaternion(asin(1./sqrt(3.)),XYZ(1,0,-1)));
    break;
  case RightTopBack:
    transRotate.rotate(mvct::Quaternion(-3.*M_PI_4,XYZ(0,1,0))*mvct::Quaternion(-asin(1./sqrt(3.)),XYZ(1,0,1)));
    break;
  case LeftBottomBack:
    transRotate.rotate(mvct::Quaternion(-M_PI_4,XYZ(0,1,0))*mvct::Quaternion(-asin(1./sqrt(3.)),XYZ(1,0,-1)));
    break;
  case RightBottomBack:
    transRotate.rotate(mvct::Quaternion(-3.*M_PI_4,XYZ(0,1,0))*mvct::Quaternion(asin(1./sqrt(3.)),XYZ(1,0,1)));
    break;
  }
  transform = transRotate;
  //queue_draw();
}

void basic_ball::arcball_transformRotate()const
{
  if(controls3d::Rotate == currenttr) {
    double mat[16];
    arcball.value().fill_array_row_major(mat);
    glMultMatrixd(mat);
  }
  transRotate.apply();
}
void basic_ball::arcball_transform()const{                     // Apply the arcball transformation
  double mat[16];
  
  // Do the dollying separately before everything else
  //glTranslated(0,0,dollycontrol.dolly_value());
  switch ( currenttr ) {
  case controls3d::Pan :
    trcontrol.value().fill_array_row_major(mat);
    glMultMatrixd(mat);
    break;
  case controls3d::Zoom :
    zoomcontrol.value().fill_array_row_major(mat);
    glMultMatrixd(mat);
    break;
  case controls3d::Rotate :
    arcball.value().fill_array_row_major(mat);
    glMultMatrixd(mat);
    break;
  default:
    break;
  }
  transform.apply();
}

void basic_ball::draw_axis()
{
  GLboolean lw=glIsEnabled(GL_LIGHTING);
  glDisable(GL_LIGHTING);
  glColor4f(1., 0., 0.,.7);
  glBegin (GL_LINE_STRIP);
  glVertex3f(-3. * Rmax, 0., 0.);
  glVertex3f(3. * Rmax, 0., 0.);
  glEnd ();
  glColor4f(0., 1., 0.,.7);
  glBegin (GL_LINE_STRIP);
  glVertex3f(0., -3. * Rmax, 0.);
  glVertex3f(0., 3. * Rmax, 0.);
  glEnd ();
  glColor4f(0., 0., 1.,.7);
  glBegin (GL_LINE_STRIP);
  glVertex3f(0., 0., -3. * Rmax);
  glVertex3f(0., 0., 3. * Rmax);
  glEnd ();
  if(lw)glEnable(GL_LIGHTING);
}

void basic_ball::draw_zoom_frame(){
  if(b_zoom_frame){
    glDisable (GL_DEPTH_TEST);
    glMatrixMode (GL_PROJECTION);
    glPushMatrix ();
    glLoadIdentity ();
    gluOrtho2D (0,view_width(),view_height(),0);
    glMatrixMode (GL_MODELVIEW);
    glPushMatrix ();
    glLoadIdentity ();
    glBegin (GL_LINE_LOOP);
    glColor4d (1., 1., 1., 0.9);
    glVertex3d (mouse.x, mouse.y, 0);
    glVertex3d (d.x, mouse.y, 0);
    glVertex3d (d.x, d.y, 0);
    glVertex3d (mouse.x, d.y, 0);
    glEnd ();
    glPopMatrix ();
    glMatrixMode (GL_PROJECTION);
    glPopMatrix ();
    glMatrixMode (GL_MODELVIEW);
    glEnable (GL_DEPTH_TEST);
  }
}


void basic_ball::draw_zoom_scale(){
  bool lightrest = glIsEnabled(GL_LIGHTING);
  glDisable (GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  double det=pow(transform.matrix().determinant(),1./3.);
/*  if (aspect_ratio >= 1.) {
    vmin = -1*Rmax;
    vmax = 1*Rmax;
    umin = vmin * aspect_ratio;
    umax = vmax * aspect_ratio;
  } else {
    vmin = -1*Rmax/ aspect_ratio;
    vmax = 1*Rmax / aspect_ratio;
    umin = vmin * aspect_ratio;
    umax = vmax * aspect_ratio;
  }*/
//  double minix = -det*Rmax,maxix=det*Rmax;
//  gluOrtho2D (minix,maxix,view_height(),0);
  int text_size=view_width()/100;
  gluOrtho2D (-1.5,1.5,view_height()+20,20);
  glMatrixMode (GL_MODELVIEW);
  glMatrixMode (GL_MODELVIEW);
  glPushMatrix ();
  glLoadIdentity ();
    glColor4d (0., 0., 0., 0.8);
  text(text_size,0,view_height()-12,0.,"0");
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  glBegin(GL_QUADS);
    glColor4d (1., 1., 1., 0.8);
#if 0
    glVertex3d(.5,view_height()-20,0);
    glVertex3d(.5,view_height()-10,0);
    glVertex3d(1.,view_height()-10,0);
    glVertex3d(1.,view_height()-20,0);
#else
    glVertex3d(0,view_height()-10,0);
    glVertex3d(0,view_height(),0);
    glVertex3d(.5,view_height(),0);
    glVertex3d(.5,view_height()-10,0);
#endif
    glColor4d (0., 0., 0., 0.8);
    glVertex3d(.5,view_height()-10,0);
    glVertex3d(.5,view_height(),0);
    glVertex3d(1.,view_height(),0);
    glVertex3d(1.,view_height()-10,0);
  glEnd();
/*  glBegin(GL_LINES);
    glVertex3d(.5,view_height()-20,0);
    glVertex3d(.5,view_height()-10,0);
  glEnd();*/
    text(text_size,1.,view_height()-12,0.,"%.6g",0.6666666666666666666666*Rmax/det);
  for(int i=0;i<10;i+=2){
  glBegin(GL_QUADS);
    glColor4d (1., 1., 1., 0.8);
#if 0
    glVertex3d(-.1*i,view_height()-20,0);
    glVertex3d(-.1*i,view_height(),0);
    glVertex3d(-.1*i-.1,view_height(),0);
    glVertex3d(-.1*i-.1,view_height()-20,0);
    
    glVertex3d(-.1*i-.1,view_height()-20,0);
    glVertex3d(-.1*i-.1,view_height()-10,0);
    glVertex3d(-.1*i-.2,view_height()-10,0);
    glVertex3d(-.1*i-.2,view_height()-20,0);
#else
    glVertex3d(-.1*i,view_height()-10,0);
    glVertex3d(-.1*i,view_height(),0);
    glVertex3d(-.1*i-.1,view_height(),0);
    glVertex3d(-.1*i-.1,view_height()-10,0);
#endif

    glColor4d (0., 0., 0., 0.8);
    glVertex3d(-.1*i-.1,view_height()-10,0);
    glVertex3d(-.1*i-.1,view_height(),0);
    glVertex3d(-.1*i-.2,view_height(),0);
    glVertex3d(-.1*i-.2,view_height()-10,0);
  glEnd();
    double sval=0.6666666666666666666666*Rmax/det*(.1*i+.2);
    if(sval < 1. || sval >1.e+6 )text(text_size,-.1*i-.2,view_height()-12,0.,"%.4e",sval);
    else text(text_size,-.1*i-.2,view_height()-12,0.,"%g",sval);
  }
  
  glBegin(GL_LINES);
  for(int i=0;i<10;i++){
    glColor4d (0., 0., 0., 0.8);
    glVertex3d(-.1*i,view_height()-((i%2)?10:12),0.001);
    glVertex3d(-.1*i,view_height(),0);
  }
  glEnd();
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  glPopMatrix ();
  glMatrixMode (GL_PROJECTION);
  glPopMatrix ();
  glMatrixMode (GL_MODELVIEW);
  glEnable(GL_DEPTH_TEST);
  if(lightrest)glEnable(GL_LIGHTING);
}


void basic_ball::draw_3d_orbit(){
  GLUquadric* sph =gluNewQuadric();
  GLUquadric* cylX=gluNewQuadric();
  GLUquadric* conX=gluNewQuadric();
  GLUquadric* cylY=gluNewQuadric();
  GLUquadric* conY=gluNewQuadric();
  GLUquadric* cylZ=gluNewQuadric();
  GLUquadric* conZ=gluNewQuadric();

  double w=view_width(),h=view_height();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  if ( projtype == Perspective ) 
    gluPerspective(fovy,aspect_ratio,near_cp,far_cp);
  else if ( projtype == Orthographic)
    //glOrtho(umin,umax,vmin,vmax,near_cp,far_cp);
  glOrtho(-fabs(Rmax)*w/h,fabs(Rmax)*w/h,
          -fabs(Rmax),        fabs(Rmax),
          -fabs(Rmax)*10.2,fabs(Rmax)*10.2);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  glLoadIdentity();
  glTranslatef(Rmax*w/h-Rmax/3.,2.*Rmax/3.,+5.*Rmax/*-5.*Rmax*/);

  arcball_transformRotate();  

  glPushAttrib(GL_LIGHTING_BIT);
  glEnable(GL_LIGHTING);
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
  GLfloat m_specular[]={1.0,1.0,1.0,.5};
  GLfloat m_shine=0.7;
  glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,m_specular);
  glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,m_shine*128.0);
  glColor4f(.7,.7,.0,1.0);
  gluSphere(sph,Rmax/36.,15,15);
  glColor4f(.8,0.,.0,1);
  glPushMatrix();
  glRotatef(90.,0.,1.,0.);
  gluCylinder(cylX,Rmax/74.,Rmax/74.,Rmax/8.,15,5);
  glTranslatef(0,0,Rmax/9.);
  gluCylinder(conX,Rmax/48.,0.,Rmax/10.,15,5);
  glColor4f(.2,0.,.0,1.0);
  stext(12,0.,0.,.15*Rmax,_("X"));
  glPopMatrix();

  glColor4f(0.,.8,0.,1.0);
  glPushMatrix();
  glRotatef(-90.,1.,0.,0.);
  gluCylinder(cylY,Rmax/74.,Rmax/74.,Rmax/8.,15,5);
  glTranslatef(0,0,Rmax/9.);
  gluCylinder(conY,Rmax/48.,0.,Rmax/10.,15,5);
  glColor4f(0.,.2,0.,1.0);
  stext(12,0.,0.,.15*Rmax,_("Y"));
  glPopMatrix();

  glColor4f(0.,0.,.8,1.0);
  glPushMatrix();
  gluCylinder(cylZ,Rmax/74.,Rmax/74.,Rmax/8.,15,5);
  glTranslatef(0,0,Rmax/9.);
  gluCylinder(conZ,Rmax/48.,0.,Rmax/10.,15,5);
  glColor4f(0.,0.,.2,1.0);
  stext(12.,0.,0.,.15*Rmax,_("Z"));
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glPopAttrib();
  glPopMatrix();
  gluDeleteQuadric(sph);
  gluDeleteQuadric(cylX);
  gluDeleteQuadric(conX);
  gluDeleteQuadric(cylY);
  gluDeleteQuadric(conY);
  gluDeleteQuadric(cylZ);
  gluDeleteQuadric(conZ);
  glDisable(GL_LIGHTING);
  glDisable(GL_COLOR_MATERIAL);
}

void basic_ball::text(GLfloat x, GLfloat y, GLfloat z,const char *format, ...)const{
  va_list args;
  char buffer[1024];
  va_start(args,format);
  vsnprintf(buffer,1024,format, args);
  va_end(args);
  stext(12,x,y,z,buffer);
}
void basic_ball::text(int size, GLfloat x, GLfloat y, GLfloat z,const char *format, ...)const{
  va_list args;
  char buffer[1024];
  va_start(args,format);
  vsnprintf(buffer,1024,format, args);
  va_end(args);
  stext(size,x,y,z,buffer);
}

#if 0
// legacy drawing 

#define LG_NSEGS 4
#define NSEGS (1<<LG_NSEGS)
#define CIRCSEGS 32
#define HALFCIRCSEGS 16

inline void glVertex(const XYZ&v){
  glVertex3dv(reinterpret_cast<const double*>(&v));
}
inline void glColor(const XYZ&v){
  glColor3dv(reinterpret_cast<const double*>(&v));
}
inline void glScale(const XYZ&v){
  glScaled(v[0],v[1],v[3]);
}


static void unitCircle(void)
{
  // Draw a unit circle in the XY plane, centered at the origin
  float dtheta = 2.0*M_PI/32.0;
  
  glBegin (GL_LINE_LOOP);
  for (int i=0; i < 32; ++i)
    glVertex3d (cos (i*dtheta), sin (i*dtheta), 0.0);
  glEnd ();
}

/*
 * Draw a circle with the given normal, center and radius 
 */
static void drawCircle (const XYZ& center, const XYZ& normal, double radius)
{
  // First find the coordinate axis centered at the circle center.
  // The normal will be the Z axis.
  XYZ xaxis, yaxis, zaxis;
  
  zaxis = normalized (normal);
  xaxis = XYZ (0,1,0) ^ zaxis;
  if ( xaxis.abs2() < 1.0e-5 ) 
    xaxis.set (1,0,0);
  yaxis = zaxis ^ xaxis;
  
  // Circle will be on the XY plane, defined by the axis system 
  // just computed 
  XYZ pts[CIRCSEGS+1];
  double theta = 0.0, dtheta = M_PI/HALFCIRCSEGS;
  double costheta, sintheta;
  for (int i=0; i < (HALFCIRCSEGS >> 2); ++i )
    {
      costheta = radius*cos (theta); sintheta = radius*sin (theta);
      pts[0] = center + costheta*xaxis + sintheta*yaxis;
      pts[HALFCIRCSEGS-i] = center - costheta*xaxis + sintheta*yaxis;
      pts[HALFCIRCSEGS+i] = center - costheta*xaxis - sintheta*yaxis;
      pts[CIRCSEGS-i] = center + costheta*xaxis - sintheta*yaxis;
      theta += dtheta;
    }
  
  glBegin (GL_LINE_LOOP);
  {
    for (int i=0; i < CIRCSEGS; ++i)
      glVertex (pts[i]);
  }
  glEnd ();
}

/*
 * Halve arc between unit vectors v1 and v2.
 * Assumes that v1 and v3 are unit vectors
 */
static XYZ bisect (const XYZ& v1, const XYZ& v2)
{
  XYZ v = v1 + v2;
  double Nv = v.abs2();
  
  if (Nv < 1.0e-5) v.set (0.0, 0.0, 1.0);
  else v /= sqrt(Nv);
  return v;
}

/*
 *  Draw an arc defined by its ends.
 */
static void drawAnyArc (const XYZ& vFrom, const XYZ& vTo)
{
  int i;
  XYZ pts[NSEGS+1];
  double dot;
  
  pts[0] = vFrom;
  pts[1] = pts[NSEGS] = vTo;
  for (i=0; i < LG_NSEGS; ++i) pts[1] = bisect (pts[0], pts[1]);
  
  dot = 2.0*(pts[0]*pts[1]);
  for (i=2; i < NSEGS; ++i)
    pts[i] = pts[i-1]*dot - pts[i-2];
  
  glBegin (GL_LINE_STRIP);
  for (i=0; i <= NSEGS; ++i)
    glVertex (pts[i]);
  glEnd ();
}

/*
 * Draw the arc of a semi-circle defined by its axis.
 */
static void drawHalfArc(const XYZ& n)
{
  XYZ p, m;
  
  if ( fabs(n[2]-1.0) > 1.0e-5 ){
    p[0] = n[1]; p[1] = -n[0];
    normalize(p);
  }else{
    p[0] = 0; p[1] = 1;
  }
  m = p ^ n;
  drawAnyArc (p, m);
  drawAnyArc (m, -p);
}

/**
 * Draw all constraint arcs.
 */
void basic_ball::drawConstraints (void) const
{
  if ( arcball.axis() == controls3d::ArcballCntrl::NoAxes ) return;
  
  XYZ axis;
  int i;
  for(i=0; i < 3; ++i){
    if( arcball.axis_index() != i){
	    if(arcball.dragging()) continue;
	    glColor(far_color_);
	  }else glColor (near_color_);
      
    axis = arcball.AxesSets[arcball.axis()][i];
    if ( fabs(axis[2]-1.0) < 1.0e-5 ) unitCircle();
    else drawHalfArc(axis);
  }
}

/**
 *  Draw "rubber band" arc during dragging.
 */
void basic_ball::drawDragArc(void)
{
  if ( arcball.dragging () )
    {
      glColor (drag_color_);
      drawAnyArc ((arcball.from ()), (arcball.to ()));
    }
}

/**
 * Draw the controller with all its arcs. Parameter is the vector from 
 * the eye point to the center of interest. Default is -ve Z axis
 */
void basic_ball::arcball_draw ()
{
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(-1.0,1.0,-1.0,1.0,-1.0,1.0);
  
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  {
    glColor (rim_color_);
    glScaled (arcball.radius (), arcball.radius (), arcball.radius ());
    unitCircle();
    
    drawConstraints();
    drawDragArc();
  }
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

#endif

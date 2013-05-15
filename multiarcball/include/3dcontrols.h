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

#ifndef _3_d_controls_h_
#define _3_d_controls_h_

#include "vmmath.h"

#ifdef _MSC_VER
#ifdef MA_DLL
# define MA_API __declspec(dllexport)
#else
#  define MA_API __declspec(dllimport)
#endif

#include <windows.h>

#else
#  define MA_API
#endif


#include <GL/gl.h>
namespace controls3d{
  using mvct::XYZ;
  using mvct::MATRIX4x4;
  using mvct::Quaternion;
  // matrix from other types
  inline MATRIX4x4 translation(double tx, double ty, double tz){
    MATRIX4x4 transmat;
    transmat[0][3] = tx;
    transmat[1][3] = ty;
    transmat[2][3] = tz;
    return transmat;
  }
  inline MATRIX4x4 translation(const XYZ& t){
    return translation(t.x,t.y,t.z);
  };
  // rotation matrixes angle in radians
  inline MATRIX4x4 rotation_x(double angle){
    MATRIX4x4 rotmat;
    double ct = cos(angle), st = sin(angle);
    rotmat[0].set( 1,  0,  0, 0);
    rotmat[1].set( 0, ct, st, 0);
    rotmat[2].set( 0,-st, ct, 0);
    rotmat[3].set( 0,  0,  0, 1);
    return rotmat;
  }
  inline MATRIX4x4 rotation_y(double angle){
    MATRIX4x4 rotmat;
    double ct = cos(angle), st = sin(angle);
    rotmat[0].set(ct, 0,-st, 0);
    rotmat[1].set( 0, 1,  0, 0);
    rotmat[2].set(st, 0, ct, 0);
    rotmat[3].set( 0, 0,  0, 1);
    return rotmat;
  }
  inline MATRIX4x4 rotation_z(double angle){
    MATRIX4x4 rotmat;
    double ct = cos(angle), st = sin(angle);
    rotmat[0].set( ct, st, 0, 0);
    rotmat[1].set(-st, ct, 0, 0);
    rotmat[2].set(  0,  0, 1, 0);
    rotmat[3].set(  0,  0, 0, 1);
    return rotmat;
  }
  // scaling
  inline MATRIX4x4 scaling(double sx, double sy, double sz) {
    MATRIX4x4 scmat;
    scmat[0][0] = sx;
    scmat[1][1] = sy;
    scmat[2][2] = sz;
    return scmat;
  }
  inline MATRIX4x4 scaling(const XYZ& s){
    return scaling(s.x,s.y,s.z);
  }
  //
  class MA_API Transformation {
    MATRIX4x4 transform;//transformation matrix
    public:
    Transformation():transform(){}
    Transformation(const MATRIX4x4& mat): transform(mat) {}
    /**
     * Combining two transformations.
     * Post-multiply with given transformation.
     */
    void operator *= (const Transformation& tr) {
      transform *= tr.transform;
    }
    /**
     * Combining transformation and matrix.
     * Post-multiply with given matrix.
     */  
    void operator *= (const MATRIX4x4& mat) {
      transform *= mat;
    }
    /** 
     * Pre-multiply with given transformation/matrix
     * The operator chosen is not the most intuitive, but the only one that makes
     * some kind of sense
     */
    void operator /= (const Transformation& tr) {
      transform = tr.transform * transform;
    }
    /** 
     * Pre-multiply with given FTransformation/matrix
     * The operator chosen is not the most intuitive, but the only one that makes
     * some kind of sense.
     */
    void operator /= (const MATRIX4x4& mat) {
      transform = mat * transform;
    }
    
    Transformation operator * (const Transformation& tr) {
      Transformation newtr(*this);
      newtr *= tr;
      return newtr;
    }
    
    Transformation operator * (const MATRIX4x4& mat) {
      Transformation newtr(*this);
      newtr *= mat;
      return newtr;
    }
    
    friend Transformation operator * (const MATRIX4x4& mat, const Transformation& tr) {
      Transformation newtr(mat);
      newtr *= tr;
      return newtr;
    }
    /**
     * Invert the Transformation matrix.
     */
    void invert (void) {
      transform.invert();
    }
    // Apply Transformations - pre-multiply by corresponding Transformation matrices
    void translate (const XYZ& t) {
      transform = translation(t) * transform;
    }
    void translate (double tx, double ty, double tz) {
      transform = translation (tx,ty,tz) * transform;
    }
    void rotate_x (double angle) {
      transform = rotation_x (angle) * transform;
    }
    void rotate_y (double angle) {
      transform = rotation_y (angle) * transform;
    }
    void rotate_z (double angle) {
      transform = rotation_z (angle) * transform;
    }
    /**
     * Rotate according to the rotation specified by the quaternion.
     */
    void rotate (const Quaternion& quat) {
      transform = quat.to_matrix4() * transform;
    }
/*    void rotate (double angle,double x, double y, double z){
      quaternion qt(XYZ(x, y, z), angle);
      rotate(qt); 
    }
    void rotate (double angle,const XYZ& vect){
      quaternion qt(vect, angle);;
      rotate(qt); 
    }*/
    
    void scale (const XYZ& s) {
      transform = scaling(s.x,s.y,s.z) * transform;
    }
    void scale (double sx, double sy, double sz) {
      transform = scaling(sx,sy,sz) * transform;
    }
    /**
     * Apply Transformations - post-multiply
     */
    void post_translate (const XYZ& t) {
      transform *= translation(t);
    }
/*    void post_translate (double tx, double ty, double tz) {
      transform *= translation(tx,ty,tz);
    }*/
    
    void post_rotate_x (double angle) {
      transform *= rotation_x(angle);
    }
    void post_rotate_y (double angle) {
      transform *= rotation_y(angle);
    }
    void post_rotate_z (double angle) {
      transform *= rotation_z(angle);
    }
    /**
     * Rotate according to the rotation specified by the quaternion.
     */
    void post_rotate (const Quaternion& quat) {
      transform *= quat.to_matrix4();
    }
    
    void post_scale (const XYZ& s) {
      transform *= scaling(s);
    }
/*    void post_scale (double sx, double sy, double sz) {
      transform *= scaling(sx,sy,sz);
    }*/
    /**
     * Apply the Transformation in OpenGL. Calls only glMultMatrix.
     */
    void apply (void) const {
      double mat[16];
      transform.fill_array_column_major(mat);
      glMultMatrixd(mat);
    }
    /**
     * Get the Transformation matrix.
     */
    MATRIX4x4 matrix (void) {
      return transform;
    }
    /**
     * Reset the Transformation matrix.
     */
    void reset (void) {
      transform.reset();
    }
    /**
     * Set the FTransformation matrix.
     */
    void set (const MATRIX4x4 mat) {
      transform = mat;
    }
  
  };
  enum TransformsType { NoTransform=0, Pan=1, Rotate=2, Zoom=3, Dolly=4 };
  //basic controller class
  class MA_API BaseCntrl{
    protected:
      bool dragging_;
      MATRIX4x4 mtrx;
      XYZ mOld,mCur;
    public:
      bool dragging()const{return dragging_;}
      BaseCntrl():dragging_(false){}
      virtual ~BaseCntrl(){}
      //
      const MATRIX4x4& value()const{return mtrx;}
      //
      virtual void reset(){mOld.reset();mCur.reset();mtrx.reset();};
      virtual void mouse(const XYZ&)=0;
    //
      virtual void begin_drag()=0;//{dragging_=true;mOld=mCur;};
      virtual void end_drag()=0;//{dragging_=false;mOld=mCur;}
    private:
      BaseCntrl(const BaseCntrl&);
      BaseCntrl& operator =(const BaseCntrl&);
  };
  class MA_API ZoomCntrl:public BaseCntrl{
    // Mouse x coordinate
    double vNowx, vDownx;
    XYZ zNow, zDown;

    public:
    ZoomCntrl():zNow(1,1,1), zDown(1,1,1){}
    void begin_drag(){dragging_ = true; vDownx = vNowx;}
    void end_drag(){dragging_ = false; zDown = zNow;}
    void reset(){zNow.set(1,1,1); zDown.set(1,1,1); mtrx.reset();}
    void mouse (const XYZ& pos) {
      vNowx = pos[0];
      if(dragging()){
        zNow = zDown;
        // Mapping between mouse movement and scale change is as follows
        // For a mouse movement of 1.0, scale changes by a factor of 2
        // Note: This is in the transformed mouse coordinates (-1 to 1)
        // Also only the x mouse coordinate is used
        double diff = vNowx - vDownx;
        double fact = 1 + fabs (diff);
        if ( diff < 0.0 ) zNow /= fact;
        else zNow *= fact;
      
        // All 3 coordinates of the zoom FVector are changed to get proper
        // view scaling (rotate after zoom, etc.)
      
        // Fill in transposed order for GL
        mtrx[0][0] = zNow[0];
        mtrx[1][1] = zNow[1];
        mtrx[2][2] = zNow[2];
      }
    }
    XYZ zoom_value() const {
      return zNow;
    }

  };
  class MA_API PanCntrl:public BaseCntrl{
    // Mouse translations/points
    XYZ vNow, vDown;
    // Current translation
    XYZ tNow, tDown;
    public:
    void begin_drag(){dragging_ = true; vDown = vNow;}
    void end_drag(){dragging_ = false; tDown = tNow;}
    void reset(){tNow.reset(); tDown.reset(); mtrx.reset();}
    void mouse(const XYZ& pos){
      vNow = pos;
      if(dragging()) {
        tNow = tDown; tNow += vNow; tNow -= vDown;
        // Fill in transposed order for GL
        mtrx[3][0] = tNow[0];
        mtrx[3][1] = tNow[1];
        mtrx[3][2] = tNow[2];
      }
    }
    XYZ trans_value (void) const {
      return tNow;
    }
  };
  class MA_API DollyCntrl:public BaseCntrl{
    double scale_factor;
    double vNow, vDown;
    // Current dolly
    double dNow, dDown;
    public:
    DollyCntrl(double sf):scale_factor(sf){}
    void begin_drag(){dragging_ = true; vDown = vNow;}
    void end_drag(){dragging_ = false; dDown = dNow;}
    void reset(){dNow = 0.0; dDown = 0.0 ; mtrx.reset();}
    void mouse(const XYZ&pos){
      vNow = scale_factor*pos.z;
      if (dragging()) {
        dNow = dDown; dNow += vNow; dNow -= vDown;
        // Fill in transposed order for GL
        mtrx[3][0] = 0.0;
        mtrx[3][1] = 0.0;
        mtrx[3][2] = dNow;
      }
    }
    double dolly_value() const {
      return dNow;
    }

  };
  class MA_API ArcballCntrl:public BaseCntrl{
   public:
    enum AxisSet { CameraAxes=0, BodyAxes=1, NoAxes=2 };
   protected:
    XYZ vCenter;
    double dRadius;
    mvct::Quaternion qNow, qDown, qDrag;
    XYZ vNow, vDown, vFrom, vTo;
    //mvct::MATRIX4x4 mNow;
    AxisSet asAxisSet;
    int iAxisIndex;

    public:
    XYZ* AxesSets[2];
    ArcballCntrl(const XYZ&,double);
    ~ArcballCntrl();
    void begin_drag (){dragging_ = true; vDown = vNow;}
    void end_drag () {
      dragging_ = false; qDown = qNow;
      AxesSets[BodyAxes][0] = mtrx[0];
      AxesSets[BodyAxes][1] = mtrx[1];
      AxesSets[BodyAxes][2] = mtrx[2];
    }
    void reset(){qDown.reset(); qNow.reset(); mtrx.reset();}
    void mouse(const XYZ& tr);
    /**
      * Gets the "from" point.
      * @return The point on the arcball we are rotating from (initial click).
      */
    XYZ & from() { return vFrom; }

    /**
      * Gets the "to" point.
      * @return The point on the arcball we are rotating to.
      */
    XYZ & to() { return vTo; }
    mvct::Quaternion quaternion_value() const {return qNow;}
    void center(const XYZ& cen) {vCenter = cen;}
    XYZ center() const {return vCenter;}
    double radius() const { return dRadius; }
    void radius(const double r) { dRadius = r; }
    //
    AxisSet axis()const{return asAxisSet;}
    int axis_index()const{return iAxisIndex;}
    
  };
  //for arcball
  XYZ MA_API mouseOnSphere(const XYZ& mouse, const XYZ& ballCenter, double ballRadius);
  XYZ MA_API constrainToAxis(const XYZ& loose, const XYZ& axis);
  int MA_API nearestConstraintAxis(const XYZ& loose, XYZ * axes, int nAxes);
  mvct::Quaternion MA_API quatFromBallPoints(const XYZ& from, const XYZ& to);
  void MA_API quatToBallPoints(const mvct::Quaternion& q, XYZ& arcFrom, XYZ& arcTo);
  //auxiliary functions
  inline static void glColor(const mvct::XYZ& v){glColor3d(v.x,v.y,v.z);}

};

#endif //_3_d_controls_h_

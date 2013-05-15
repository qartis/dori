/* 
   Copyright 2004-2009 by the Yury Fedorchenko.
   
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
#ifndef _math_vectors_h_
#define _math_vectors_h_

#ifndef __GCCXML__

#include <math.h>
#include <iostream>
#include <iomanip>
#include <algorithm>

#endif //__GCCXML__

#ifndef M_PI
#define M_2_SQRTPI      1.12837916709551257390
#define M_2_PI          0.63661977236758134308
#define M_1_PI          0.31830988618379067154
#define M_SQRTPI        1.77245385090551602792981
#define M_3PI_4         2.3561944901923448370E0
#define M_PI_4          0.78539816339744830962
#define M_PI_2          1.57079632679489661923
#define M_PI            3.14159265358979323846
#endif


namespace mvct{

#ifndef __GCCXML__
inline double sign (double x) {
  return ( (x < 0.0) ? -1.0 : ((x > 0.0) ? 1.0 : 0.0) );
}
inline double deg2rad(double deg){return deg*M_PI/180.;};
/**functions*/
template <typename T>
T pow2(T x){return x*x;}
template <typename T>
T pow3(T x){return x*x*x;}
#endif //__GCCXML__
inline bool is_non_zero(double x){
#ifndef __GCCXML__
  return fabs(x)>1.e-12;
#endif //__GCCXML__
}
/**
 * For matrices - sign of cofactor. 1 if i+j is even, -1 if i+j is odd
 */
inline int cofsign (unsigned int i,unsigned int j) {
  return ( ((i+j)%2) ? -1 : 1 );
}


/**
  vector 2D 
*/
class XY
{
public:
  double x, y;//< coordinates
  XY operator - (const XY & v) const { return XY (this->x - v.x, this->y - v.y);}
  XY operator + (const XY & v) const { return XY (this->x + v.x, this->y + v.y);}

  XY & operator -= (const XY & v){ x -= v.x, y -= v.y; return *this;}
  XY & operator += (const XY & v){ x += v.x, y += v.y; return *this;}
  XY & operator = (const XY & v) { x = v.x, y = v.y;   return *this;}

  XY ():x(0),y(0){};
  XY (double X, double Y):x(X),y(Y){};
  XY (const XY & xy):x(xy.x),y(xy.y){};

  double abs2 ()const{return (x * x + y * y);}
  double abs () const{
#ifndef __GCCXML__
   return sqrt (x * x + y * y);
#endif
   }
  //|this|>|v|
  bool operator > (const XY & v) const{return (this->abs2 () > v.abs2 ()); }
  //|this|<|v|
  bool operator < (const XY & v) const{return (this->abs2 () < v.abs2 ()); }
  //scale
  XY operator * (const double &d) const {return XY (x * d, y * d);};
  XY operator / (const double &d) const {return XY (x / d, y / d);};
  XY operator *= (const double &d){ this->x *= d; this->y *= d; return *this; };
  XY operator /= (const double &d){ this->x /= d; this->y /= d; return *this; };
};

/**
  vector 3D 
*/
class XYZ : public XY
{
public:
  double z;
  XYZ ():z (0){}
  XYZ (const XYZ & xyz):XY(xyz.x,xyz.y),z (xyz.z){}
  XYZ (const XY & xyz):XY(xyz.x,xyz.y),z(0){}
  XYZ (double X, double Y, double Z):XY(X,Y),z (Z){}

  XYZ & operator = (const XYZ & v){ x = v.x; y = v.y; z = v.z; return *this; }
  //<|XYZ|
  double abs () const{
#ifndef __GCCXML__
  return sqrt (x * x + y * y + z * z); 
#endif  
  };
  //<|XYZ|^2
  double abs2 () const{return (x * x + y * y + z * z); };
  // vector multiplication !!!! note it has lower Precedence that + and - use () 
  XYZ operator ^ (const XYZ & v) const{ return XYZ (y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);};
  // scalar
  double operator * (const XYZ & v) const{return (x * v.x + y * v.y + z * v.z);};
  //
  XYZ operator - (const XYZ & v) const{return XYZ (x - v.x, y - v.y, z - v.z);};
  XYZ operator + (const XYZ & v) const{return XYZ (x + v.x, y + v.y, z + v.z);};
  XYZ operator - (const XY & v) const{return XYZ (x - v.x, y - v.y, z);};
  XYZ operator + (const XY & v) const{return XYZ (x + v.x, y + v.y, z);};
  XYZ & operator -= (const XYZ & v) {x -= v.x; y -= v.y;z -= v.z; return *this; };
  XYZ & operator += (const XYZ & v) {x += v.x; y += v.y;z += v.z; return *this; };

  XYZ & operator -= (const XY & v) {x -= v.x; y -= v.y; return *this; };
  XYZ & operator += (const XY & v) {x += v.x; y += v.y; return *this; };

  //scale
  XYZ operator * (const double &d) const {return XYZ (x * d, y * d, z * d);};
  XYZ operator / (const double &d) const {return XYZ (x / d, y / d, z / d);};
  XYZ operator *= (const double &d){ this->x *= d; this->y *= d;this->z *= d; return *this; };
  XYZ operator /= (const double &d){ this->x /= d; this->y /= d;this->z /= d; return *this; };
  //|this|>|v|
  bool operator > (const XYZ & v) const{return (this->abs2 () > v.abs2 ()); }
  //|this|<|v|
  bool operator < (const XYZ & v) const{return (this->abs2 () < v.abs2 ()); }
  //
  void set(double a,double b,double c){x=a,y=b,z=c;}
  void set(double a){x=y=z=a;}
  void reset(){set(0.);} 
  double& operator [](size_t i){return reinterpret_cast<double*>(this)[i];}
  const double& operator [](size_t i)const{return reinterpret_cast<const double*>(this)[i];}
  void get(double&a,double&b,double&c)const{a=x,b=y,c=z;} 
//
#if !defined(__GCCXML__) && defined(HAVE_TAO_ORB_H)
  operator CFD::XYZ(){CFD::XYZ a; a.x=x;  a.y=y;  a.z=z; return a;};
#endif
};

class XYZ_w : public XYZ{
public:
  double w;
  XYZ_w():XYZ(),w(0){}
  XYZ_w(const XYZ & xyz):XYZ(xyz),w(0){}
  XYZ_w(double X, double Y, double Z,double W):XYZ(X,Y,Z),w (W){}
  XYZ_w& operator=(const XYZ & xyz){this->XYZ::operator=(xyz),w=0;return *this;}
  //
  void set(double a,double b,double c,double d){x=a,y=b,z=c,w=d;}
  void set(double a){x=y=z=w=a;}
  void get(double&a,double&b,double&c,double&d)const{a=x,b=y,c=z,d=w;} 
  //
  double operator * (const XYZ_w & v) const{return (x*v.x + y*v.y + z*v.z + w*v.w);};
  XYZ_w operator * (const double &d) const {return XYZ_w (x * d, y * d, z * d,w*d);};
  XYZ_w operator / (const double &d) const {return XYZ_w (x / d, y / d, z / d,w/d);};
  XYZ_w operator *= (const double &d){ x *= d; y *= d;z *= d;w*=d; return *this; };
  XYZ_w operator /= (const double &d){ x /= d; y /= d;z /= d;w/=d; return *this; };

  XYZ_w operator - (const XYZ_w & v) const{return XYZ_w (x - v.x, y - v.y, z - v.z,w - v.w);};
  XYZ_w operator + (const XYZ_w & v) const{return XYZ_w (x + v.x, y + v.y, z + v.z,w + v.w);};
  XYZ_w & operator -= (const XYZ_w & v) {x -= v.x; y -= v.y;z -= v.z;w -= v.w; return *this; };
  XYZ_w & operator += (const XYZ_w & v) {x += v.x; y += v.y;z += v.z;w += v.w; return *this; };

};
//scale
inline XY    operator * (const double &d, const XY &v){return XY(v.x * d, v.y * d);}
inline XYZ   operator * (const double &d, const XYZ &v){return XYZ(v.x * d, v.y * d, v.z * d);}
inline XYZ_w operator * (const double &d, const XYZ_w &v){return XYZ_w(v.x*d,v.y*d,v.z*d,v.w*d);}

inline XYZ operator -(const XYZ & v){return XYZ(-v.x,-v.y,-v.z);}
inline XYZ_w operator -(const XYZ_w & v){return XYZ_w(-v.x,-v.y,-v.z,-v.w);}

inline XYZ product(const XYZ &a,const XYZ &b){return XYZ(a.x*b.x,a.y*b.y,a.z*b.z);}
inline XYZ_w product(const XYZ_w &a,const XYZ_w &b){return XYZ_w(a.x*b.x,a.y*b.y,a.z*b.z,a.w*b.w);}

inline XYZ normalized(const XYZ& vec)
{
  double n = vec.abs();
  if (is_non_zero (n) == true) return mvct::XYZ(vec)/n;
  return XYZ(vec);
}
inline double normalize (XYZ& vec)
{
  double n = vec.abs();
  if (is_non_zero (n) == true) vec /= n;
  return n;
}
#ifndef __GCCXML__
#include <iostream>
inline std::ostream& operator<<(std::ostream& os,const XYZ& v){
  std::scientific(os);
  os.precision(16);
  return os<<v.x<<'\t'<<v.y<<'\t'<<v.z;
}
inline std::istream& operator>>(std::istream& o,XYZ& v){
  o>>v.x>>v.y>>v.z;
  return o;
}
#endif //__GCCXML__

class SingularM{};

class MATRIX3x3{
  XYZ row[3];
public:
  MATRIX3x3(){row[0].set(1,0,0),row[1].set(0,1,0),row[2].set(0,0,1);}
  MATRIX3x3(const XYZ&r1,const XYZ&r2,const XYZ&r3){row[0]=r1,row[1]=r2,row[2]=r3;}
  MATRIX3x3(double a11,double a12,double a13,
            double a21,double a22,double a23,
            double a31,double a32,double a33){
            row[0].set(a11,a12,a13),row[1].set(a21,a22,a23),row[2].set(a31,a32,a33);}
  XYZ& operator[](size_t i){return row[i];}
  const XYZ& operator[](size_t i)const{return row[i];}
  //
  XYZ operator *(const XYZ&v)const{return XYZ(row[0]*v,row[1]*v,row[2]*v);}
  //
  void transpose(){
#ifndef __GCCXML__
    for(int i=0; i < 3; ++i)
	    for(int j=i+1; j < 3; ++j)std::swap(row[i][j],row[j][i]);
#endif	    
  }
  void invert()throw(SingularM){
#ifndef __GCCXML__
    XYZ inv[3];
    int i,j,swaprow;
    for (i=0; i<3; ++i)inv[i][i] = 1.0;
      // inv will be identity initially and will become the inverse at the end
    for (i=0; i < 3; ++i){
	    // i is column
	    // Find row in this column which has largest element (magnitude)
	    swaprow = i;
	    for (j=i+1; j < 3; ++j)
	      if ( fabs (row[j][i]) > fabs (row[i][i]) ) swaprow = j;
	  
	    if ( swaprow != i ){
	      // Swap the two rows to get largest element in main diagonal
	      // Do this for the RHS also
	      std::swap (row[i],row[swaprow]); 
	      std::swap (inv[i],inv[swaprow]);
	    }
	    // Check if pivot is non-zero
	    if ( !is_non_zero(row[i][i]) ){
        throw(SingularM());
      }
	    // Divide matrix by main diagonal element to make it 1.0
	    double fact = row[i][i];
	    for (j=0; j < 3; ++j){
	      row[j] /= fact;
	      inv[j] /= fact;
	    }
	    // Make non-main-diagonal elements in this column 0 using main-diagonal row
	    for (j=0; j < 3; ++j){
	      if ( j != i ){
		      double temp = row[j][i];
		      row[j] -= row[i]*temp;
		      inv[j] -= inv[i]*temp;
		    }
	    }
	  }
    // Main-diagonal elements on LHS may not be 1.0 now. Divide to make LHS identity
    // Last row will be 1.0
    for (i=0; i < 2; ++i){
	    double pivot = row[i][i];
	    row[i] /= pivot;
	    inv[i] /= pivot;
	  }
    for (i=0; i < 3; ++i)row[i] = inv[i];
#endif
  }
  // return matrix determinant
  double determinant(){
    double det = 0.0;
    det =   row[0][0] * row[1][1] * row[2][2] 
	        + row[0][1] * row[1][2] * row[2][0] 
	        + row[0][2] * row[1][0] * row[2][1] 
	        - row[2][0] * row[1][1] * row[0][2] 
	        - row[2][1] * row[1][2] * row[0][0] 
	        - row[2][2] * row[1][0] * row[0][1];
    return det;
  }

#ifndef __GCCXML__
  friend MATRIX3x3 transpose(const MATRIX3x3& mat){
    MATRIX3x3 trans(mat); trans.transpose();return trans;
  }
#endif
  friend MATRIX3x3 operator * (const MATRIX3x3& mat1, const MATRIX3x3& mat2);
};
class MATRIX4x4{
  XYZ_w row[4];
public:
  MATRIX4x4(){
      row[0].set(1.0,0.0,0.0,0.0),row[1].set(0.0,1.0,0.0,0.0),
      row[2].set(0.0,0.0,1.0,0.0),row[3].set(0.0,0.0,0.0,1.0);
  }
#ifndef __GCCXML__
  MATRIX4x4(double a,double b,double c,double d,double e,double f,double g,double h,
            double i,double j,double k,double l,double m,double n,double o,double p){
      row[0].set(a,b,c,d),row[1].set(e,f,g,h),
      row[2].set(i,j,k,l),row[3].set(m,n,o,p);
  }
#endif
  MATRIX4x4(const MATRIX3x3& m){for(int i=0; i < 3; ++i)row[i] = m[i];}
  void reset(void){
      row[0].set (1.0, 0.0, 0.0, 0.0),row[1].set (0.0, 1.0, 0.0, 0.0),
      row[2].set (0.0, 0.0, 1.0, 0.0),row[3].set (0.0, 0.0, 0.0, 1.0);
  }
//
  XYZ_w& operator[](size_t i){return row[i];}
  const XYZ_w& operator[](size_t i)const{return row[i];}
//
  XYZ operator *(const XYZ&v)const{ 
        return XYZ(row[0].x*v.x+row[0].y*v.y+row[0].z*v.z+row[0].w,
        row[1].x*v.x+row[1].y*v.y+row[1].z*v.z+row[1].w,
        row[2].x*v.x+row[2].y*v.y+row[2].z*v.z+row[2].w);
  }
  XYZ mul_as3(const XYZ&v)const{ 
        return XYZ(row[0].x*v.x+row[0].y*v.y+row[0].z*v.z,
        row[1].x*v.x+row[1].y*v.y+row[1].z*v.z,
        row[2].x*v.x+row[2].y*v.y+row[2].z*v.z);
  }
  XYZ_w operator *(const XYZ_w&v)const{return XYZ_w(row[0]*v,row[1]*v,row[2]*v,row[3]*v);}
//
  MATRIX4x4& operator *=(const MATRIX4x4& mat){
    return *this=(*this)*mat;
  }
//
  void invert(void)throw(SingularM){
#ifndef __GCCXML__
    XYZ_w inv[4];
    int i,j,swaprow;
    for (i=0; i < 4; ++i)inv[i][i] = 1.0;
    // inv will be identity initially and will become the inverse at the end
    for (i=0; i < 4; ++i){
	    // i is column
	    // Find row in this column which has largest element (magnitude)
	    swaprow = i;
	    for (j=i+1; j < 4; ++j)
	      if ( fabs (row[j][i]) > fabs (row[i][i]) ) swaprow = j;
	    if ( swaprow != i ){
	      // Swap the two rows to get largest element in main diagonal
	      // Do this for the RHS also
	      std::swap(row[i],row[swaprow]);std::swap (inv[i], inv[swaprow]);
	    }
	    // Check if pivot is non-zero
	    if ( !is_non_zero (row[i][i]) ){
        throw(SingularM());//cerr << "FMatrix4x4 inverse(const FMatrix4x4&) : Singular matrix!" << endl;	      // Return original matrix without change	      return;
      }
	    // Divide matrix by main diagonal element to make it 1.0
	    double fact = row[i][i];
	    for(j=0; j < 4; ++j){
	      row[j] /= fact;
	      inv[j] /= fact;
	    }
	    // Make non-main-diagonal elements in this column 0 using main-diagonal row
	    for(j=0; j < 4; ++j){
        if ( j != i ){
          double temp = row[j][i];
          row[j] -= row[i]*temp;
          inv[j] -= inv[i]*temp;
        }
	    }
	  }
    // Main-diagonal elements on LHS may not be 1.0 now. Divide to make LHS identity
    // Last row will be 1.0
    for (i=0; i < 3; ++i){
	    double pivot = row[i][i];
	    row[i] /= pivot;
	    inv[i] /= pivot;
	  }
    for (i=0; i < 4; ++i)row[i] = inv[i];
#endif
  }

  void transpose(){
#ifndef __GCCXML__
    for(int i=0; i < 4; ++i)
	    for(int j=i+1; j < 4; ++j)std::swap(row[i][j],row[j][i]);
#endif
  }
#ifndef __GCCXML__
  friend MATRIX4x4 transpose(const MATRIX4x4& mat){
    MATRIX4x4 trans(mat); trans.transpose();return trans;
  }
#endif
//
  /**
   * Fill an array with contents of the matrix
   * Row - major form -> Row 1 == { array[0], array[1], array[2], array[3] }
   */
  void fill_array_row_major (double array[16]) const{
    int index=0;
    for(int i=0; i < 4; ++i){
      row[i].get (array[index],array[index+1],array[index+2],array[index+3]);
	    index += 4;
	  }
  }
  /**
   * Fill an array with contents of the matrix
   * Row - major form -> Column 1 == { array[0], array[1], array[2], array[3] }  
   */
  void fill_array_column_major (double array[16]) const{
    int index=0;
    for (int i=0; i < 4; ++i){
	    row[i].get(array[index],array[index+4],array[index+8],array[index+12]);
	    index += 1;
	  }
  }
  ///cofactor
  MATRIX3x3 cofactor(int r, int c) const {
    MATRIX3x3 cof;
    XYZ cofrow;
    int cfcol, cfrow;
      
    cfrow = 0;
    for (int i=0; i < 4; ++i){
	    if ( i != r ){
	      cfcol = 0;
	      for (int j=0; j < 4; ++j)
		      if ( j != c ) cofrow[cfcol++] = row[i][j];
	        cof[cfrow++] = cofrow;
	    }
	  }
    return cof;
  }
    
  double determinant()const{
    double det = 0.0;
      
    for (int i=0; i < 4; ++i)
	    det += row[0][i] * cofsign (0, i) * cofactor(0, i).determinant();
    return det;
  }

  friend MATRIX4x4 operator * (const MATRIX4x4& mat1, const MATRIX4x4& mat2);
#ifndef __GCCXML__
  friend std::ostream& operator<<(std::ostream& o,const MATRIX4x4& mat);
  friend std::istream& operator>>(std::istream& o,MATRIX4x4& mat);
#endif
};

class Quaternion{
  XYZ v;
  double w;
public:
  Quaternion():v(),w(1.0){}
  Quaternion(const XYZ& vv,const double ww):v(vv),w(ww){}
  Quaternion(const double ang,const XYZ& ax){
    set_axis_and_angle(ax,ang);
  }
  //
  Quaternion operator *=(const Quaternion& quat){
    double s(w*quat.w - v*quat.v);
    v = quat.v*w + v*quat.w + (v ^ quat.v);
    w = s;
    return *this;
  }
  Quaternion operator * (const Quaternion& q2){
    Quaternion prod = *this;
    prod *= q2;
    return prod;
  }
  //
  MATRIX3x3 to_matrix3()const{
    MATRIX3x3 mat(
    XYZ( 1.0 - 2.0*(v.y*v.y + v.z*v.z), 2.0*(v.x*v.y - w*v.z), 2.0*(v.x*v.z + w*v.y) ),
    XYZ( 2.0*(v.x*v.y + w*v.z), 1.0 - 2.0*(v.x*v.x + v.z*v.z), 2.0*(v.y*v.z - w*v.x) ),
    XYZ( 2.0*(v.x*v.z - w*v.y), 2.0*(v.y*v.z + w*v.x), 1.0 - 2.0*(v.x*v.x + v.y*v.y) ));
    return mat;
  }
  MATRIX4x4 to_matrix4()const{
    MATRIX4x4 mat(this->to_matrix3());
    mat[3][3]=1.0;
    return mat;
  }
  //
  double& operator [](size_t i){
    if ( i == 3 ) return w;
    return v[i];
  }
  const double& operator [](size_t i) const{
    if ( i == 3 ) return w;
    return v[i];
  }
  //
  void reset(){v=XYZ();w=1;}
#ifndef __GCCXML__
  friend Quaternion conjugate(const Quaternion& quat){
    return Quaternion(-quat.v,quat.w);
  }
#endif
  void set_axis_and_angle(const XYZ& axis, double theta){
#ifndef __GCCXML__
    w = cos(theta/2.0);
    v = normalized(axis);
    v *= sin(theta/2.0);
#endif
  }

};

#ifndef __GCCXML__
inline MATRIX4x4 operator * (const MATRIX4x4& mat1, const MATRIX4x4& mat2)
{
  MATRIX4x4 prod, trans;
  // Find the transpose of the 2nd matrix
  trans = transpose (mat2);
  // The columns of mat2 are now the rows of trans
  // Multiply appropriate rows and columns to get the product
  prod.row[0].set (mat1.row[0]*trans.row[0],
                   mat1.row[0]*trans.row[1],
                   mat1.row[0]*trans.row[2],
                   mat1.row[0]*trans.row[3]);
  prod.row[1].set (mat1.row[1]*trans.row[0],
                   mat1.row[1]*trans.row[1],
                   mat1.row[1]*trans.row[2],
                   mat1.row[1]*trans.row[3]);
  prod.row[2].set (mat1.row[2]*trans.row[0],
                   mat1.row[2]*trans.row[1],
                   mat1.row[2]*trans.row[2],
                   mat1.row[2]*trans.row[3]);
  prod.row[3].set (mat1.row[3]*trans.row[0],
                   mat1.row[3]*trans.row[1],
                   mat1.row[3]*trans.row[2],
                   mat1.row[3]*trans.row[3]);
  return prod;
}

inline XYZ_w operator * (const XYZ_w& vec, const MATRIX4x4& mat)
{
  return (transpose(mat) * vec);
}
/*
  The following functions are defined outside the class so that they use the
  friend versions of member functions instead of the member functions themselves
*/
inline MATRIX3x3 operator * (const MATRIX3x3& mat1, const MATRIX3x3& mat2)
{
  MATRIX3x3 prod, trans;
  // Find the transpose of the 2nd matrix
  trans = transpose (mat2);
  // The columns of mat2 are now the rows of trans
  // Multiply appropriate rows and columns to get the product
  prod.row[0].set (mat1.row[0]*trans.row[0],
                   mat1.row[0]*trans.row[1],
                   mat1.row[0]*trans.row[2]);
  prod.row[1].set (mat1.row[1]*trans.row[0],
                   mat1.row[1]*trans.row[1],
                   mat1.row[1]*trans.row[2]);
  prod.row[2].set (mat1.row[2]*trans.row[0],
                   mat1.row[2]*trans.row[1],
                   mat1.row[2]*trans.row[2]);
  return prod;
}

inline XYZ operator * (const XYZ& vec, const MATRIX3x3& mat)
{
  return (transpose(mat) * vec);
}
inline std::ostream& operator<<(std::ostream& o,const XYZ_w& v){
  o.setf(std::ios_base::floatfield,std::ios_base::scientific);
  o<<std::setprecision(16)<<v.x<<'\t'<<v.y<<'\t'<<v.z<<'\t'<<v.w;
  return o;
}
inline std::istream& operator>>(std::istream& o,XYZ_w& v){
  o>>v.x>>v.y>>v.z>>v.w;
  return o;
}

inline std::ostream& operator<<(std::ostream& o,const MATRIX4x4& mat){
  for(int i=0;i<4;++i)o<<mat.row[i]<<std::endl;
  return o;
}
inline std::istream& operator>>(std::istream& o,MATRIX4x4& mat){
  for(int i=0;i<4;++i)o>>mat.row[i];
  return o;
}
#endif //__GCCXML__

};//nmsps

#endif

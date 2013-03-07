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

#include "3dcontrols.h"

using mvct::pow2;

namespace controls3d{

ArcballCntrl::ArcballCntrl(const mvct::XYZ& cen, double rad)
{
  vCenter = cen;
  dRadius = rad;
  asAxisSet = NoAxes;
  
  AxesSets[CameraAxes] = new mvct::XYZ[3];
  AxesSets[BodyAxes] = new mvct::XYZ[3];
  
  AxesSets[CameraAxes][0].set(1.0,0.0,0.0); // X axis for camera
  AxesSets[CameraAxes][1].set(0.0,1.0,0.0); // Y axis for camera
  AxesSets[CameraAxes][2].set(0.0,0.0,1.0); // Z axis for camera
  
  AxesSets[BodyAxes][0] = mtrx[0];
  AxesSets[BodyAxes][1] = mtrx[1];
  AxesSets[BodyAxes][2] = mtrx[2];
}

ArcballCntrl::~ArcballCntrl()
{
  delete [] AxesSets[CameraAxes];
  delete [] AxesSets[BodyAxes];
}
void ArcballCntrl::mouse(const XYZ& tr){
  vNow.set(tr.x, tr.y, 0);

  vFrom = mouseOnSphere (vDown, vCenter, dRadius);
  vTo = mouseOnSphere (vNow, vCenter, dRadius);
  if(dragging()){
      if ( asAxisSet != NoAxes)
	    {
	      vFrom = constrainToAxis (vFrom, AxesSets[asAxisSet][iAxisIndex]);
	      vTo = constrainToAxis (vTo, AxesSets[asAxisSet][iAxisIndex]);
	    }
      
      //vnFrom[0] = -vFrom[0];
      //vnFrom[1] = -vFrom[1];
      //vnFrom[2] = vFrom[2];
      //vnTo[0] = -vTo[0];
      //vnTo[1] = -vTo[1];
      //vnTo[2] = vTo[2];
      
      qDrag = quatFromBallPoints (vFrom, vTo);
      qNow = qDrag * qDown;
  }else{
    if( asAxisSet != NoAxes)
	    iAxisIndex = nearestConstraintAxis (vTo, AxesSets[asAxisSet], 3);
  }
  mtrx = conjugate(qNow).to_matrix4();//!!!!
}
mvct::XYZ mouseOnSphere (const mvct::XYZ& mouse, const mvct::XYZ& center, double radius)
{
  mvct::XYZ ballMouse;
  register double magsqr;

  ballMouse[0] = (mouse[0] - center[0]) / radius;
  ballMouse[1] = (mouse[1] - center[1]) / radius;
  magsqr = pow2(ballMouse[0]) + pow2(ballMouse[1]);
  if (magsqr > 1.0)
    {
      register double scale = 1.0/sqrt(magsqr);
      ballMouse[0] *= scale; 
      ballMouse[1] *= scale;
      ballMouse[2] = 0.0;
    }
  else
    {
      ballMouse[2] = sqrt (1.0 - magsqr);
    }
  return ballMouse;
}

/**
 * Construct a unit quaternion from two points on unit sphere
 */
Quaternion quatFromBallPoints (const mvct::XYZ& from, const mvct::XYZ& to)
{
  return Quaternion (from ^ to, from * to);
}

/**
 * Convert a unit quaternion to two points on unit sphere
 * Assumes that the given quaternion is a unit quaternion
 */
void quatToBallPoints (const Quaternion& q, mvct::XYZ& arcFrom, mvct::XYZ& arcTo)
{
  double s;
  s = sqrt (pow2 (q[0]) + pow2 (q[1]));

  if (fabs (s) < 1.0e-5) 
    arcFrom.set (0.0, 1.0, 0.0);
  else
    arcFrom.set (-q[1]/s, q[0]/s, 0.0);
  
  arcTo[0] = q[3]*arcFrom[0] - q[2]*arcFrom[1];
  arcTo[1] = q[3]*arcFrom[1] + q[2]*arcFrom[0];
  arcTo[2] = q[0]*arcFrom[1] - q[1]*arcFrom[0];
  if (q[3] < 0.0) arcFrom *= -1.0;
}

/**
 * Force sphere point to be perpendicular to axis.
 */
XYZ constrainToAxis (const mvct::XYZ& loose, const mvct::XYZ& axis)
{
  mvct::XYZ onPlane;
  register float norm;
  onPlane = loose - axis * (axis*loose);
  norm = onPlane.abs2();//normsqr(onPlane);
  if (norm > 0.0)
    {
      if (onPlane[2] < 0.0) onPlane *= -1.0;
      onPlane /= sqrt(norm);
    }
  else if ( fabs (axis[2]-1.0) < 1.0e-5 )
    {
      onPlane.set (1.0, 0.0, 0.0);
    }
  else
    {
      onPlane.set (-axis[1], axis[0], 0.0); 
      normalize (onPlane);
    }
  return onPlane;
}

/**
 * Find the index of nearest arc of axis set.
 */
int nearestConstraintAxis (const mvct::XYZ& loose, mvct::XYZ * axes, int nAxes)
{
  mvct::XYZ onPlane;
  register float max, dot;
  register int i, nearest;
  max = -1.0; nearest = 0;
  for (i=0; i < nAxes; i++)
    {
      onPlane = constrainToAxis(loose, axes[i]);
      dot = onPlane * loose;
      if ( dot > max )
	{
	  max = dot; nearest = i;
	}
    }
  return nearest;
}

}

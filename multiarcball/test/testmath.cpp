#include "vmmath.h"
using namespace mvct;//math classes;

int main(){
  XYZ a(1,2,0),b(3,2,1);
  double m(2);
  std::cout<<(a+b)<<'\n' // vector summ
      <<(a-b)<<'\n' // vector 
      <<(a*b)<<'\n' // scalar product 
      <<(a^b)<<'\n' // cross product
      <<(m*b)<<'\n' // myltiply vector b by m
      <<(a*m)<<'\n' // myltiply vector a by m 
      <<(b/m)<<'\n' // divide vector b by m 
      <<a.abs()<<'\n' // absolute value of vector a 
      <<std::endl;
  return 0;
}

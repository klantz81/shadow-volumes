#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>

class vector3 {
  private:
  protected:
  public:
	double x, y, z;
	vector3();
	vector3(double x, double y, double z);
	double operator*(const vector3& v);
	vector3 cross(const vector3& v);
	vector3 operator+(const vector3& v);
	vector3 operator-(const vector3& v);
	vector3 operator*(const double s);
	vector3& operator=(const vector3& v);
	vector3 h(const vector3& v);		// hadamard product
	double length();
	vector3 unit();
};

class vector2 {
  private:
  protected:
  public:
	double x, y;
	vector2();
	vector2(double x, double y);
	double operator*(const vector2& v);
	vector2 operator+(const vector2& v);
	vector2 operator-(const vector2& v);
	vector2 operator*(const double s);
	vector2& operator=(const vector2& v);
	double length();
	vector2 unit();
};

#endif
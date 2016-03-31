#include "vector.h"

vector3::vector3() : x(0.0), y(0.0), z(0.0) { }
vector3::vector3(double x, double y, double z) : x(x), y(y), z(z) { }

double vector3::operator*(const vector3& v) {
	return this->x*v.x + this->y*v.y + this->z*v.z;
}

vector3 vector3::cross(const vector3& v) {
	return vector3(this->y*v.z - this->z*v.y, this->z*v.x - this->x*v.z, this->x*v.y - this->y*v.x);
}

vector3 vector3::operator+(const vector3& v) {
	return vector3(this->x + v.x, this->y + v.y, this->z + v.z);
}

vector3 vector3::operator-(const vector3& v) {
	return vector3(this->x - v.x, this->y - v.y, this->z - v.z);
}

vector3 vector3::operator*(const double s) {
	return vector3(this->x*s, this->y*s, this->z*s);
}

vector3& vector3::operator=(const vector3& v) {
	this->x = v.x; this->y = v.y; this->z = v.z;
	return *this;
}

vector3 vector3::h(const vector3& v) {
	return vector3(this->x * v.x, this->y * v.y, this->z * v.z);
}

double vector3::length() {
	return sqrt(this->x*this->x + this->y*this->y + this->z*this->z);
}

vector3 vector3::unit() {
	double l = this->length();
	return vector3(this->x/l, this->y/l, this->z/l);
}



vector2::vector2() : x(0.0), y(0.0) { }
vector2::vector2(double x, double y) : x(x), y(y) { }

double vector2::operator*(const vector2& v) {
	return this->x*v.x + this->y*v.y;
}

vector2 vector2::operator+(const vector2& v) {
	return vector2(this->x + v.x, this->y + v.y);
}

vector2 vector2::operator-(const vector2& v) {
	return vector2(this->x - v.x, this->y - v.y);
}

vector2 vector2::operator*(const double s) {
	return vector2(this->x*s, this->y*s);
}

vector2& vector2::operator=(const vector2& v) {
	this->x = v.x; this->y = v.y;
	return *this;
}

double vector2::length() {
	return sqrt(this->x*this->x + this->y*this->y);
}

vector2 vector2::unit() {
	double l = this->length();
	return vector2(this->x/l, this->y/l);
}

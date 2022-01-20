#pragma once

#include <cassert>
#include <cstdarg>
#include <string>

/// Utility class to represent an n-component vector
template<typename T, int N>
class Vec {

	T components[N];

public:


	/// Default constructor. Does not initialize any values.
	// disable warning about uninitialized components - this is intended here; calling code is expected to set the components up after declaration of the vector.
#pragma warning(push)
#pragma warning(disable: 26495)
	inline Vec() {}
#pragma warning(pop)

	// Component constructors
	// Variadic params for ease of use - should pass N components exactly!
	inline Vec(T x, ...) {
		components[0] = x;
		va_list arguments;
		va_start(arguments, x);
		for (int comp = 1; comp < N; ++comp) {
			components[comp] = va_arg(arguments, T);
		}
		va_end(arguments);
	}
	inline Vec(int x, ...) {
		components[0] = (T)x;
		va_list arguments;
		va_start(arguments, x);
		for (int comp = 1; comp < N; ++comp) {
			components[comp] = (T)va_arg(arguments, int);
		}
		va_end(arguments);
	}



	// Getters - includes standards for X, Y, Z, W
	inline T get (const int& i) const { assert(N > i); return components[i]; }
	inline T operator[](const int& i) const { return get(i); }
	inline T X () const { return get(0); }
	inline T Y () const { return get(1); }
	inline T Z () const { return get(2); }
	inline T W () const { return get(3); }

	// Swizzles (2- and 3-components only; all possible permutations of x, y, z, w)
#define SWIZ2(a, b)			inline Vec<T, 2> a ## b				() const { return Vec<T, 2>( a(), b() ); }
#define SWIZ3(a, b, c)		inline Vec<T, 3> a ## b ## c		() const { return Vec<T, 3>(a(), b(), c()); }
	// .x, .xx, .xxx
#define SWIZZLE(a) SWIZ2(a, a) SWIZ3(a, a, a)
	SWIZZLE(X) SWIZZLE(Y) SWIZZLE(Z) SWIZZLE(W)
#undef SWIZZLE
	// .xy, .yx, .xxy, .xyx, .yxx, .xyy, .yxy, .yyx
#define SWIZZLE(a, b) SWIZ2(a, b) SWIZ2(b, a) SWIZ3(a, a, b) SWIZ3(a, b, a) SWIZ3(b, a, a) SWIZ3(a, b, b) SWIZ3(b, a, b) SWIZ3(b, b, a)
	SWIZZLE(X, Y) SWIZZLE(X, Z) SWIZZLE(X, W) SWIZZLE(Y, Z) SWIZZLE(Y, W) SWIZZLE(Z, W)
#undef SWIZZLE
	// .xyz, .xzy, .yxz, .zxy, .yzx, .zyx
#define SWIZZLE(a, b, c) SWIZ3(a, b, c) SWIZ3(a, c, b) SWIZ3(b, a, c) SWIZ3(c, a, b) SWIZ3(b, c, a) SWIZ3(c, b, a)
	SWIZZLE(X, Y, Z) SWIZZLE(X, Y, W) SWIZZLE(X, Z, W) SWIZZLE(Y, Z, W)
#undef SWIZZLE
#undef SWIZ2
#undef SWIZ3

	// Setters
	inline T set (const int& i, const T& val) { assert(N > i); return components[i] = val; }
	inline T setX (const T& val) { return set(0, val); }
	inline T setY (const T& val) { return set(1, val); }
	inline T setZ (const T& val) { return set(2, val); }
	inline T setW (const T& val) { return set(3, val); }



	/// Vector length squared
	inline T lengthSqr() {
		T val = 0;
		for (int i = 0; i < N; ++i) val += get(i) * get(i);
		return val;
	}

	/// Normalized vector
	inline Vec<T, N> normalized() {
		T length = (T)sqrt((double)lengthSqr());
		Vec<T, N> ret;
		for (int i = 0; i < N; ++i) ret.set(i, get(i) / length);
		return ret;
	}



	/// Standard operations

	/// Component-wise addition & subtraction
	inline Vec<T, N> operator+(const Vec<T, N>& v) const {
		Vec<T, N> ret;
		for (int i = 0; i < N; ++i) ret.set(i, v[i] + get(i));
		return ret;
	}

	inline Vec<T, N> operator+=(const Vec<T, N>& v) {
		Vec<T, N> n = (*this) + v;
		for (int i = 0; i < N; ++i) set(i, n[i]);
		return *this;
	}

	inline Vec<T, N> operator-(const Vec<T, N>& v) const {
		Vec<T, N> ret;
		for (int i = 0; i < N; ++i) ret.set(i, get(i) - v[i]);
		return ret;
	}

	inline Vec<T, N> operator-=(const Vec<T, N>& v) {
		Vec<T, N> n = (*this) - v;
		for (int i = 0; i < N; ++i) set(i, n[i]);
		return *this;
	}

	/// Product
	inline Vec<T, N> operator*(const T& f) const {
		Vec<T, N> ret;
		for (int i = 0; i < N; ++i) ret.set(i, get(i) * f);
		return ret;
	}



	// String conversion
	inline std::string toString() const {
		std::string ret = "[ ";
		for (int i = 0; i < N; ++i) {
			ret += std::to_string(components[i]);
			if (i < N - 1) ret += ", \t";
		}
		return ret + " ]";
	}

};


// Common Vec types
typedef Vec<double, 4> Vec4;
typedef Vec<double, 3> Vec3;
typedef Vec<double, 2> Vec2;

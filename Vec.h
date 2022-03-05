﻿#pragma once

#include <cmath>
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
	template<typename TT>
	inline Vec(TT x, ...) {
		components[0] = (T)x;
		va_list arguments;
		va_start(arguments, x);
		for (int comp = 1; comp < N; ++comp) {
			components[comp] = (T)va_arg(arguments, TT);
		}
		va_end(arguments);
	}

	static inline Vec<T, N> Zero() {
		Vec<T, N> zero;
		for (int i = 0; i < N; ++i) zero.set(i, (T)0);
		return zero;
	}

	static inline Vec<T, N> One() {
		Vec<T, N> one;
		for (int i = 0; i < N; ++i) one.set(i, (T)1);
		return one;
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

	// Swizzle setters (2- and 3-components only; all possible permutations of x, y, z, w)
#define SWIZ2(a, b)			inline void set ## a ## b		(const Vec<T, 2>& v)	{ set ## a (v[0]); set ## b (v[1]); }
#define SWIZ3(a, b, c)		inline void set ## a ## b ## c	(const Vec<T, 3>& v)	{ set ## a (v[0]); set ## b (v[1]); set ## c (v[2]); }
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


	/// Vector length squared
	inline T lengthSqr() const {
		T val = 0;
		for (int i = 0; i < N; ++i) val += get(i) * get(i);
		return val;
	}

	/// Normalized vector
	inline Vec<T, N> normalized() const {
		T length = (T)sqrt((double)lengthSqr());
		Vec<T, N> ret;
		for (int i = 0; i < N; ++i) ret.set(i, get(i) / length);
		return ret;
	}
	inline void normalize() {
		T length = (T)sqrt((double)lengthSqr());
		for (int i = 0; i < N; ++i) set(i, get(i) / length);
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

	inline void add(const Vec<T, N>& v) {
		for (int i = 0; i < N; ++i) set(i, get(i) + v[i]);
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

	inline void multiply(const T& f) {
		for (int i = 0; i < N; ++i) set(i, get(i) * f);
	}

	/// Linear interpolation
	inline static Vec<T, N> Lerp(const Vec<T, N>& a, const Vec<T, N>& b, double t) {
		Vec<T, N> ret;
		for (int i = 0; i < N; ++i) ret.set(i, (T)(a.get(i) * (1.0 - t) + b.get(i) * t));
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



// Custom operator for hadamard (elementwise) product
// Two Vec<T, N> instances v1 and v2 can be elementwise-multiplied using v1 <o> v2
template<typename T, int N> struct HadamardVecLhs { Vec<T, N> lhs; };
enum { hadamard };
template<typename T, int N>
HadamardVecLhs<T, N> operator<(const Vec<T, N>& lhs, decltype(hadamard)) {
	return { lhs };
}
template<typename T, int N>
Vec<T, N> operator>(const HadamardVecLhs<T, N>& lhs, const Vec<T, N>& rhs) {
	Vec<T, N> result;
	for (int i = 0; i < N; ++i) result.set(i, lhs.lhs[i] * rhs[i]);
	return result;
}



// Common Vec types
typedef Vec<double, 4> Vec4;
typedef Vec<double, 3> Vec3;
typedef Vec<double, 2> Vec2;
typedef Vec<int, 4> IVec4;
typedef Vec<int, 3> IVec3;
typedef Vec<int, 2> IVec2;

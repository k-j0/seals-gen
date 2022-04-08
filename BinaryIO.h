#pragma once

#include <vector>
#include <string>

#include "Vec.h"

namespace bio {

	/// Writes a value of trivial type T (no pointers) to data as bytes
	template<typename T>
	void writeSimple(std::vector<uint8_t>& data, const T& val) {
		union {
			T original;
			uint8_t bytes[sizeof(T)];
		} o2b;
		o2b.original = val;
		for (std::size_t i = 0; i < sizeof(T); ++i) {
			data.push_back(o2b.bytes[i]);
		}
	}

	/// Reads a value of trivial type T (no pointers) from data as bytes
	template<typename T>
	T readSimple(const std::vector<uint8_t>& data, int& at) {
		union {
			T original;
			uint8_t bytes[sizeof(T)];
		} b2o;
		for (std::size_t i = 0; i < sizeof(T); ++i) {
			assert(at < data.size());
			b2o.bytes[i] = data[at];
			++at;
		}
		return b2o.original;
	}

	void writeString(std::vector<uint8_t>& data, const std::string& val) {
		for (const char& c : val) {
			writeSimple<char>(data, c);
		}
		writeSimple<char>(data, '\0');
	}

	std::string readString(const std::vector<uint8_t>& data, int& at) {
		std::string s = "";
		assert(at < data.size());
		while (at < (int)data.size()) {
			char c = readSimple<char>(data, at);
			if (c == '\0') break;
			else s += c;
		}
		return s;
	}

	template<typename T, int N>
	void writeVec (std::vector<uint8_t>& data, const Vec<T, N>& val) {
		for (int i = 0; i < N; ++i) {
			writeSimple<T>(data, val[i]);
		}
	}

	template<typename T, int N>
	Vec<T, N> readVec (const std::vector<uint8_t>& data, int& at) {
		Vec<T, N> r;
		for (int i = 0; i < N; ++i) {
			r.set(i, readSimple<T>(data, at));
		}
		return r;
	}

}

#pragma once

#include <vector>
#include <string>
#include <cstdio>

#include "Vec.h"

///
/// Functionality to read/write from binary data
/// bio::writeXxx functions take a Bytes element (can be a std::vector<std::uint8_t> or a BufferedBinaryFileOutput<>) and writes values into it
/// bio::readXxx functions take a std::vector<std::uint8_t> and read values from it
///

namespace bio {
	
	/// Helper binary output stream that writes its contents to file incrementally
	/// Can be fed instead of a std::vector<std::uint8_t> to the bio::writeXxx functions
	template<int BatchSize=16384>
	struct BufferedBinaryFileOutput {
	private:
		std::uint8_t data[BatchSize];
		FILE* file;
		std::size_t size = 0;
		
	public:
		
		// Opens the file to start writing
		BufferedBinaryFileOutput(std::string filename)
		#ifndef _MSC_VER
			: file(std::fopen(filename.c_str(), "wb")) {}
		#else
			{
				if (fopen_s(&file, filename.c_str(), "wb") != 0) {
					std::printf("File %s could not be open for binary write, aborting.\n", filename.c_str());
					std::exit(1);
				}
			}
		#endif
		
		/// Adds the byte to the buffer and if necessary dumps out contents into the file
		inline void push_back(const std::uint8_t elem) {
			data[size] = elem;
			++size;
			if (size >= BatchSize) {
				dump();
			}
		}
		
		/// Dumps out the current contents of data into the output file, and gets ready to add more data
		inline void dump() {
			if (size <= 0) return;
			std::fwrite(data, sizeof(std::uint8_t), size, file);
			size = 0;
		}
		
		/// Dumps out remaining contents to the file and cleans up
		~BufferedBinaryFileOutput() {
			dump();
			std::fclose(file);
		}
		
	}; // BufferedBinaryFileOutput
	

	/// Writes a value of trivial type T (no pointers) to data as bytes
	template<typename T, typename Bytes=BufferedBinaryFileOutput<>>
	void writeSimple(Bytes& data, const T& val) {
		union {
			T original;
			std::uint8_t bytes[sizeof(T)];
		} o2b = {};
		o2b.original = val;
		for (std::size_t i = 0; i < sizeof(T); ++i) {
			data.push_back(o2b.bytes[i]);
		}
	}

	/// Reads a value of trivial type T (no pointers) from data as bytes
	template<typename T>
	T readSimple(const std::vector<std::uint8_t>& data, std::size_t& at) {
		union {
			T original;
			std::uint8_t bytes[sizeof(T)];
		} b2o = {};
		for (std::size_t i = 0; i < sizeof(T); ++i) {
			assert(at < data.size());
			b2o.bytes[i] = data[at];
			++at;
		}
		return b2o.original;
	}
	
	template<typename Bytes=BufferedBinaryFileOutput<>>
	void writeString(Bytes& data, const std::string& val) {
		for (const char& c : val) {
			writeSimple<char>(data, c);
		}
		writeSimple<char>(data, '\0');
	}

	std::string readString(const std::vector<std::uint8_t>& data, std::size_t& at);

	template<typename T, int N, typename Bytes=BufferedBinaryFileOutput<>>
	void writeVec (Bytes& data, const Vec<T, N>& val) {
		for (int i = 0; i < N; ++i) {
			writeSimple<T>(data, val[i]);
		}
	}

	template<typename T, int N>
	Vec<T, N> readVec (const std::vector<std::uint8_t>& data, std::size_t& at) {
		Vec<T, N> r;
		for (int i = 0; i < N; ++i) {
			r.set(i, readSimple<T>(data, at));
		}
		return r;
	}
	
	template<typename T, typename Bytes=BufferedBinaryFileOutput<>>
	void writeCollection (Bytes& data, const T& val) {
		writeSimple<std::uint32_t>(data, std::uint32_t(val.size()));
		for (auto it = val.begin(); it != val.end(); it++) {
			writeSimple(data, *it);
		}
	}
	
}

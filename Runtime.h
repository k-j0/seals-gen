#pragma once

#include <chrono>

class Runtime {
    
	std::chrono::system_clock clock;
	std::chrono::system_clock::time_point start = clock.now();
    long long& outMs;
    
public:
    
    Runtime (long long& outMs) : outMs(outMs) {
        start = clock.now();
    }
    
    long long getMs () const {
        auto end = clock.now();
        long long ms = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        return ms;
    }
    
    virtual ~Runtime () {
        auto ms = getMs();
        outMs = ms;
        std::printf("Runtime: ");
        if (ms > 1000) {
            auto s = ms / 1000;
            ms -= s * 1000;
            if (s > 60) {
                auto min = s / 60;
                s -= min * 60;
                if (min > 60) {
                    auto hr = min / 60;
                    min -= hr * 60;
                    std::printf("%02lldhr ", hr);
                }
                std::printf("%02lldmin ", min);
            }
            std::printf("%02llds ", s);
        }
        std::printf("%03lldms.\n\n", ms);
    }
    
};

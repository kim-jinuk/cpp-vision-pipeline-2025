#pragma once
#include <chrono>
#include <string>
#include <vector>
#include <fstream>

namespace cvp {
struct Stamp { std::string name; double ms; int frame; };
class Metrics {
public:
    void mark(const std::string& name, int frame) {
        using clk = std::chrono::high_resolution_clock;
        auto now = clk::now();
        double ms = std::chrono::duration<double, std::milli>(now.time_since_epoch()).count();
        stamps_.push_back({name, ms, frame});
    }

    void dump_csv(const std::string& path) {
        std::ofstream f(path);
        f << "frame,stage,timestamp_ms\n";
        for (auto& s : stamps_) f << s.frame << "," << s.name << "," << s.ms << "\n";
    }

private:
    std::vector<Stamp> stamps_;
};
}   // namespace cvp
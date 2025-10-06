#pragma once
#include "metrics.hpp"
#include <string>
#include <utility>

#define CVP_CONCAT_(a, b) a##b
#define CVP_CONCAT(a, b) CVP_CONCAT_(a, b)

#define CVP_TRACE_METRICS(metrics, stage, frame) \
    cvp::ScopeStamp CVP_CONCAT(_scope_stamp_, __LINE__)(metrics, stage, frame)

namespace cvp {
struct ScopeStamp {
    Metrics& m;
    std::string stage;
    int frame;

    ScopeStamp(Metrics& met, std::string s, int f) : m(met), stage(std::move(s)), frame(f) {
        m.mark(stage+":in", frame);
    }

    ~ScopeStamp() { m.mark(stage+":out", frame); }
};
}
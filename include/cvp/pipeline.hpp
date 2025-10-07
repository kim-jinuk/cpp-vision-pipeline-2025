#pragma once
#include "frame.hpp"
#include "metrics.hpp"
#include <functional>

namespace cvp {
struct Detection { cv::Rect box; float score; int cls; };
using Dets = std::vector<Detection>;

struct Pipeline {
    std::function<bool(Frame&)> capture;
    std::function<void(Frame&)> preprocess;
    std::function<Dets(const Frame&)> infer;
    std::function<Dets(const Dets&)> postprocess;
    std::function<void(Frame&, const Dets&)> overlay;
};
}
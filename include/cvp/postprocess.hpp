#pragma once
#include "pipeline.hpp"
namespace cvp { float IoU(const cv::Rect& a, const cv::Rect& b); Dets NMS(const Dets& ds, float thr); }
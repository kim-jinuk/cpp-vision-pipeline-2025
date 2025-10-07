#include "cvp/preprocess.hpp"
#include <opencv2/imgproc.hpp>
namespace cvp { void resize_inplace(Frame& f, int w, int h){ cv::resize(f.bgr, f.bgr, {w, h}); } }
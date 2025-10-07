#include <doctest/doctest.h>
#include "cvp/preprocess.hpp"
#include <opencv2/opencv.hpp>

TEST_CASE("resize_inplace"){
  cvp::Frame f{0, cv::Mat(100,100,CV_8UC3)};
  cvp::resize_inplace(f, 50, 40);
  CHECK(f.bgr.cols==50);
  CHECK(f.bgr.rows==40);
}
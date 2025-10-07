#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "cvp/postprocess.hpp"

TEST_CASE("IoU basic"){
  cv::Rect a(0,0,10,10), b(5,5,10,10);
  CHECK(cvp::IoU(a,b) > 0.0f);
}
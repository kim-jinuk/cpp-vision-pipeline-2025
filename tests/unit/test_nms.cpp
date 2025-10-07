#include <doctest/doctest.h>
#include "cvp/postprocess.hpp"

using namespace cvp;
TEST_CASE("NMS simple"){
  Dets ds{{{0,0,10,10},0.9f,0}, {{1,1,10,10},0.8f,0}};
  auto out = NMS(ds, 0.5f);
  CHECK(out.size()>=1);
}
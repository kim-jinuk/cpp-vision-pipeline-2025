#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <opencv2/opencv.hpp>
#include "cvp/pipeline.hpp"

using namespace cvp;

TEST_CASE("E2E smoke 50 frames"){
  cv::VideoCapture cap("data/samples/sample.mp4");
  REQUIRE(cap.isOpened());

  Pipeline pl; Frame fr; int count=0;
  pl.capture = [&](Frame& f){ cv::Mat m; if(!cap.read(m)) return false; f.id=++count; f.bgr=m; return true; };
  pl.preprocess = [&](Frame& f){};
  pl.infer = [&](const Frame& f){ return Dets{Detection{cv::Rect(10,10,30,30),0.5f,0}}; };
  pl.postprocess = [&](const Dets& d){ return d; };
  pl.overlay = [&](Frame& f, const Dets& d){};

  while(count<50){ if(!pl.capture(fr)) break; pl.preprocess(fr); auto d=pl.infer(fr); d=pl.postprocess(d); pl.overlay(fr,d); }
  CHECK(count>=30);
}
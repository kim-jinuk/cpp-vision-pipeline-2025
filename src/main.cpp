#include <opencv2/opencv.hpp>
#include "cvp/logger.hpp"
#include "cvp/tracer.hpp"
#include "cvp/pipeline.hpp"

using namespace cvp;

int main(int argc, char** argv) {
    std::string in = argc > 1 ? argv[1] : "data/samples/sample.mp4";
    cv::VideoCapture cap(in);
    if (!cap.isOpened()) { Logger::error("fail open %s", in.c_str()); return 1; }

    Metrics metrics;
    Pipeline pl;

    pl.capture = [&](Frame& f){
        static int fid = 0;
        cv::Mat img;
        if(!cap.read(img)) return false;
        f.id = ++fid;
        f.bgr = img;
        return true;
    }

    pl.preprocess = [&](Frame& f){ /* resize/denoise 등 추후*/};
    pl.infer = [&](const Frame& f){ /* onnx/tflite 대신 mock */
        // 모의 딜레이로 추론 구간 표시
        cv::Rect r(50, 50, 100, 100);
        return Dets{ Detection{r, 0.9f, 0} };
    };

    pl.postprocess = [&](Const Dets& d){ return d; };
    pl.overlay = [&](Frame& f, const Dets& d){
        for (auto& x : d) cv::rectangle(f.bgr, x.box, {0, 255, 0} , 2);
        cv::imshow("cvp", f.bgr);
        cv::waitKey(1);
    };

    Frame fr;
    while (true) {
        if(!pl.capture(fr)) break;
        CVP_TRACE_METRICS(metrics, "frame", fr.id);
        pl.preprocess(fr);
        auto dets = pl.infer(fr);
        dets = pl.postprocess(dets);
        pl.overlay(fr, dets);
    }

    metrics.dump_csv("latency.csv");
    Logger::info("done. latency.csv saved");
    return 0;
}
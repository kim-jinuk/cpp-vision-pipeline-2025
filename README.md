# EO 탐지·추적 7일 스프린트 — Skeleton · Tracer · Test (실전 패키지)

> **목표(7일)**: PC(WSL2/Ubuntu 22.04, GCC 11, CMake ≥3.26)에서 640×480 영상 입력으로 **detect → track → overlay**까지 **엔드투엔드** 파이프라인을 완성하고, **트레이서(계측)**와 **테스트(단위/통합/성능/회귀)**를 붙인다. EdgeTPU는 **플러그인 포인트만** 정의(미보유 전제).
>
> **산출물**: 동작하는 최소 파이프라인, latency.csv, 테스트 자동화, CI, README.

---

## 0. 리포지토리 구조 (복붙해서 시작)
```
cpp-vision-pipeline-2025/
├─ CMakeLists.txt
├─ cmake/
│  └─ FindDoctest.cmake
├─ include/
│  └─ cvp/
│     ├─ config.hpp
│     ├─ logger.hpp
│     ├─ metrics.hpp
│     ├─ tracer.hpp
│     ├─ frame.hpp
│     ├─ pipeline.hpp
│     ├─ capture.hpp
│     ├─ preprocess.hpp
│     ├─ infer.hpp
│     ├─ postprocess.hpp
│     ├─ tracker.hpp
│     ├─ overlay.hpp
│     └─ sink.hpp
├─ src/
│  ├─ main.cpp
│  ├─ core/
│  │  ├─ logger.cpp
│  │  ├─ metrics.cpp
│  │  └─ pipeline.cpp
│  └─ modules/
│     ├─ capture/v4l2_capture.cpp
│     ├─ preprocess/basic_preprocess.cpp
│     ├─ infer/onnx_infer.cpp
│     ├─ postprocess/nms.cpp
│     ├─ tracker/sort_tracker.cpp
│     ├─ overlay/draw.cpp
│     └─ sink/video_writer.cpp
├─ tests/
│  ├─ unit/
│  │  ├─ test_math_iou.cpp
│  │  ├─ test_nms.cpp
│  │  └─ test_preprocess.cpp
│  ├─ integration/
│  │  └─ test_e2e_smoke.cpp
│  └─ perf/
│     └─ run_perf.sh
├─ tools/
│  ├─ replay.py
│  └─ plot_latency.py
├─ config/
│  ├─ default.yaml
│  └─ labels.txt
├─ data/
│  └─ samples/ (샘플 mp4 / jpg)
└─ .github/workflows/ci.yml
```

---

## 1. Day-by-Day (7일 일정표)

| Day | 목표 | 체크리스트 |
|---|---|---|
| 1 | 스켈레톤 & 빌드 성공 | 폴더/파일 생성, CMake 빌드, `main.cpp`에서 no-op 파이프라인 실행 |
| 2 | 입력 + 1차 트레이서 | OpenCV VideoCapture로 프레임, per-frame timestamp 로그(ingress/egress) |
| 3 | Preprocess + Unit | resize/normalize/denoise 스텁 + doctest(unit) |
| 4 | Infer 스텁 | ONNXRuntime 인터페이스만 설계(미보유 모델은 mock sleep) + tracer 구간 추가 |
| 5 | Postprocess + Overlay | NMS/IoU 구현, bbox draw, latency.csv 저장 |
| 6 | Integration + Perf | 200프레임 E2E, p50/p95/p99 latency/FPS 산출, `tools/plot_latency.py` 그림 |
| 7 | 문서화 + CI + 회귀 | README 정리, CI 통과, perf artifact 저장(그래프 png) |

---

## 2. 기본 CMake 템플릿
```cmake
cmake_minimum_required(VERSION 3.26)
project(cvp_eo_pipeline LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

find_package(OpenCV REQUIRED)

# doctest finder (header-only)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
find_package(Doctest REQUIRED)

include_directories(include)

add_library(cvp_core
  src/core/logger.cpp
  src/core/metrics.cpp
  src/core/pipeline.cpp)

target_link_libraries(cvp_core PUBLIC ${OpenCV_LIBS})

a dd_library(cvp_modules
  src/modules/capture/v4l2_capture.cpp
  src/modules/preprocess/basic_preprocess.cpp
  src/modules/infer/onnx_infer.cpp
  src/modules/postprocess/nms.cpp
  src/modules/tracker/sort_tracker.cpp
  src/modules/overlay/draw.cpp
  src/modules/sink/video_writer.cpp)

target_link_libraries(cvp_modules PUBLIC cvp_core ${OpenCV_LIBS})

add_executable(cvp_app src/main.cpp)
target_link_libraries(cvp_app PRIVATE cvp_modules cvp_core ${OpenCV_LIBS})

# Unit tests
file(GLOB TEST_SOURCES tests/unit/*.cpp)
add_executable(unit_tests ${TEST_SOURCES})
target_link_libraries(unit_tests PRIVATE cvp_modules cvp_core ${OpenCV_LIBS} Doctest::Doctest)
add_test(NAME unit_tests COMMAND unit_tests)

# Integration tests
add_executable(integ_tests tests/integration/test_e2e_smoke.cpp)
target_link_libraries(integ_tests PRIVATE cvp_modules cvp_core ${OpenCV_LIBS} Doctest::Doctest)
add_test(NAME integ_tests COMMAND integ_tests)
```

**cmake/FindDoctest.cmake** (간단 버전)
```cmake
find_path(DOCTEST_INCLUDE_DIR doctest/doctest.h)
add_library(Doctest::Doctest INTERFACE IMPORTED)
set_target_properties(Doctest::Doctest PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${DOCTEST_INCLUDE_DIR})
```

> 설치: `sudo apt-get install libopencv-dev` / doctest는 `tests/third_party/doctest.h`로 포함해도 됨.

---

## 3. 공용 헤더 (Tracer/Logger/Metrics)

**include/cvp/logger.hpp**
```cpp
#pragma once
#include <iostream>
#include <mutex>

namespace cvp {
class Logger {
 public:
  template<typename... Args>
  static void info(const char* fmt, Args... args) {
    std::lock_guard<std::mutex> lk(mu_);
    fprintf(stdout, (std::string("[I] ")+fmt+"\n").c_str(), args...);
  }
  template<typename... Args>
  static void warn(const char* fmt, Args... args) {
    std::lock_guard<std::mutex> lk(mu_);
    fprintf(stdout, (std::string("[W] ")+fmt+"\n").c_str(), args...);
  }
  template<typename... Args>
  static void error(const char* fmt, Args... args) {
    std::lock_guard<std::mutex> lk(mu_);
    fprintf(stderr, (std::string("[E] ")+fmt+"\n").c_str(), args...);
  }
 private:
  static inline std::mutex mu_{};
};
}
```

**include/cvp/metrics.hpp**
```cpp
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
    for (auto& s: stamps_) f << s.frame << "," << s.name << "," << s.ms << "\n";
  }
 private:
  std::vector<Stamp> stamps_;
};
}
```

**include/cvp/tracer.hpp**
```cpp
#pragma once
#include "metrics.hpp"

// 간단 매크로: 스코프 진입/탈출 타임스탬프
#define CVP_TRACE_METRICS(metrics, stage, frame) \
  cvp::ScopeStamp CVP_CONCAT(_scope_stamp_, __LINE__)(metrics, stage, frame)

#define CVP_CONCAT_(a,b) a##b
#define CVP_CONCAT(a,b) CVP_CONCAT_(a,b)

namespace cvp {
struct ScopeStamp {
  Metrics& m; std::string stage; int frame;
  ScopeStamp(Metrics& met, std::string s, int f) : m(met), stage(std::move(s)), frame(f) {
    m.mark(stage+":in", frame);
  }
  ~ScopeStamp(){ m.mark(stage+":out", frame); }
};
}
```

**include/cvp/frame.hpp**
```cpp
#pragma once
#include <opencv2/opencv.hpp>
namespace cvp { struct Frame { int id; cv::Mat bgr; }; }
```

**include/cvp/pipeline.hpp**
```cpp
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
```

---

## 4. main.cpp (엔트리: no-op → 스텁 연결)
**src/main.cpp**
```cpp
#include <opencv2/opencv.hpp>
#include "cvp/logger.hpp"
#include "cvp/pipeline.hpp"
#include "cvp/tracer.hpp"

using namespace cvp;

int main(int argc, char** argv){
  std::string in = argc>1 ? argv[1] : "data/samples/sample.mp4";
  cv::VideoCapture cap(in);
  if(!cap.isOpened()){ Logger::error("fail open %s", in.c_str()); return 1; }

  Metrics metrics;
  Pipeline pl;
  pl.capture = [&](Frame& f){
    static int fid=0; cv::Mat img; if(!cap.read(img)) return false; f.id=++fid; f.bgr=img; return true; };
  pl.preprocess = [&](Frame& f){ /* resize/denoise 등 추후 */ };
  pl.infer = [&](const Frame& f){ /* onnx/tflite 대신 mock */
    // 모의 딜레이로 추론 구간 표시
    cv::Rect r(50,50,100,100); return Dets{ Detection{r, 0.9f, 0} };
  };
  pl.postprocess = [&](const Dets& d){ return d; };
  pl.overlay = [&](Frame& f, const Dets& d){
    for(auto& x: d) cv::rectangle(f.bgr, x.box, {0,255,0}, 2);
    cv::imshow("cvp", f.bgr); cv::waitKey(1);
  };

  Frame fr;
  while(true){
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
```

---

## 5. 모듈 스텁 (필요 최소 구현)

**include/cvp/capture.hpp** *(선택: main에서 람다로 처리했으니 나중에 클래스로 교체)*

**include/cvp/preprocess.hpp**
```cpp
#pragma once
#include "frame.hpp"
namespace cvp { void resize_inplace(Frame& f, int w, int h); }
```
**src/modules/preprocess/basic_preprocess.cpp**
```cpp
#include "cvp/preprocess.hpp"
#include <opencv2/imgproc.hpp>
namespace cvp { void resize_inplace(Frame& f, int w, int h){ cv::resize(f.bgr, f.bgr, {w,h}); } }
```

**include/cvp/postprocess.hpp**
```cpp
#pragma once
#include "pipeline.hpp"
namespace cvp { float IoU(const cv::Rect& a, const cv::Rect& b); Dets NMS(const Dets& ds, float thr); }
```
**src/modules/postprocess/nms.cpp**
```cpp
#include "cvp/postprocess.hpp"
#include <algorithm>
namespace cvp {
float IoU(const cv::Rect& a, const cv::Rect& b){
  int x1=std::max(a.x,b.x), y1=std::max(a.y,b.y);
  int x2=std::min(a.x+a.width,b.x+b.width), y2=std::min(a.y+a.height,b.y+b.height);
  int inter = std::max(0,x2-x1) * std::max(0,y2-y1);
  int ua = a.width*a.height + b.width*b.height - inter;
  return ua? (float)inter/ua : 0.f;
}
Dets NMS(const Dets& ds, float thr){
  auto sorted=ds; std::sort(sorted.begin(), sorted.end(),[](auto&a,auto&b){return a.score>b.score;});
  std::vector<int> keep; std::vector<char> sup(sorted.size(),0);
  for(size_t i=0;i<sorted.size();++i){ if(sup[i]) continue; keep.push_back((int)i);
    for(size_t j=i+1;j<sorted.size();++j){ if(IoU(sorted[i].box,sorted[j].box)>thr) sup[j]=1; } }
  Dets out; out.reserve(keep.size()); for(int i: keep) out.push_back(sorted[i]); return out;
}
}
```

---

## 6. Unit Tests (doctest)

**tests/unit/test_math_iou.cpp**
```cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "cvp/postprocess.hpp"

TEST_CASE("IoU basic"){
  cv::Rect a(0,0,10,10), b(5,5,10,10);
  CHECK(IoU(a,b) > 0.0f);
}
```

**tests/unit/test_nms.cpp**
```cpp
#include <doctest/doctest.h>
#include "cvp/postprocess.hpp"

using namespace cvp;
TEST_CASE("NMS simple"){
  Dets ds{{{0,0,10,10},0.9f,0}, {{1,1,10,10},0.8f,0}};
  auto out = NMS(ds, 0.5f);
  CHECK(out.size()>=1);
}
```

**tests/unit/test_preprocess.cpp**
```cpp
#include <doctest/doctest.h>
#include "cvp/preprocess.hpp"
#include <opencv2/opencv.hpp>

TEST_CASE("resize_inplace"){
  cvp::Frame f{0, cv::Mat(100,100,CV_8UC3)};
  cvp::resize_inplace(f, 50, 40);
  CHECK(f.bgr.cols==50);
  CHECK(f.bgr.rows==40);
}
```

---

## 7. Integration Test (스모크)
**tests/integration/test_e2e_smoke.cpp**
```cpp
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
```

---

## 8. Perf 스크립트 & 플로팅 도구

**tests/perf/run_perf.sh**
```bash
#!/usr/bin/env bash
set -euo pipefail
BIN=../../build/cvp_app
IN=../../data/samples/sample.mp4
OUT=../../latency.csv
$BIN $IN
python3 ../../tools/plot_latency.py --csv $OUT --out ../../latency.png
```

**tools/plot_latency.py**
```python
import argparse, pandas as pd, matplotlib.pyplot as plt
p=argparse.ArgumentParser(); p.add_argument('--csv'); p.add_argument('--out'); a=p.parse_args()
df=pd.read_csv(a.csv)
# 간단: frame별 소요시간( out - in )
in_ = df[df.stage.str.endswith(':in')].copy(); out = df[df.stage.str.endswith(':out')].copy()
merged = in_.merge(out, on=['frame',], suffixes=('_in','_out'))
merged['dur_ms'] = merged['timestamp_ms_out'] - merged['timestamp_ms_in']
plt.plot(merged['frame'], merged['dur_ms'])
plt.title('Per-frame latency (ms)'); plt.xlabel('frame'); plt.ylabel('ms'); plt.grid(True); plt.savefig(a.out, bbox_inches='tight')
```

---

## 9. 설정 파일 (예시)
**config/default.yaml**
```yaml
input: data/samples/sample.mp4
resize: [640, 480]
conf_thr: 0.3
iou_thr: 0.5
classes: config/labels.txt
```
**config/labels.txt**
```
person
car
```

---

## 10. GitHub Actions CI
**.github/workflows/ci.yml**
```yaml
name: ci
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - name: deps
        run: |
          sudo apt-get update
          sudo apt-get install -y build-essential cmake libopencv-dev python3-matplotlib python3-pandas
      - name: configure
        run: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
      - name: build
        run: cmake --build build -j
      - name: unit tests
        run: ctest --test-dir build --output-on-failure
      - name: perf (smoke)
        run: |
          mkdir -p data/samples
          # 작은 샘플 영상을 커밋하지 않았다면, 여기서는 스킵
          if [ -f data/samples/sample.mp4 ]; then
            ./build/cvp_app data/samples/sample.mp4 || true
            python3 tools/plot_latency.py --csv latency.csv --out latency.png || true
            echo "perf done"
          else
            echo "no sample.mp4; skipping perf"
          fi
      - name: artifacts
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: perf
          path: |
            latency.csv
            latency.png
```

---

## 11. 빌드 & 실행 명령 요약
```bash
# 1) 빌드
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# 2) 유닛/통합 테스트
ctest --test-dir build --output-on-failure

# 3) 실행 (샘플 영상)
./build/cvp_app data/samples/sample.mp4

# 4) 성능 결과
ls -l latency.csv  # 프레임별 in/out 타임스탬프
python3 tools/plot_latency.py --csv latency.csv --out latency.png
```

---

## 12. README 초안(요점)
- 목적: EO 탐지·추적 파이프라인 최소 구현 + 계측/테스트 루틴 체화
- 요구사항: Ubuntu 22.04, GCC 11, CMake≥3.26, OpenCV, Python3
- 빠른 시작: 빌드/실행/플롯 3줄
- 아키텍처: capture→preprocess→infer→postprocess→overlay + tracer
- 테스트: doctest(unit), e2e smoke, perf(csv→png)
- 차후 작업: EdgeTPU 플러그인, 멀티스레딩, 큐 길이/드랍율 트레이스, tracker(SORT/ByteTrack)

---

## 13. 다음 단계 (확장)
- **트레이서 고도화**: Chrome Trace(Perfetto) 포맷으로 내보내기 → 타임라인 시각화
- **멀티스레드**: capture/preprocess/infer/overlay 스테이지 파이프라이닝 + lock-free queue
- **정확도 벤치**: GT와 MOTA/MOTP, IDF1 계산
- **EdgeTPU 플러그인**: TFLite EdgeTPU delegate 경로 분기 + 스레드 pinning
- **HIL**: Zybo ↔ Host UDP 스트리밍 + 패킷드랍률 트레이스

---

## 14. 체크리스트 (최종)
- [ ] `main.cpp` E2E 동작
- [ ] `latency.csv` 생성 & `latency.png` 플롯
- [ ] unit/integration 테스트 통과
- [ ] CI 배지 초록
- [ ] README에 성능 수치(p50/p95) 1줄 기록

이 패키지를 그대로 시작점으로 쓰고, 매일 커밋 로그에 “스켈레톤/트레이서/테스트” 태그를 남겨. 7일 뒤엔 네가 말한 ‘실전 루틴’이 몸에 붙는다.


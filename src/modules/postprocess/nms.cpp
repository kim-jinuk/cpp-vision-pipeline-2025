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
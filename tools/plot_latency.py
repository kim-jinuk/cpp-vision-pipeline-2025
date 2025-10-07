import argparse, pandas as pd, matplotlib.pyplot as plt
p=argparse.ArgumentParser(); p.add_argument('--csv'); p.add_argument('--out'); a=p.parse_args()
df=pd.read_csv(a.csv)
# 간단: frame별 소요시간( out - in )
in_ = df[df.stage.str.endswith(':in')].copy(); out = df[df.stage.str.endswith(':out')].copy()
merged = in_.merge(out, on=['frame',], suffixes=('_in','_out'))
merged['dur_ms'] = merged['timestamp_ms_out'] - merged['timestamp_ms_in']
plt.plot(merged['frame'], merged['dur_ms'])
plt.title('Per-frame latency (ms)'); plt.xlabel('frame'); plt.ylabel('ms'); plt.grid(True); plt.savefig(a.out, bbox_inches='tight')
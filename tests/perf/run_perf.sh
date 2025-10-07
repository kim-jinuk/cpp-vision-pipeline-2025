#!/usr/bin/env bash
set -euo pipefail
BIN=../../build/cvp_app
IN=../../data/samples/sample.mp4
OUT=../../latency.csv
$BIN $IN
python3 ../../tools/plot_latency.py --csv $OUT --out ../../latency.png
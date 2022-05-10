#!/bin/sh
QUICK_START_DIR=`dirname $0`
git clone https://github.com/google/sentencepiece.git
${QUICK_START_DIR}/../scripts/mine_patterns.sh -d sentencepiece -o sentencepiece_training_data.ts -l 4
git clone https://github.com/google/xrtl.git
mkdir test3_scan_output
${QUICK_START_DIR}/../scripts/scan_for_anomalies.sh -d xrtl/ -t sentencepiece_training_data.ts -o test3_scan_output -l 4
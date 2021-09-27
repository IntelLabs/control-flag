#!/bin/sh
QUICK_START_DIR=`dirname $0`
git clone https://github.com/github/glb-director.git
${QUICK_START_DIR}/../scripts/mine_patterns.sh -d glb-director -o glb_director_training_data.ts -l 1
git clone https://github.com/github/brubeck.git
mkdir test1_scan_output
${QUICK_START_DIR}/../scripts/scan_for_anomalies.sh -d brubeck/ -t glb_director_training_data.ts -o test1_scan_output -l 1

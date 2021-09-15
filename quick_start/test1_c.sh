#!/bin/sh
git clone https://github.com/github/glb-director.git
../scripts/mine_patterns.sh -d glb-director -o glb_director_training_data.ts -l 1
git clone https://github.com/github/brubeck.git
mkdir test1_scan_output
../scripts/scan_for_anomalies.sh -d brubeck/ -t glb_director_training_data.ts -o test1_scan_output -l 1

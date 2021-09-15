#!/bin/sh
git clone https://github.com/tommythorn/yarvi.git
../scripts/mine_patterns.sh -d yarvi -o yarvi_training_data.ts -l 2
git clone https://github.com/ddk/ddk-fpga.git
mkdir test2_scan_output
../scripts/scan_for_anomalies.sh -d ddk-fpga -t yarvi_training_data.ts -o test2_scan_output -l 2

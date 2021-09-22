#!/bin/sh

function print_usage() {
  echo -n "Usage: $1 -t <training_data>"
  echo " -d <directory_to_scan_for_anomalous_patterns>"
  echo "Optional:"
  echo " [-c max_cost_for_autocorrect]              (default: 2)"
  echo " [-n max_number_of_results_for_autocorrect] (default: 5)"
  echo " [-j number_of_scanning_threads]            (default: num_cpus_on_systems)"
  echo " [-o output_log_dir]                        (default: /tmp)"
  echo " [-a anomaly_threshold]                     (default: 3.0)"
  echo " [-l source_language_number]                (default: 1 (C), supported: 1 (C), 2 (Verilog)"

  exit
}

OUTPUT_DIR="/tmp"
MAX_AUTOCORRECT_COST=2
MAX_AUTOCORRECT_RESULTS=5
NUM_SCAN_THREADS=`nproc`
ANOMALY_THRESHOLD=3
LANGUAGE=1

while getopts d:t:o:c:n:j:a:l: flag
do
  case "${flag}" in
    d) SCAN_DIR=${OPTARG};;
    t) TRAIN_FILE=${OPTARG};;
    o) OUTPUT_DIR=${OPTARG};;
    c) MAX_AUTOCORRECT_COST=${OPTARG};;
    n) MAX_AUTOCORRECT_RESULTS=${OPTARG};;
    j) NUM_SCAN_THREADS=${OPTARG};;
    a) ANOMALY_THRESHOLD=${OPTARG};;
    l) LANGUAGE=${OPTARG};;
  esac
done

if [ "${SCAN_DIR}" = "" ] || [ "${TRAIN_FILE}" = "" ] ||
   [ ! -d "${SCAN_DIR}" ] || [ ! -f "${TRAIN_FILE}" ]
then
  echo "ERROR: $0 requires training data file and a directory to scan for anomalies"
  print_usage $0
fi

if [ ! -d "${OUTPUT_DIR}" ]
then
  echo "ERROR: output directory is not a directory."
  print_usage $0
fi

if (( ${LANGUAGE} < 1  || ${LANGUAGE} > 2 ));
then
  echo "ERROR: Only 1 (C) and 2 (Verilog) are supported languages; received ${LANGUAGE}"
  print_usage $0
fi 

SCAN_FILE_LIST=`mktemp -p ~/tmp`
if [ "${LANGUAGE}" = "1" ];
then
  find "${SCAN_DIR}" -iname "*.c" -o -iname "*.h" -type f > ${SCAN_FILE_LIST}
else
  find "${SCAN_DIR}" -iname "*.v" -o -iname "*.vh" -type f > ${SCAN_FILE_LIST}
fi

SCRIPTS_DIR=`dirname $0`

${SCRIPTS_DIR}/../bin/cf_file_scanner -t ${TRAIN_FILE} \
-s ${SCAN_FILE_LIST} \
-c ${MAX_AUTOCORRECT_COST} \
-n ${MAX_AUTOCORRECT_RESULTS} \
-j ${NUM_SCAN_THREADS} \
-o ${OUTPUT_DIR} \
-a ${ANOMALY_THRESHOLD} \
-l ${LANGUAGE}

rm ${SCAN_FILE_LIST}

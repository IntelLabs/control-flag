#!/bin/bash

function print_usage() {
  echo -n "Usage: $1 -d <directory_to_mine_patterns_from>"
  echo " -o <output_file_to_store_training_data>"
  echo "Optional:"
  if ! command -v nproc &> /dev/null
  then
    echo "[-n number_of_processes_to_use_for_mining]  (default: 1)"
  else
    echo "[-n number_of_processes_to_use_for_mining]  (default: num_cpus_on_system)"
  fi
  echo "[-l source_language_number] (default: 1 (C), supported: 1 (C), 2 (Verilog), 3 (PHP)"

  exit
}

# Default values
if ! command -v nproc &> /dev/null
then
  NUM_MINER_PROCS=1
else
  NUM_MINER_PROCS=`nproc`
fi
LANGUAGE=1

while getopts d:o:n:l: flag
do
  case "${flag}" in
    d) TRAIN_DIR=${OPTARG};;
    o) OUTPUT_FILE=${OPTARG};;
    n) NUM_MINER_PROCS=${OPTARG};;
    l) LANGUAGE=${OPTARG};;
  esac
done

if [ "${TRAIN_DIR}" = "" ] || [ "${OUTPUT_FILE}" = "" ] ||
   [ ! -d "${TRAIN_DIR}" ]
then
  echo "ERROR: $0 requires a directory to mine patterns and an output file to store them"
  print_usage $0
fi

if (( ${LANGUAGE} < 1  || ${LANGUAGE} > 3 ));
then
  echo "ERROR: Only 1 (C), 2 (Verilog) and 3 (PHP) are supported languages; received ${LANGUAGE}"
  print_usage $0
fi

if [ -f "${OUTPUT_FILE}" ]
then
  echo "ERROR: Output file exists. We don't want to over-write it."
  print_usage $0
fi

TMP_DIR=`mktemp -d`
FILE_LIST=${TMP_DIR}/file_list.txt
if [ "${LANGUAGE}" = "1" ];
then
  find "${TRAIN_DIR}" -iname "*.c" -o -iname "*.h" -type f > ${FILE_LIST}
elif [ "${LANGUAGE}" = "3" ];
then
  find "${TRAIN_DIR}" -iname "*.php" -type f | fgrep -v "/vendor/" > ${FILE_LIST}
else
  find "${TRAIN_DIR}" -iname "*.v" -o -iname "*.vh" -type f > ${FILE_LIST}
fi

SCRIPTS_DIR=`dirname $0`
function dump_code_blocks() {
  id=`echo "$1" | cut -d ':' -f 1`
  f=`echo  "$1" | cut -d ':' -f 2-`
  ${SCRIPTS_DIR}/../bin/cf_dump_code_blocks -f "$f" -t 100 -g ${id} -l ${LANGUAGE} >> $2
}
export -f dump_code_blocks
export LANGUAGE
export SCRIPTS_DIR

if ! command -v parallel &> /dev/null
then
  echo "GNU Parallel does not exist. Invoking serial dump.."
  for id_f in `cat $FILE_LIST`;
  do
    dump_code_blocks ${id_f} ${OUTPUT_FILE}
  done

else

  echo "GNU Parallel exists. Invoking parallel dump.."
  cat ${FILE_LIST} | parallel --eta --bar --progress \
    -I% -j0 dump_code_blocks % ${TMP_DIR}/proc_{%}.log

  for i in `seq 1 $NUM_MINER_PROCS`;
  do
    if [ -f ${TMP_DIR}/proc_${i}.log ]
    then
      cat ${TMP_DIR}/proc_${i}.log >> ${OUTPUT_FILE}
    fi
  done
fi

rm -rf ${TMP_DIR}

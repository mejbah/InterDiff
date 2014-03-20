#!/bin/sh
DATA_DIR=$1
OUTPUT_DIR=$2
EXEC_NO=$3
## $3 :: 0 for train

echo "$APP_NAME"
echo "Exec $EXEC_NO"
APP_DATA=/home/mejbah/pintool/pin-2.10-45467-gcc.3.4.6-ia32_intel64-linux/source/tools/Pin_ins_dep/results/$APP_NAME
OUTPUT_DIR=/home/mejbah/DiffExec
DATA_DIR=/home/mejbah/pintool/pin-2.10-45467-gcc.3.4.6-ia32_intel64-linux/source/tools/Pin_ins_dep/results/$APP_NAME/run$EXEC_NO
BIN_DIR=/home/mejbah/pintool/pin-2.10-45467-gcc.3.4.6-ia32_intel64-linux/source/tools/Pin_ins_dep
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/mejbah/pintool/pin-2.10-45467-gcc.3.4.6-ia32_intel64-linux/intel64/lib-ext:/home/mejbah/opencv-libs/lib
export LD_LIBRARY_PATH

echo "./analyzer config $DATA_DIR/FFT.out.compact $OUTPUT_DIR $EXEC_NO"
./analyzer config $DATA_DIR/FFT.out.compact $OUTPUT_DIR $EXEC_NO



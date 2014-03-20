#!/bin/sh
EXEC_NO=$1
CHECK=$2
APP_NAME=$3
BENCHMARK_PATH=/home/mejbah/splash2
KERNELS_PATH=$BENCHMARK_PATH/codes/kernels
APPS_PATH=$BENCHMARK_PATH/codes/apps

#INPUT_PATH=/home/mejbah/bugbenchmarks/mysql644/
PIN_HOME=/home/mejbah/pintool/pin-2.10-45467-gcc.3.4.6-ia32_intel64-linux
OUTPUT_PATH=/home/mejbah/pintool/pin-2.10-45467-gcc.3.4.6-ia32_intel64-linux/source/tools/Pin_ins_dep/results


echo "Execution no $1"
if [ $1 -eq 1 ]
then
mkdir $OUTPUT_PATH/$APP_NAME
fi

mkdir $OUTPUT_PATH/$APP_NAME/run$1
case $APP_NAME in
	"fft" )
	ARGS=" -p4"
	BIN_PATH=$KERNELS_PATH/$APP_NAME
	echo ${PIN_HOME}/pin -mt -t ./nova_modules.so -c ./data/FFT.config -o ${OUTPUT_PATH}/${APP_NAME}/run$1/FFT.out -- ${BIN_PATH}/FFT $ARGS
	${PIN_HOME}/pin -mt -t ./nova_modules.so -c ./data/FFT.config -o ${OUTPUT_PATH}/${APP_NAME}/run$1/FFT.out  -d $CHECK -- ${BIN_PATH}/FFT $ARGS
        ;;
	"radix" )
	ARGS=" -p4"
	BIN_PATH=$KERNELS_PATH/$APP_NAME
	echo ${PIN_HOME}/pin -mt -t ./nova_modules.so -c ./data/FFT.config -o ${OUTPUT_PATH}/${APP_NAME}/run$1/FFT.out -- ${BIN_PATH}/RADIX $ARGS
	${PIN_HOME}/pin -mt -t ./nova_modules.so -c ./data/FFT.config -o ${OUTPUT_PATH}/${APP_NAME}/run$1/FFT.out  -d $CHECK -- ${BIN_PATH}/RADIX $ARGS
        ;;
	"lu" )
	ARGS=" -p4"
	BIN_PATH=$KERNELS_PATH/lu-cont
	echo ${PIN_HOME}/pin -mt -t ./nova_modules.so -c ./data/FFT.config -o ${OUTPUT_PATH}/${APP_NAME}/run$1/FFT.out -- ${BIN_PATH}/LU $ARGS
	${PIN_HOME}/pin -mt -t ./nova_modules.so -c ./data/FFT.config -o ${OUTPUT_PATH}/${APP_NAME}/run$1/FFT.out  -d $CHECK -- ${BIN_PATH}/LU $ARGS
        ;;
	"fmm" )	
	BIN_PATH=$APPS_PATH/fmm
        INPUT_PATH=$APPS_PATH/fmm/inputs

	echo ${PIN_HOME}/pin -mt -t ./nova_modules.so -c ./data/FFT.config -o ${OUTPUT_PATH}/${APP_NAME}/run$1/FFT.out -stdin $INPUT_PATH/input.256 -- ${BIN_PATH}/FMM 
	${PIN_HOME}/pin -mt -t ./nova_modules.so -c ./data/FFT.config -o ${OUTPUT_PATH}/${APP_NAME}/run$1/FFT.out  -d $CHECK -stdin $INPUT_PATH/input.256 -- ${BIN_PATH}/FMM $ARGS
        ;;
	"volrend" )
	INPUT_PATH=$APPS_PATH/$APP_NAME/inputs
	ARGS=" 4 $INPUT_PATH/head-scaleddown4"
	BIN_PATH=$APPS_PATH/volrend
	echo ${PIN_HOME}/pin -mt -t ./nova_modules.so -c ./data/FFT.config -o ${OUTPUT_PATH}/${APP_NAME}/run$1/FFT.out -- ${BIN_PATH}/VOLREND $ARGS
	${PIN_HOME}/pin -mt -t ./nova_modules.so -c ./data/FFT.config -o ${OUTPUT_PATH}/${APP_NAME}/run$1/FFT.out  -d $CHECK -- ${BIN_PATH}/VOLREND $ARGS
        ;;

esac
echo done


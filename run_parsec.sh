#!/bin/bash
# $1 - program_name
# $2 - input_size - test , small
# $3 - no of ReExecution 
# $4 - [Config file]

BENCHMARK_DIR=/home/mejbah/benchmark/parsec-2.1_pinsesc/pkgs
BIN_DIR=/inst/amd64-linux.gcc-pthreads/bin/
PIN_HOME=/home/mejbah/pintool/pin-2.10-45467-gcc.3.4.6-ia32_intel64-linux
CONF_NAME=$4
NO_EXEC=$3

NTHREADS=4

mkdir -p ./data
echo "Running test for $1"

case $1 in
	"canneal" ) APP_NAME=canneal
	PKG_PART=/kernels/canneal
	INPUT=${BENCHMARK_DIR}${PKG_PART}/inputs
	
	case $2 in
		"test" ) tar -xvf ${INPUT}/input_test.tar -C ${INPUT}/;
		RUN_ARGS="${NTHREADS} 5 100 ${INPUT}/10.nets 1"
		;;
		"small" ) tar -xvf ${INPUT}/input_simsmall.tar -C ${INPUT}/;
		RUN_ARGS="${NTHREADS} 10000 2000 ${INPUT}/100000.nets 32"
		;;		
	esac
	;;
	"streamcluster" ) APP_NAME=streamcluster
	PKG_PART=/kernels/streamcluster
        OUTPUT=/run/output.txt
	case $2 in
		"test" ) RUN_ARGS="2 5 1 10 10 5 none ${BENCHMARK_DIR}${PKG_PART}${OUTPUT} ${NTHREADS}";;
		"small" ) RUN_ARGS="10 20 32 4096 4096 1000 none ${BENCHMARK_DIR}${PKG_PART}${OUTPUT} ${NTHREADS}";;
		
	esac;;

	"blackscholes" ) APP_NAME=blackscholes
	PKG_PART=/apps/blackscholes
	INPUT=${BENCHMARK_DIR}${PKG_PART}/inputs
        OUTPUT=${BENCHMARK_DIR}${PKG_PART}/run
	case $2 in
		"test" ) tar -xvf ${INPUT}/input_test.tar -C ${INPUT}/;
		RUN_ARGS="${NTHREADS} ${INPUT}/in_4.txt ${OUTPUT}/prices.txt ";;
		"small" ) tar -xvf ${INPUT}/input_simsmall.tar -C ${INPUT}/;
		RUN_ARGS="${NTHREADS} ${INPUT}/in_4K.txt ${OUTPUT}/prices.txt ";;
		
	esac;;


	"bodytrack" ) APP_NAME=bodytrack
	PKG_PART=/apps/bodytrack
	INPUT=${BENCHMARK_DIR}${PKG_PART}/inputs
	
	case $2 in
		"test" ) tar -xvf ${INPUT}/input_test.tar -C ${INPUT}/;
		RUN_ARGS="${INPUT}/sequenceB_1 4 1 5 1 0  ${NTHREADS}";;	
		"small" ) tar -xvf ${INPUT}/input_simsmall.tar -C ${INPUT}/;
		RUN_ARGS="${INPUT}/sequenceB_1 4 1 1000 5 0  ${NTHREADS}";;		
	esac;;


	"facesim" ) APP_NAME=facesim
	PKG_PART=/apps/facesim
	INPUT=${BENCHMARK_DIR}${PKG_PART}/inputs
	case $2 in
		"test" ) tar -xvf ${INPUT}/input_native.tar; # for facesim the input folder has to be in the current folder
		RUN_ARGS="-h";;
		"small" ) tar -xvf ${INPUT}/input_simsmall.tar;
		RUN_ARGS="-timing -threads ${NTHREADS}";;
	esac;;

	"ferret" ) APP_NAME=ferret
	PKG_PART=/apps/ferret
	INPUT=${BENCHMARK_DIR}${PKG_PART}/inputs
	OUTPUT=${BENCHMARK_DIR}${PKG_PART}/run
	case $2 in
		"test" )  tar -xvf ${INPUT}/input_test.tar -C ${INPUT}/;
		RUN_ARGS="${INPUT}/corel lsh ${INPUT}/queries 1 1 ${NTHREADS} ${OUTPUT}/output.txt";;
		"small" ) tar -xvf ${INPUT}/input_simsmall.tar -C ${INPUT}/;
		RUN_ARGS="${INPUT}/corel lsh ${INPUT}/queries 10 20 ${NTHREADS} ${OUTPUT}/output.txt";;
	esac;;

	"swaptions" ) APP_NAME=swaptions
	PKG_PART=/apps/swaptions
	case $2 in
		"test" ) RUN_ARGS="-ns 1 -sm 5 -nt ${NTHREADS}";;
		"small" ) RUN_ARGS="-ns 16 -sm 5000 -nt ${NTHREADS}";;
	esac;;

	"raytrace" ) APP_NAME=rtview
	PKG_PART=/apps/raytrace
	INPUT=${BENCHMARK_DIR}${PKG_PART}/inputs
	case $2 in
		"test" ) tar -xvf ${INPUT}/input_test.tar -C ${INPUT}/;
		RUN_ARGS="${INPUT}/octahedron.obj -nodisplay -automove -nthreads  ${NTHREADS} -frames 1 -res 1 1";;
		"small" ) tar -xvf ${INPUT}/input_simsmall.tar -C ${INPUT}/;
		RUN_ARGS="${INPUT}/happy_buddha.obj -nodisplay -automove -nthreads  ${NTHREADS} -frames 3 -res 480 270";;
	esac;;

	"fluidanimate" ) APP_NAME=fluidanimate
	PKG_PART=/apps/fluidanimate
	INPUT=${BENCHMARK_DIR}${PKG_PART}/inputs
	OUTPUT=${BENCHMARK_DIR}${PKG_PART}/run
	
	case $2 in
		"test" ) 
		echo "Running for test input";
		tar -xvf ${INPUT}/input_test.tar -C ${INPUT}/;
		RUN_ARGS="${NTHREADS} 1 ${INPUT}/in_5K.fluid ${OUTPUT}/out.fluid"
		;;
		"small" )
		echo "Running for small input"
		 tar -xvf ${INPUT}/input_simsmall.tar -C ${INPUT}/;
		RUN_ARGS="${NTHREADS} 5 ${INPUT}/in_35K.fluid ${OUTPUT}/out.fluid"
		;;
	esac;;

	"x264" ) APP_NAME=x264
	PKG_PART=/apps/x264
	INPUT=${BENCHMARK_DIR}${PKG_PART}/inputs
	OUTPUT=${BENCHMARK_DIR}${PKG_PART}/run
	case $2 in
		"test" ) tar -xvf ${INPUT}/input_test.tar -C ${INPUT}/;
		RUN_ARGS="--quiet --qp 20 --partitions b8x8,i4x4 --ref 5 --direct auto --b-pyramid --weightb --mixed-refs --no-fast-pskip --me umh --subme 7 --analyse b8x8,i4x4 --threads ${NTHREADS} -o eledream.264 ${INPUT}/eledream_32x18_1.y4m";;

		"small" ) tar -xvf ${INPUT}/input_simsmall.tar -C ${INPUT}/;
		RUN_ARGS="--quiet --qp 20 --partitions b8x8,i4x4 --ref 5 --direct auto --b-pyramid --weightb --mixed-refs --no-fastpskip --me umh --subme 7 --analyse b8x8,i4x4 --threads ${NTHREADS} -o eledream.264 ${INPUT}eledream_640x360_8.y4m";;
	esac;;

esac


mkdir ./data/${APP_NAME}
if [ -z "$CONF_NAME" ]
then
CONF_NAME="config"
fi

for (( i=1; i<=$NO_EXEC; i++ ))
do 
	echo "Execution no.$i "
 	mkdir ./data/${APP_NAME}/run$i       
	echo "${PIN_HOME}/pin -mt -t ./nova_modules.so -c ./conf/${CONF_NAME} -o ./data/${APP_NAME}/run$i/${APP_NAME}.out -- ${BENCHMARK_DIR}${PKG_PART}${BIN_DIR}${APP_NAME} ${RUN_ARGS}"
	${PIN_HOME}/pin -mt -t ./nova_modules.so -c ./conf/${CONF_NAME} -o ./data/${APP_NAME}/run$i/${APP_NAME}.out -- ${BENCHMARK_DIR}${PKG_PART}${BIN_DIR}${APP_NAME} ${RUN_ARGS} 
	echo "Trace Finished...Filtering..."
	if  [ $i -eq 1 ]
	then
	./filter ./conf/${CONF_NAME} ./data/${APP_NAME}/run$i/${APP_NAME}.out ./data/${APP_NAME}/run$i/${APP_NAME}.out.sym ./data/${APP_NAME}/run$i/${APP_NAME}.out.malloc ./data/${APP_NAME}/run$i/${APP_NAME}.out.free ./data/${APP_NAME}/run$i/${APP_NAME}.out.modules ./data/${APP_NAME}/run$i/${APP_NAME}.out.modules ./data/${APP_NAME}/run$i/${APP_NAME}.out.compact
	mv ./data/${APP_NAME}/run$i/$APP_NAME.out.modules ./data/${APP_NAME}/$APP_NAME.out.modules.ref
	else
	./filter ./conf/${CONF_NAME} ./data/${APP_NAME}/run$i/${APP_NAME}.out ./data/${APP_NAME}/run$i/${APP_NAME}.out.sym ./data/${APP_NAME}/run$i/${APP_NAME}.out.malloc ./data/${APP_NAME}/run$i/${APP_NAME}.out.free ./data/${APP_NAME}/run$i/${APP_NAME}.out.modules ./data/${APP_NAME}/${APP_NAME}.out.modules.ref ./data/${APP_NAME}/run$i/${APP_NAME}.out.compact
	fi
	echo "Analyzer start... "
	./analyzer ./conf/${CONF_NAME} ./data/${APP_NAME}/run$i/${APP_NAME}.out.compact ./data/${APP_NAME} $i 	
	echo "Analyzer finish..."
done  
  
echo Calculating interleaving change for ${NO_EXEC} executions of ${APP_NAME} ...
#./calc ./data/${APP_NAME} 1 2 ./conf/${CONF_NAME}
echo ./calc ./data/$APP_NAME 1 $NO_EXEC ./conf/${CONF_NAME}
./calc ./data/$APP_NAME 1 $NO_EXEC ./conf/${CONF_NAME}

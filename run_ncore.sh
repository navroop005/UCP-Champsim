#!/bin/bash

NUM_CORE=${1} 

if [ "$#" -lt `expr ${1} + 4` ] || [ "$#" -gt `expr ${1} + 5` ]; then

    echo "Illegal number of parameters"

    echo "Usage: ./run_ncore.sh [BINARY] [N_WARM] [N_SIM] [N_MIX] [TRACE0] [TRACE1] .... [TRACEN]"

    exit 1

fi

TRACE_DIR=$PWD/dpc3_traces

BINARY=${2}

N_WARM=${3}

N_SIM=${4}

N_MIX=${5}

if [ -z $TRACE_DIR ] || [ ! -d "$TRACE_DIR" ] ; then

    echo "[ERROR] Cannot find a trace directory: $TRACE_DIR"

    exit 1

fi



if [ ! -f "bin/$BINARY" ] ; then

    echo "[ERROR] Cannot find a ChampSim binary: bin/$BINARY"

    exit 1

fi



re='^[0-9]+$'

if ! [[ $N_WARM =~ $re ]] || [ -z $N_WARM ] ; then

    echo "[ERROR]: Number of warmup instructions is NOT a number" >&2;

    exit 1

fi



re='^[0-9]+$'

if ! [[ $N_SIM =~ $re ]] || [ -z $N_SIM ] ; then

    echo "[ERROR]: Number of simulation instructions is NOT a number" >&2;

    exit 1

fi

TRACES= 
shift
shift
shift
shift
shift

echo Number of cores: $NUM_CORE
echo Warmup instructions: $N_WARM M
echo Simulation instructions: $N_SIM M


while (($#))
 do
 TR=$1
 echo TRACE: $TR
 TRACES+=" $TRACE_DIR/$TR"
 if [ ! -f "$TRACE_DIR/$TR" ] ; then

    echo "[ERROR] Cannot find a trace1 file: $TRACE_DIR/$TR"

    exit 1

 fi
 shift
done

mkdir -p results_${NUM_CORE}core_${N_SIM}M

(./bin/${BINARY} -warmup_instructions ${N_WARM}000000 -simulation_instructions ${N_SIM}000000 -traces ${TRACES}) &> results_${NUM_CORE}core_${N_SIM}M/mix${N_MIX}-${BINARY}${OPTION}.txt

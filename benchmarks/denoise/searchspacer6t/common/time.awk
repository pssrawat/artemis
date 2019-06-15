#!/bin/bash
set -e

echo
echo "-------------------- PPCG Run ----------------------"
echo
htod=`grep "HtoD" ppcg-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
echo "Untiled MemCopy Time from Host to Device (ms) : " ${htod}

dtoh=`grep "DtoH" ppcg-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
echo "Untiled MemCopy Time from Host to Device (ms) : " ${dtoh}

time=`grep -E 'float|double' ppcg-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
echo "PPCG Run Time (ms) : " ${time}

echo
echo "-------------------- Untiled Run ----------------------"
echo
htod=`grep "HtoD" untiled-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
echo "Untiled MemCopy Time from Host to Device (ms) : " ${htod}

dtoh=`grep "DtoH" untiled-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
echo "Untiled MemCopy Time from Host to Device (ms) : " ${dtoh}

time=`grep -E 'float|double' untiled-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
echo "Untiled Run Time (ms) : " ${time}

echo
echo "-------------------- Forma Overtile Run ----------------------"
echo
htod=`grep "HtoD" forma-ot-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
echo "Untiled MemCopy Time from Host to Device (ms) : " ${htod}

dtoh=`grep "DtoH" forma-ot-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
echo "Untiled MemCopy Time from Host to Device (ms) : " ${dtoh}

time=`grep -E 'float|double' forma-ot-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
echo "Forma Overtile Run Time (ms) : " ${time}

echo
echo "-------------------- Overtile Run ----------------------"
echo
htod=`grep "HtoD" overtile-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
echo "Untiled MemCopy Time from Host to Device (ms) : " ${htod}

dtoh=`grep "DtoH" overtile-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
echo "Untiled MemCopy Time from Host to Device (ms) : " ${dtoh}

time=`grep -E 'float|double' overtile-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
echo "Overtile Run Time (ms) : " ${time}

echo
echo "-------------------- Opt Run ----------------------"
echo

htod=`grep "HtoD" opt-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
echo "Opt MemCopy Time from Host to Device (ms) : " ${htod}

dtoh=`grep "DtoH" opt-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
echo "Opt MemCopy Time from Host to Device (ms) : " ${dtoh}

time=`grep -E 'float|double' opt-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
echo "Opt Run Time (ms) : " ${time}

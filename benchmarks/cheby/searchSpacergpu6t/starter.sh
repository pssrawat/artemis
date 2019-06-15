#!/bin/bash -i
#PBS -l nodes=1:ppn=8:gpus=1
#PBS -l walltime=06:00:00
python -u searchSpacer.py cheby.idsl | tee a.log


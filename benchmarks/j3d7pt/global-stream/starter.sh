#!/bin/bash -i
#PBS -l nodes=1:ppn=8:gpus=1
#PBS -l walltime=06:00:00
python -u searchSpacer.py j3d7pt.idsl | tee a.log


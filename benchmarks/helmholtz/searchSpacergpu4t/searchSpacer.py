import itertools
from functools import reduce
import os
import re
import sys
#import dataset
import pickle
import subprocess
maxThreadsPerBlockLg2 = 11 # 2048
maxUnrollFactorLg2 = 4 # 16
dimensions = []

def compileAndGetResults(cfg):
    args = cfgToCommandLine(cfg).split(' ')


def FilterParams(spaceVector):
    _, _, blockDim, unrollDim = spaceVector
    """ can't stream and unroll in same dimension """
    streamDim = dimensions[0]
    if streamDim in [dimensions[i] for i in range(len(unrollDim)) if
      unrollDim[i] > 1]:
        return False
    if reduce(lambda x, y: x * y, unrollDim) > 4:
        return False
    # specific to j3d7pt
    # if not streamDim and reduce(lambda x, y: x * y, 
    #  itertools.chain(blockDim, unrollDim, [8])) > 0xc000:
    #    return False
    return True

def cfgToCommandLine(spaceVector):
    prefetch, retime, blockDim, unrollDim = spaceVector
    cmd = ''
    if blockDim:
        cmd += "--blockdim x={0},y={1}".format(*blockDim)
    if unrollDim:
        cmd += " --unroll " + ",".join(
          [x + '=' + str(y) for (x, y) in zip(dimensions, unrollDim)])
        #cmd += " --unroll " + ",".join(
        #[dimensions[i/2] if not i%2 else str(unrollDim[i/2])
        #  for i in range(2 * len(unrollDim))])
    cmd += " --full-stream {0}".format(dimensions[0])
    if prefetch:
        cmd += " --prefetch"
    if retime:
        cmd += " --retime"
    return cmd

def cfgToString(spaceVector):
    prefetch, retime, blockDim, unrollDim = spaceVector
    cmd = ''
    if blockDim:
        cmd += "bx{0}y{1}".format(*blockDim)
    if unrollDim:
        cmd += "u" + "".join(
          [x + str(y) for (x, y) in zip(dimensions, unrollDim)])
        #cmd += " --unroll " + ",".join(
        #[dimensions[i/2] if not i%2 else str(unrollDim[i/2])
        #  for i in range(2 * len(unrollDim))])
    if prefetch:
        cmd += "p"
    if retime:
        cmd += "r"
    return cmd

def searchSpace(dslFiles):
    assert(len(dslFiles) == 2)
    global dimensions
    with open(os.path.expanduser(dslFiles[1])) as dslFile:
        for line in dslFile:
            m = re.match(r"iterator (.*);", line)
            if m:
                dimensions = [i.strip() for i in m.group(1).split(',')]
                break
    # can't be 2: as the size would be lesser when considering distance -1 and so on
    # can't be 4: there is floating point error
    blockDim = itertools.product([2**i for i in range(5, maxThreadsPerBlockLg2)],
                                 repeat=len(dimensions) - 1)
    unrollDim = itertools.product([2**i for i in range(maxUnrollFactorLg2)],
     repeat=len(dimensions))
    for i in itertools.ifilter(FilterParams, itertools.product(
      [False, True], # prefetch?
      [False, True], # retime?
      itertools.ifilter(lambda (x, y): x * y <=
        2 ** (maxThreadsPerBlockLg2 - 1) and x * y >= 32,
        blockDim), # blockDim
      unrollDim, # unrollFactors
     )):
        config = cfgToCommandLine(i)
	cmd = " ".join(["bash run.sh", dslFiles[1], \
          config, "--ndim L=512,M=512,N=512"])
        output = subprocess.check_output(cmd, shell=True)
        print(config + ": " + output)
if __name__ == '__main__':
    searchSpace(sys.argv)

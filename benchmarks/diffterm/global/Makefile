ARCH=sm_60
ifdef DEBUG
OPTFLAGS=-O0 -g -maxrregcount=255 -ccbin=g++ -std=c++11 -Xcompiler "-fPIC -g -O0 -fno-strict-aliasing" --use_fast_math -Xptxas "-dlcm=ca"
else
OPTFLAGS=-O3 -maxrregcount=255 -ccbin=g++ -std=c++11 -Xcompiler "-fPIC -fopenmp -O3 -fno-strict-aliasing" --use_fast_math -Xptxas "-dlcm=ca"
endif
diffterm:
	nvcc diffterm.driver.cpp out.cu -arch=$(ARCH) $(OPTFLAGS)

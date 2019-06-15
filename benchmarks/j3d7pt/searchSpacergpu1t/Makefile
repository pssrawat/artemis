ARCH=sm_60
MAXREGCOUNT=128
ifdef DEBUG
OPTFLAGS=-O0 -g -maxrregcount=$(MAXREGCOUNT) -ccbin=g++ -std=c++11 -Xcompiler "-fPIC -g -O0 -fno-strict-aliasing" --use_fast_math -Xptxas "-dlcm=cg"
else
OPTFLAGS=-O3 -maxrregcount=$(MAXREGCOUNT) -ccbin=g++ -std=c++11 -Xcompiler "-fPIC -fopenmp -O3 -fno-strict-aliasing" --use_fast_math -Xptxas "-dlcm=cg"
endif
.DEFAULT_GOAL := j3d7pt
gold:
	nvcc j3d7pt.driver.cpp out.cu -arch=$(ARCH) $(OPTFLAGS)
	./a.out
clean:
	rm test.txt gold.txt
j3d7pt:
	nvcc j3d7pt.baked.cpp out.cu -arch=$(ARCH) $(OPTFLAGS)

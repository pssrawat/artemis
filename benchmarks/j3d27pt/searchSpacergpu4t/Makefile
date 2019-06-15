ARCH=sm_60
MAXREGCOUNT=128
ifdef DEBUG
OPTFLAGS=-O0 -g -maxrregcount=$(MAXREGCOUNT) -ccbin=g++ -std=c++11 -Xcompiler "-fPIC -g -O0 -fno-strict-aliasing" --use_fast_math -Xptxas "-dlcm=cg"
else
OPTFLAGS=-O3 -maxrregcount=$(MAXREGCOUNT) -ccbin=g++ -std=c++11 -Xcompiler "-fPIC -fopenmp -O3 -fno-strict-aliasing" --use_fast_math -Xptxas "-dlcm=cg"
endif
.DEFAULT_GOAL := j3d27pt
gold:
	nvcc j3d27pt.driver.cpp out.cu -arch=$(ARCH) $(OPTFLAGS)
	./a.out
clean:
	rm test.txt gold.txt
j3d27pt:
	nvcc j3d27pt.baked.cpp out.cu -arch=$(ARCH) $(OPTFLAGS)

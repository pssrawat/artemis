ARCH=sm_60
MAXREGCOUNT=128
ifdef DEBUG
OPTFLAGS=-O0 -g -maxrregcount=$(MAXREGCOUNT) -ccbin=g++ -std=c++11 -Xcompiler "-fPIC -g -O0 -fno-strict-aliasing" --use_fast_math -Xptxas "-dlcm=ca"
else
OPTFLAGS=-O3 -maxrregcount=$(MAXREGCOUNT) -ccbin=g++ -std=c++11 -Xcompiler "-fPIC -fopenmp -O3 -fno-strict-aliasing" --use_fast_math -Xptxas "-dlcm=ca"
endif
.DEFAULT_GOAL := sw4
gold:
	nvcc sw4.driver.cpp out.cu -arch=$(ARCH) $(OPTFLAGS)
	./a.out
clean:
	rm test.txt gold.txt
sw4:
	nvcc sw4.driver.cpp out.cu -arch=$(ARCH) $(OPTFLAGS)

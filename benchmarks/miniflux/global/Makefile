ARCH=sm_60
MAXREGCOUNT=255
ifdef DEBUG
OPTFLAGS=-O0 -g -maxrregcount=$(MAXREGCOUNT) -ccbin=g++ -std=c++11 -Xcompiler "-fPIC -g -O0 -fno-strict-aliasing" --use_fast_math -Xptxas "-dlcm=ca -v"
else
OPTFLAGS=-O3 -maxrregcount=$(MAXREGCOUNT) -ccbin=g++ -std=c++11 -Xcompiler "-fPIC -fopenmp -O3 -fno-strict-aliasing" --use_fast_math -Xptxas "-dlcm=ca -v"
endif
.DEFAULT_GOAL := miniflux
gold:
	nvcc miniflux.driver.cpp out.cu -arch=$(ARCH) $(OPTFLAGS)
	./a.out
clean:
	rm test.txt gold.txt
miniflux:
	nvcc miniflux.driver.cpp out.cu -arch=$(ARCH) $(OPTFLAGS)

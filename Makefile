EXE = gppKerOpenMPFor2.ex
SRC1 = gppKerOpenMPFor.cpp 

#CXX = xlc++
#CXX = g++
CXX = CC

LINK = ${CXX}

#CXXFLAGS=-O3 -qopenmp -std=c++11 -qopt-report=5
#CXXFLAGS+=-xCORE_AVX2
##CXXFLAGS+=-xMIC_AVX512
#LINKFLAGS=-qopenmp -std=c++11

ifeq ($(CXX),CC)
	CXXFLAGS=-O3 -qopenmp -qopt-report=5 -std=c++11
	#CXXFLAGS+=-xCORE_AVX2
	CXXFLAGS+=-xMIC_AVX512
	LINKFLAGS=-qopenmp
endif 

ifeq ($(CXX),g++)
	CXXFLAGS= -g -O3 -std=c++11 -fopenmp -foffload="-lm" -foffload=nvptx-none
	LINKFLAGS=-fopenmp
endif 

ifeq ($(CXX),xlc++)
	CXXFLAGS=-O3 -std=gnu++11 -g -qsmp=noauto:omp -qoffload #-Xptxas -v
	LINKFLAGS=-qsmp=noauto:omp -qoffload 
endif 

ifeq ($(CXX),clang++)
	CXXFLAGS=-O3 -std=gnu++11 -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda --cuda-path=${CUDA_HOME}
	LINKFLAGS=-fopenmp -fopenmp-targets=nvptx64-nvidia-cuda --cuda-path=${CUDA_HOME}
endif 

OBJ1 = $(SRC1:.cpp=.o)

$(EXE): $(OBJ1)  
	$(CXX) $(OBJ1) -o $(EXE) $(LINKFLAGS)

$(OBJ1): $(SRC1) 
	$(CXX) -c $(SRC1) $(CXXFLAGS)

clean: 
	rm -f $(OBJ1) $(EXE)


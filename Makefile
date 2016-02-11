#  Copyright 2018 Oxford Nanopore Technologies, Ltd

#  This Source Code Form is subject to the terms of the Oxford Nanopore
#  Technologies, Ltd. Public License, v. 1.0. If a copy of the License 
#  was not  distributed with this file, You can obtain one at
#  http://nanoporetech.com

buildDir ?= build
hdf5Root ?= ''
releaseType ?= Debug 

.PHONY: all
all: flappie
flappie: ${buildDir}/flappie
	cp ${buildDir}/flappie flappie

${buildDir}:
	mkdir ${buildDir}

.PHONY: test
test: ${buildDir}
	cd ${buildDir} && \
	cmake .. -DCMAKE_CXX_FLAGS="-std=c++11 -Wno-format  -DENABLE_DMA=1 -g -fpermissive -Wfatal-errors -mfloat-abi=hard -mfpu=neon  -DUSE_SSE2  -D__USE_MISC -D_POSIX_SOURCE -DNDEBUG" -DCMAKE_BUILD_TYPE=${releaseType} -DHDF5_ROOT=${hdf5Root} && \
	make testf
	cp ${buildDir}/testf testf

.PHONY: clean
clean:
	rm -rf ${buildDir} flappie

${buildDir}/flappie: ${buildDir}
	cd ${buildDir} && \
	cmake .. -DCMAKE_CXX_FLAGS="-pg -std=c++11 -Wno-format  -DENABLE_DMA=1 -g -fpermissive -Wfatal-errors -mfloat-abi=hard -mfpu=neon  -DUSE_SSE2  -D__USE_MISC -D_POSIX_SOURCE -DNDEBUG" -DCMAKE_BUILD_TYPE=${releaseType} -DHDF5_ROOT=${hdf5Root} && \
	make flappie
    

#include /home/mejbah/pintool/pin-2.10-45467-gcc.3.4.6-ia32_intel64-linux/source/tools/makefile.gnu.config
include makefile.gnu.config
LINKER =${CXX}
CXXFLAGS = -I$(PIN_HOME)/InstLib -fomit-frame-pointer -Wall -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
LINK_PTHREAD=-lpthread
OPT =${COPT} -DPRINT_LOCK -g -DPRINT_FREE
# This flag ensures lock, unlock and free statements to be printed in the
# profile files

NOVA_MODULES_OBJS = nova_modules.o read_symbols.o modules.o
ANALYZER_OBJS = analyzer.o instruction.o atomicsection.o 
FILTER_OBJS = filter.o instruction.o addressmap.o module_map.o
CALC_OBJS = calc.o
ALL_BINARIES = analyzer filter nova_modules.so calc
all: $(ALL_BINARIES)

nova_modules.o : nova_modules.cpp read_symbols.h modules.h
	${CXX} ${OPT} $(CXXFLAGS) ${PIN_CXXFLAGS} ${OUTOPT}$@ $< 

read_symbols.o : read_symbols.cpp read_symbols.h
	${CXX} ${OPT} $(CXXFLAGS) ${PIN_CXXFLAGS} ${OUTOPT}$@ $< 

modules.o : modules.cpp modules.h
	${CXX} ${OPT} $(CXXFLAGS) ${PIN_CXXFLAGS} ${OUTOPT}$@ $< 


nova_modules.so : $(NOVA_MODULES_OBJS)
	${LINKER} ${PIN_LDFLAGS} $(LINK_DEBUG) ${LINK_OUT}$@ ${NOVA_MODULES_OBJS} ${PIN_LPATHS} ${PIN_LIBS} $(DBG) ${LINK_LIBSTDC}

instruction.o: instruction.cpp instruction.h
	${CXX} -g ${OPT} ${OUTOPT}$@ $< 

analyzer.o: analyzer.cpp instruction.h atomicsection.h meta_data.h
	${CXX} -g ${OPT} $(CXXFLAGS) ${OUTOPT}$@ $< 

atomicsection.o: atomicsection.cpp atomicsection.h instruction.h
	${CXX} -g ${OPT} ${OUTOPT}$@ $< 

analyzer: $(ANALYZER_OBJS)
	${LINKER} ${OUTOPT}$@ ${ANALYZER_OBJS} ${OPENCV_LIBS}  

addressmap.o: addressmap.cpp addressmap.h instruction.h
	${CXX} ${OPT} ${OUTOPT}$@ $< 

filter.o: filter.cpp instruction.h addressmap.h module_map.h
	${CXX} ${OPT} ${OUTOPT}$@ $< 

module_map.o: module_map.cpp module_map.h
	${CXX} ${OPT} ${OUTOPT}$@ $< 

filter: $(FILTER_OBJS)
	${LINKER} ${OUTOPT}$@ ${FILTER_OBJS} 

calc.o: calc.cpp
	${CXX} ${OPT} ${OUTOPT}$@ $< 

calc: $(CALC_OBJS)
	${LINKER} ${OUTOPT}$@ ${CALC_OBJS}	
## cleaning
clean:
	rm -rf *.d *.so *.o ${ALL_BINARIES}

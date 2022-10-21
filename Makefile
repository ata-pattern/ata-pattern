CC = g++
CFLAGS = -Isrc -Isrc/full_classes -O3 -Wall -std=c++11
rm = @rm
mkdir = @mkdir
exe = mapper
OBJs = objs/CostFunc.o \
		objs/Expander.o \
		objs/Filter.o \
		objs/NodeMod.o \
		objs/myParser.o \
		objs/Latency.o \
		objs/Queue.o \
		objs/Node.o
HPPs = src/CostFunc.hpp \
		src/Expander.hpp \
		src/Filter.hpp \
		src/NodeMod.hpp \
		src/Latency.hpp \
		src/Queue.hpp \
		src/CostFunc/Meta.hpp \
		src/Expander/Meta.hpp \
		src/Filter/Meta.hpp \
		src/NodeMod/Meta.hpp \
		src/Latency/Meta.hpp \
		src/Queue/Meta.hpp \
		src/full_classes/Environment.hpp \
		src/full_classes/GateNode.hpp \
		src/full_classes/LinkedStack.hpp \
		src/full_classes/Node.hpp \
		src/full_classes/ScheduledGate.hpp \
		src/full_classes/myParser.hpp

ifeq ($(OS),Windows_NT)
	rm = @del /F /Q
	CFLAGS += -D WINDOWS
	exe = mapper.exe
else
	mkdir += -p 
	rm += -f -r 
	UNAME_S := $(shell uname -s)
	UNAME_P := $(shell uname -p)

	ifeq ($(UNAME_S),Linux)
		CFLAGS += -D LINUX
		ifeq ($(UNAME_P),x86_64)
			CFLAGS += -D AMD64
			CFLAGS += -D LINUX64
		endif
	else ifeq ($(UNAME_P),x86_64)
		CFLAGS += -D AMD64
	endif
endif

default: objs ${exe}
default: objs
default: objs 

debug: CFLAGS += -g -O0
debug: default
prof: CFLAGS += -pg -fno-inline
prof: default


mapper: src/main.cpp ${OBJs} ${HPPs}
	${CC} ${CFLAGS} ${OBJs} $< -o $@

mapper.exe: src/main.cpp ${OBJs} ${HPPs}
	${CC} ${CFLAGS} ${OBJs} $< -o $@

lnnclique: src/lnnclique.cpp
	${CC} ${CFLAGS} ${OBJs} $< -o $@

lnnclique.exe: src/lnnclique.cpp
	${CC} ${CFLAGS} ${OBJs} $< -o $@

2xnclique: src/2xnclique.cpp
	${CC} ${CFLAGS} ${OBJs} $< -o $@

2xnclique.exe: src/2xnclique.cpp
	${CC} ${CFLAGS} ${OBJs} $< -o $@

objs:
	${mkdir} objs

objs/CostFunc.o:  src/CostFunc.hpp src/full_classes/Node.hpp
	${CC} ${CFLAGS} -c src/CostFunc/Meta.cpp -o $@

objs/Expander.o: src/Expander.hpp src/full_classes/Node.hpp
	${CC} ${CFLAGS} -c src/Expander/Meta.cpp -o $@

objs/Filter.o:  src/Filter.hpp src/full_classes/Node.hpp
	${CC} ${CFLAGS} -c src/Filter/Meta.cpp -o $@

objs/NodeMod.o:  src/NodeMod.hpp src/full_classes/Node.hpp
	${CC} ${CFLAGS} -c src/NodeMod/Meta.cpp -o $@

objs/Latency.o:  src/Latency.hpp
	${CC} ${CFLAGS} -c src/Latency/Meta.cpp -o $@

objs/Queue.o:  src/Queue.hpp src/full_classes/Node.hpp
	${CC} ${CFLAGS} -c src/Queue/Meta.cpp -o $@

objs/Node.o: src/full_classes/Node.cpp
	${CC} ${CFLAGS} -c $< -o $@

objs/myParser.o: src/full_classes/myParser.cpp src/full_classes/myParser.hpp src/full_classes/Environment.hpp
	${CC} ${CFLAGS} -c $< -o $@


clean:
	${rm} mapper mapper.exe lnnclique lnnclique.exe 2xnclique 2xnclique.exe objs

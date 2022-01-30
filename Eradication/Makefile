


CC = mpicxx
#CC = g++

# NOTE: Because I fail at linux development, there is an absolute path to the boost libraries installation wired into the executable's 
# library search path. 
# This avoids the use of LD_LIBRARY_FLAGS when libboost* tries to load its SOs. Statically linking boost appeared to be 
# nontrivial and so was not thoroughly explored.
#
BOOST_PATH=/home/dmacdonald/EMOD/boost/boost_1_43_0
#BOOST_PATH=/home/jbloedow/EMOD/boost/boost_1_43_0
BOOST_INCLUDES=$(BOOST_PATH)
BOOST_LIBS=$(BOOST_PATH)/stage/lib
#BOOST_LIBS=
UNIT_TEST_INCLUDES=unittest/UnitTest++/src/
#MPI_INCLUDE=/usr/include/openmpi-i386/
MPI_INCLUDE=
ROOT = $(PWD)/..
INCS=-I $(UNIT_TEST_INCLUDES) -I$(ROOT) -I$(ROOT)/Eradication -I$(ROOT)/utils/ -I$(ROOT)/cajun/include -I$(ROOT)/campaign -I$(ROOT)/jsonspirit -I$(ROOT)/unittest/UnitTest++/src -I$(BOOST_INCLUDES) -I$(ROOT)/libgeneric_static/ -I$(ROOT)/rapidjson/include
#CFLAGS = -Wall -g -fpermissive -I $(BOOST_INCLUDES) -I $(UNIT_TEST_INCLUDES) -I $(MPI_INCLUDE) -ffloat-store  -Wno-unknown-pragmas

CFLAGS = $(INCS) -w -O3 -fpermissive -ffloat-store  -std=c++14 -Wno-unknown-pragmas -save-temps -DNULL=0 -std=c++14 -DDONT_USE_BOOST_MPI
CFLAGS_DBG = $(INCS) -w -g -fpermissive -ffloat-store  -std=c++14 -Wno-unknown-pragmas -DNULL=0 -DDONT_USE_BOOST_MPI -save-temps
COMPILE = $(CC) $(CFLAGS) -c -D_new_=new -DENABLE_POLIO
DBG_COMPILE = $(CC) $(CFLAGS_DBG) -c -D_new_=new -DENABLE_POLIO
LDFLAGS=-L/usr/local/lib -L/usr/local/lib -L/usr/lib/mpich2/lib/ -L /usr/lib/mpich2/lib/ -lmpichcxx -L$(BOOST_LIBS) -Wl,-R$(BOOST_LIBS) -L libs/ -lboost_program_options -lboost_filesystem -lboost_system -ldl
SOURCES=$(wildcard *.cpp) $(wildcard cajun/src/*.cpp) $(wildcard UnitTest++/src/*.cpp) $(wildcard UnitTest++/src/Posix/*.cpp) $(wildcard json_spirit/*.cpp) $(wildcard utils/*.cpp) $(wildcard campaign/*.cpp)
#SOURCES=$(wildcard *.cpp) $(wildcard cajun/src/*.cpp) $(wildcard UnitTest++/src/*.cpp) $(wildcard UnitTest++/src/Posix/*.cpp)
OBJFILES := $(patsubst %.cpp,%.o,$(SOURCES))
OBJFILES.dbg := $(patsubst %.cpp,obj/dbg/%.o,$(SOURCES))
OBJFILES.rls := $(patsubst %.cpp,obj/rls/%.o,$(SOURCES))
EXECUTABLE=Eradication


createlinks:
	./makelinks

all: $(OBJFILES) $(EXECUTABLE)

clean:
	rm $(OBJFILES)

clean.rls:
	rm $(OBJFILES.rls)
	mkdir UNIT_TESTS/
	mv Test*.cpp UNIT_TESTS/

clean.dbg:
	rm $(OBJFILES.dbg)
	mkdir UNIT_TESTS/
	mv Test*.cpp UNIT_TESTS/

$(EXECUTABLE): $(OBJFILES)
	$(CC) $(LDFLAGS) -o $(EXECUTABLE) $(OBJFILES)

bin/dbg/$(EXECUTABLE): $(OBJFILES.dbg)
	mkdir -p bin/dbg

bin/rls/$(EXECUTABLE): $(OBJFILES.rls)
	mkdir -p bin/rls

debug: bin/dbg/$(EXECUTABLE)
	mkdir -p bin/dbg
	$(CC) $(LDFLAGS) -o bin/dbg/$(EXECUTABLE) $(OBJFILES.dbg) $(ROOT)/cajun/obj/dbg/src/*.o $(ROOT)/jsonspirit/obj/dbg/*.o $(ROOT)/utils/obj/dbg/*.o $(ROOT)/campaign/obj/dbg/*.o

release: bin/rls/$(EXECUTABLE)
	mkdir -p bin/rls
	$(CC) $(LDFLAGS) -o bin/rls/$(EXECUTABLE) $(OBJFILES.rls) $(ROOT)/cajun/obj/rls/src/*.o $(ROOT)/jsonspirit/obj/rls/*.o $(ROOT)/utils/obj/rls/*.o $(ROOT)/campaign/obj/rls/*.o

%.o: %.cpp
	$(COMPILE) -o $@ $<

obj/dbg/%.o: %.cpp
	mkdir -p obj/dbg/
	$(DBG_COMPILE) -o $@ $<

obj/rls/%.o: %.cpp
	mkdir -p obj/rls/
	$(COMPILE) -o $@ $<


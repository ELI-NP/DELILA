COMP_NAME = Recorder

all: $(COMP_NAME)Comp

SRCS += $(COMP_NAME).cpp
SRCS += $(COMP_NAME)Comp.cpp

LDFLAGS += $(shell root-config --glibs) -lRHTTP -O2 -fopenmp
CPPFLAGS += -I$(shell root-config --incdir) -pthread -O2 -fopenmp 

# sample install target
#
# MODE = 0755
# BINDIR = /tmp/mybinary
#
# install: $(COMP_NAME)Comp
#	mkdir -p $(BINDIR)
#	install -m $(MODE) $(COMP_NAME)Comp $(BINDIR)

include /usr/share/daqmw/mk/comp.mk

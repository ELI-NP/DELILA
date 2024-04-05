COMP_NAME = Pulser

all: $(COMP_NAME)Comp

SRCS += $(COMP_NAME).cpp
SRCS += $(COMP_NAME)Comp.cpp
SRCS += ../src/TDataContainer.cpp

LDLIBS += -lCAENDigitizer
CPPFLAGS += -I../../TDigiTES/include -I../include
CPPFLAGS += -O2

LDFLAGS += $(shell root-config --glibs)
CPPFLAGS += -I$(shell root-config --incdir)

# sample install target
#
# MODE = 0755
# BINDIR = /tmp/mybinary
#
# install: $(COMP_NAME)Comp
#	mkdir -p $(BINDIR)
#	install -m $(MODE) $(COMP_NAME)Comp $(BINDIR)

include /usr/share/daqmw/mk/comp.mk

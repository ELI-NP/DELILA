COMP_NAME = Emulator

all: $(COMP_NAME)Comp

SRCS += $(COMP_NAME).cpp
SRCS += $(COMP_NAME)Comp.cpp
SRCS += ../src/TDataContainer.cpp

LDLIBS += -lCAENDigitizer
CPPFLAGS += -I../../TDigiTES/include -I../include -O2

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

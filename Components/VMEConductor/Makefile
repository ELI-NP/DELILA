COMP_NAME = VMEConductor

all: $(COMP_NAME)Comp

SRCS += $(COMP_NAME).cpp
SRCS += $(COMP_NAME)Comp.cpp

SRCS += ../src/TDataContainer.cpp
#SRCS += ../src/VMEController.cpp
#SRCS += ../src/VMEDevice.cpp


LDLIBS += -ldl

CPPFLAGS += -O2 -I../../TDigiTES/include -I../include

# sample install target
#
# MODE = 0755
# BINDIR = /tmp/mybinary
#
# install: $(COMP_NAME)Comp
#	mkdir -p $(BINDIR)
#	install -m $(MODE) $(COMP_NAME)Comp $(BINDIR)

include /usr/share/daqmw/mk/comp.mk

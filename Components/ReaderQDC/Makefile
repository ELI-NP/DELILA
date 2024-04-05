COMP_NAME = ReaderQDC

all: $(COMP_NAME)Comp

SRCS += $(COMP_NAME).cpp
SRCS += $(COMP_NAME)Comp.cpp
SRCS += ../../TDigiTES/src/BoardUtils.cpp
SRCS += ../../TDigiTES/src/Configure.cpp
SRCS += ../../TDigiTES/src/ParamParser.cpp
SRCS += ../../TDigiTES/src/TDigiTes.cpp
SRCS += ../../TDigiTES/src/TPHA.cpp
SRCS += ../../TDigiTES/src/TPSD.cpp
SRCS += ../../TDigiTES/src/TQDC.cpp
SRCS += ../src/TDataContainer.cpp

LDLIBS += -lCAENDigitizer
CPPFLAGS += -O2 -std=c++14 -I../../TDigiTES/include -I../include

# sample install target
#
# MODE = 0755
# BINDIR = /tmp/mybinary
#
# install: $(COMP_NAME)Comp
#	mkdir -p $(BINDIR)
#	install -m $(MODE) $(COMP_NAME)Comp $(BINDIR)

include /usr/share/daqmw/mk/comp.mk

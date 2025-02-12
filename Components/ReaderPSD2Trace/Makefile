COMP_NAME = ReaderPSD2Trace

all: $(COMP_NAME)Comp

SRCS += $(COMP_NAME).cpp
SRCS += $(COMP_NAME)Comp.cpp
SRCS += PSD2.cpp

LDLIBS += -lCAEN_FELib -lpthread
CPPFLAGS += -O2 -std=c++17 -pthread -I../includ
LDFLAGS += -pthread

# sample install target
#
# MODE = 0755
# BINDIR = /tmp/mybinary
#
# install: $(COMP_NAME)Comp
#	mkdir -p $(BINDIR)
#	install -m $(MODE) $(COMP_NAME)Comp $(BINDIR)

include /usr/share/daqmw/mk/comp.mk


########################################################################
#   Preamble.
########################################################################

ifeq ($(OPTFLAGS),)
	OPTFLAGS = -g
endif

CXX = g++
#CPPFLAGS = -I/boost
CXXFLAGS = -Wall
LDFLAGS = $(OPTFLAGS)
#LDLIBS = -lm

#CXXFLAGS += $(OPTFLAGS)

#   GNU make's default rule uses $(CC) for linking
#LINK.o = $(CXX) $(LDFLAGS) $(TARGET_ARCH)


########################################################################
#   Rules.
########################################################################

all : sender receiver send recv

clean:
	rm -f sender receiver send recv

#sender.o : sender

########################################################################
#   
########################################################################



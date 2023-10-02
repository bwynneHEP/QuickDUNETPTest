SHELL = /bin/sh
UNAME = $(shell uname)

#################
# Variables
ROOTCFLAGS   = #-L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --cflags)
ROOTLIBS     = #-L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --libs)

CXX          = g++
RM           = rm -f

CXXFLAGS     = -O0 -g -fPIC -funroll-loops -DHELLO_BEN -Wall -std=c++17

EXENAME		= TPTest
SRCEXT   	= cpp
SRCDIR  	= src
INCDIR   	= include
OBJDIR   	= build
EXEDIR  	= bin

SRCS    	:= $(shell find $(SRCDIR) -name '*.$(SRCEXT)')
OBJS    	:= $(patsubst $(SRCDIR)/%.$(SRCEXT),$(OBJDIR)/%.o,$(SRCS))

GARBAGE  = $(OBJDIR)/*.o $(EXEDIR)/$(EXENAME)

#################
# Dependencies
# Linux
ifeq "$(UNAME)" "Linux"
RANLIB       = ranlib
CXXFLAGS    += -I$(INCDIR) $(ROOTCFLAGS)
LINKFLAGS    = -g $(shell root-config --nonew) $(shell root-config --ldflags) -Wl,--no-as-needed
endif

# OS X
ifeq "$(UNAME)" "Darwin"
RANLIB       = ranlib
CXXFLAGS    += -I$(INCDIR) $(ROOTCFLAGS)
LINKFLAGS    =
endif

#################
# Libraries
LIBS       += $(ROOTLIBS) #-lHtml -lThread

#################
# Targets
all : $(EXEDIR)/$(EXENAME)

$(EXEDIR)/$(EXENAME) : $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LINKFLAGS) $(LIBS)

$(OBJDIR)/%.o : $(SRCDIR)/%.$(SRCEXT)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean :
	$(RM) $(GARBAGE)

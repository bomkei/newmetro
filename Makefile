TARGET	= metro

CC		= clang
CXX		= clang++
LD		= $(CXX)

BINDIR	= /usr/local/bin

TOPDIR	?= $(CURDIR)
BUILD		= build
INCLUDE	= include
SOURCES	= src

OPTFLAGS		= -O0 -g
WARNFLAGS		= -Wall -Wextra -Wno-switch
DBGFLAGS		= -DMETRO_DEBUG
COMMONFLAGS	= $(DBGFLAGS) $(INCLUDES) $(OPTFLAGS) $(WARNFLAGS)
CFLAGS			= $(COMMONFLAGS)
CXXFLAGS		= $(CFLAGS) -std=c++20
LDFLAGS			=

%.o: %.c
	@echo $(notdir $<)
	@$(CC) $(CFLAGS) -MP -MMD -MF $*.d -c -o $@ $<

%.o: %.cc
	@echo $(notdir $<)
	@$(CXX) $(CXXFLAGS) -MP -MMD -MF $*.d -c -o $@ $<

ifneq ($(notdir $(CURDIR)),$(BUILD))

export OUTPUT		= $(TOPDIR)/$(TARGET)
export VPATH		= $(foreach dir,$(SOURCES),$(TOPDIR)/$(dir))
export INCLUDES	= $(foreach dir,$(INCLUDE),-I$(TOPDIR)/$(dir))

CFILES			= $(notdir $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c)))
CXXFILES		= $(notdir $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cc)))

export OFILES		= $(CFILES:.c=.o) $(CXXFILES:.cc=.o)

.PHONY: $(BUILD) all release clean re install

all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

release: $(BUILD)
	@$(MAKE) --no-print-directory OPTFLAGS="-O3" DBGFLAGS="" LDFLAGS="-Wl,--gc-sections" -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@[ -d $@ ] || mkdir -p $@

clean:
	rm -rf $(BUILD) $(TARGET)

re: clean all

install: all
	@echo install...
	@install $(notdir $(OUTPUT)) $(BINDIR)/$(TARGET)

else

DEPENDS	= $(OFILES:.o=.d)

$(OUTPUT): $(OFILES)
	@echo linking...
	@$(LD) $(LDFLAGS) -pthread -o $@ $^

-include $(DEPENDS)

endif
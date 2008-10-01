# The executable name is suffix depending on the target
OUT = netview
HOST_SUFFIX = _host
TARGET_SUFFIX = _target
TARGETSIM_SUFFIX = _sim_target

# this includes the framework configuration
-include .config

# decide whether we are building or dooing something other like cleaning or configuring
ifeq '' '$(filter $(MAKECMDGOALS), clean distclean config)'
# check whether a .config file has been found
ifeq '' '$(filter .config, $(MAKEFILE_LIST))'
$(error "Cannot make the target '$(MAKECMDGOALS)' without configuring the application. Please run make config to do this.")
endif
endif

# Compile options
PEDANTIC = # -pedantic

# Host-Compiler executables and flags
HOST_CC = gcc 
HOST_CFLAGS = $(HOST_FEATURES) -Wall $(PEDANTIC) -D OSC_HOST
HOST_LDFLAGS = -lm

# Cross-Compiler executables and flags
TARGET_CC = bfin-uclinux-gcc 
TARGET_CFLAGS = -Wall $(PEDANTIC) -O2 -D OSC_TARGET
TARGET_LDFLAGS = -Wl,-elf2flt="-s 1048576" -lbfdsp

# Source files of the application
SOURCES = main.c

# Default target
all: target

# this target ensures that the application has been built prior to deployment
$(OUT)_%:
	@ echo "Please use make {target,targetdbg,targetsim} to build the application first"
	@ exit 1

# Compiles the executable
target: $(SOURCES) inc/*.h lib/libosc_target.a
	$(TARGET_CC) $(SOURCES) lib/libosc_target.a $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -o $(OUT)$(TARGET_SUFFIX)
	! [ -d /tftpboot ] || cp $(OUT)$(TARGET_SUFFIX) /tftpboot/$(OUT)

# Target to explicitly start the configuration process
.PHONY: config
config:
	@ ./configure
	@ $(MAKE) --no-print-directory get

# Set symlinks to the framework
.PHONY: get
get:
	@ rm -rf inc lib
	@ ln -s $(CONFIG_FRAMEWORK)/staging/inc ./inc
	@ ln -s $(CONFIG_FRAMEWORK)/staging/lib ./lib
	@ echo "Configured Oscar framework."

# deploying to the device
.PHONY: deploy
deploy: $(OUT)$(TARGET_SUFFIX)
	rcp -rp runapp.sh root@$(CONFIG_TARGET_IP):/app/
	rcp -rp $(OUT)$(TARGET_SUFFIX) root@$(CONFIG_TARGET_IP):/app/$(OUT)
	@ echo "Application deployed."

# Cleanup
.PHONY: clean
clean:	
	rm -f $(OUT)$(HOST_SUFFIX) $(OUT)$(TARGET_SUFFIX) $(OUT)$(TARGETSIM_SUFFIX)
	rm -rf doc/{html,latex,index.html}
	rm -f *.o *.gdb
	@ echo "Directory cleaned"

# Cleans everything not intended for source distribution
.PHONY: distclean
distclean: clean
	rm -f .config
	rm -rf inc lib

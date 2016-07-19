
APP = jdaSkip
OBJS = jdaSkip.o settings.o skipmenu.o keys.o playback.o

BASE = $(shell cd ..; pwd)
CUR_DIR = $(shell pwd)

# include default settings
include ${BASE}/include/tool.mk

# output object directory
OBJ_DIR = ${CUR_DIR}/obj
BIN_DIR = ${CUR_DIR}/bin
SRC_DIR = ${CUR_DIR}

TAP_FLAGS += -fPIC

ifeq (${DEBUG},y)
TAP_FLAGS += -g -DDEBUG 
endif

ifeq (${__7100PLUS__},1)
TAP_FLAGS += -D__7100PLUS__ 
APP = jdaSkip_7100
endif

ifeq (${__7260__},1)
TAP_FLAGS += -D__7260__ 
APP = jdaSkip_7260
endif

#compile options
CFLAGS += -DLINUX  -MD -W -Wall -O2 -mips32 
CPPFLAGS += -DLINUX  -MD -W -Wall  -O2 -mips32 

#include directories.
INCLUDE_DIRS = ${BASE}/include

TAP_INCLUDE_DIRS = $(addprefix -I, $(INCLUDE_DIRS))
TAP_FLAGS += $(TAP_INCLUDE_DIRS)

TAP_OBJS = $(addprefix $(OBJ_DIR)/, ${OBJS}) 

TAP_LIBS =  ${BASE}/tapinit.o ${BASE}/libtap.so -lFireBird -ldl -lc

TAP_APP = $(BIN_DIR)/$(APP).tap
TAP_MAP = $(OBJ_DIR)/$(APP).map

all: $(OBJ_DIR) $(BIN_DIR) $(TAP_APP)

$(OBJ_DIR) $(BIN_DIR):
	@echo "[Making directory... $@]"
	${Q_}$(MKDIR) "$@"

$(TAP_APP): ${TAP_OBJS}
	@echo "[Linking... $@]"
	$(Q_)$(LD) -shared --no-undefined --allow-shlib-undefined  -o $@ ${TAP_OBJS} $(TAP_LIBS) -Map ${TAP_MAP}
	$(Q_)$(JB) $(TAP_APP)

clean:
	@echo "[Clean all objs...]"
	$(Q_)-${RM} $(TAP_APP) $(OBJ_DIR)/*.d $(OBJ_DIR)/*.o


# Implicit rule for building local apps
%.o : %.c
	@echo [Compile... $<]
	${Q_}$(CC) $(CFLAGS) $(TAP_FLAGS) -c $< -o $@

%.o : %.cpp
	@echo "[Compile... $<]"
	${Q_}$(CXX)  $(CPPFLAGS) $(TAP_FLAGS) -c $< -o $@

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@echo [Compile... $<]
	${Q_}$(CC) $(CFLAGS) $(TAP_FLAGS) -c $< -o $@

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.cpp
	@echo "[Compile... $<]"
	${Q_}$(CXX)  $(CPPFLAGS) $(TAP_FLAGS) -c $< -o $@
	

# Dependency file checking (created with gcc -M command)
-include $(OBJ_DIR)/*.d

	

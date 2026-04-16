

TARGET = libxFuzztrace.so

CC = clang
LD = $(CC)

AFLPP = ../../AFLplusplus

SOURCE_PATH  = source
INCLUDE_PATH = include
OBJ_PATH     = obj
INSTALL_HEADER = /usr/include/ctrace
INSTALL_HEADER2 = /root/anaconda3/include/ctrace

CFLAGS     = -fPIC -O3 -I$(INCLUDE_PATH) -I$(AFLPP)/include
#CFLAGS    = -fPIC -g -I$(INCLUDE_PATH) -I$(AFLPP)/include -D__DEBUG__ 
LD_CFLAGS  = -fPIC -shared -lpthread -ldl -lshmQueue $(AFLPP)/afl-compiler-rt-64.o

$SOURCE_FILES = $(wildcard $(SOURCE_PATH)/*.c)
SOURCE_FILES  = $(wildcard $(SOURCE_PATH)/*.c) #source/Queue.c source/DynTrace.c source/DbCore.c source/List.c
OBJ_FILES     = $(addprefix $(OBJ_PATH)/, $(addsuffix .o,$(notdir $(basename $(SOURCE_FILES)))))

.PHONY:all clean

all: $(TARGET)
	

$(TARGET): $(OBJ_FILES)
	@if [ -f $(AFLPP)/afl-compiler-rt-64.o ]; then $(LD) -o $@ $^ $(LD_CFLAGS); fi
	@if [ -f $(TARGET) ]; then cp $(TARGET) -rf /usr/lib/; fi

$(OBJ_PATH)/%.o: $(SOURCE_PATH)/%.c
	@if [ ! -d $(INSTALL_HEADER) ];then mkdir $(INSTALL_HEADER); fi
	@cp $(INCLUDE_PATH)/* -rf $(INSTALL_HEADER)/
	@if [ ! -d $(INSTALL_HEADER2) ];then mkdir $(INSTALL_HEADER2); fi
	@cp $(INCLUDE_PATH)/* -rf $(INSTALL_HEADER2)/
	@if [ ! -d $(OBJ_PATH) ];then mkdir $(OBJ_PATH); fi
	$(CC) $(CFLAGS) -c -o $@ $<



clean:
	rm -rf $(OBJ_PATH)
	rm -rf $(TARGET)

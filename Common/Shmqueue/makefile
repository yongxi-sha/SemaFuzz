

TARGET = libshmQueue.so

CC = clang
LD = $(CC)

SOURCE_PATH  = source
INCLUDE_PATH = include
OBJ_PATH     = obj

CFLAGS     = -fPIC -O3 -I$(INCLUDE_PATH) -DQUEUE_OPT
#CFLAGS    = -fPIC -g -I$(INCLUDE_PATH) -D__DEBUG__

LD_CFLAGS  = -shared -lpthread 

SOURCE_FILES  = $(wildcard $(SOURCE_PATH)/*.c)
OBJ_FILES     = $(addprefix $(OBJ_PATH)/, $(addsuffix .o,$(notdir $(basename $(SOURCE_FILES)))))

.PHONY:all clean

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(LD) -o $@ $^ $(LD_CFLAGS)
	@cp $(INCLUDE_PATH)/* -rf /usr/include/
	@if [ -d "/root/anaconda3" ];then cp $(INCLUDE_PATH)/* -rf /root/anaconda3/include/;fi
	@cp $(TARGET) -rf /usr/lib/
	@if [ -d "/root/anaconda3" ];then cp $(TARGET) -rf /root/anaconda3/lib/;fi
	
	
$(OBJ_PATH)/%.o: $(SOURCE_PATH)/%.c
	@if [ ! -d $(OBJ_PATH) ];then mkdir $(OBJ_PATH); fi
	$(CC) $(CFLAGS) -c -o $@ $<



clean:
	rm -rf $(OBJ_PATH)
	rm -rf $(TARGET)

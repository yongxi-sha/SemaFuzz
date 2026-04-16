

TARGET = jsonfuzz


SOURCE_PATH  = source
INCLUDE_PATH = include
OBJ_PATH     = obj

CFLAGS     = -I$(INCLUDE_PATH) -I$(JSON_PATH) -std=c++20
LD_CFLAGS  = -lstdc++ -lm -lpthread

SOURCE_FILES  = $(wildcard $(SOURCE_PATH)/*.cpp)
OBJ_FILES     = $(addprefix $(OBJ_PATH)/, $(addsuffix .o,$(notdir $(basename $(SOURCE_FILES)))))

.PHONY:all clean

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) -o $@ $^ $(LD_CFLAGS)
	@cp $(TARGET) -rf /usr/bin/
	
$(OBJ_PATH)/%.o: $(SOURCE_PATH)/%.cpp
	@if [ ! -d $(OBJ_PATH) ];then mkdir $(OBJ_PATH); fi
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_PATH)
	rm -rf $(TARGET)

OBJ = drm.c v4l2-dec.c test.c demux.c secmem.c
TARGET = v4l2-uvm-test

# rules
all: $(TARGET)

#TARGET_CFLAGS += -DDEBUG_FRAME
LD_FLAG = -ldrm -lavformat -lavcodec -lavutil -lm -lpthread -lz -lswresample -lsecmem

$(TARGET): $(OBJ)
	$(CC) $(TARGET_CFLAGS)  -D_FILE_OFFSET_BITS=64 -Wall -I$(STAGING_DIR)/usr/include/ -I$(STAGING_DIR)/usr/include/libdrm/ -L$(STAGING_DIR)/usr/lib $(LD_FLAG) $(OBJ) -o $@

.PHONY: clean

clean:
	rm -f *.o $(TARGET)

install:
	cp v4l2-uvm-test $(TARGET_DIR)/usr/bin/

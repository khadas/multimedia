.PHONY : all install uninstall

ifeq ($(ENABLE_DTS_64bit),yes)
SRC_DIR=64bit
else
SRC_DIR=32bit
endif

all:
	@echo "Nothing should be done"

clean:
	rm -f $(TARGET_DIR)/usr/lib/libHwAudio_dtsxp1.so \
		$(TARGET_DIR)/usr/lib/libHwAudio_dtshd.so $(TARGET_DIR)/usr/lib/libdtscore.so

install: clean
	if [ -s ./$(SRC_DIR)/libHwAudio_dtsxp1.so ]; then \
		install -m 0644 ./$(SRC_DIR)/libHwAudio_dtsxp1.so $(TARGET_DIR)/usr/lib/ ;\
	elif [ -s ./$(SRC_DIR)/libHwAudio_dtshd.so ]; then \
		install -m 0644 ./$(SRC_DIR)/libHwAudio_dtshd.so $(TARGET_DIR)/usr/lib/ ;\
	elif [ -s ./$(SRC_DIR)/libdtscore.so ]; then \
		install -m 0644 ./$(SRC_DIR)/libdtscore.so $(TARGET_DIR)/usr/lib/ ;\
	else \
		echo "No DTS library is found. Install nothing."; \
	fi


uninstall: clean

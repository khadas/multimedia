TAR=libdolbyms12.so
DMS12_FW=dolby_fw_dms12
SCRIPT=S55dsms12

all: $(TAR)

clean:
	rm -f $(TARGET_DIR)/sbin/$(DMS12_FW)
	rm -f $(TARGET_DIR)/usr/lib/$(TAR)
	rm -f $(TARGET_DIR)/etc/init.d/$(SCRIPT)
install:
	install -m 0755 ./$(BR2_ARCH).$(CC_TARGET_ABI_).$(CC_TARGET_FLOAT_ABI_)/dolby_fw_dms12 $(TARGET_DIR)/sbin/$(DMS12_FW)
	mkdir -p $(TARGET_DIR)/vendor/lib
	cp -P ./$(TAR).symbol_link $(TARGET_DIR)/vendor/lib/$(TAR)
	install -m 0755 ./$(TAR) $(TARGET_DIR)/usr/lib/
	install -m 0755 ./$(SCRIPT) $(TARGET_DIR)/etc/init.d/
uninstall:
	rm -f $(TARGET_DIR)/sbin/$(DOLBY_FW)
	rm -f $(TARGET_DIR)/usr/lib/$(TAR)
	rm -f $(TARGET_DIR)/etc/init.d/$(SCRIPT)

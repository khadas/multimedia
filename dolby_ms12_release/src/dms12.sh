#!/bin/sh
#
# Dolby security firmware decryption

DMS12=/sbin/dolby_fw_dms12
touch /tmp/dms12.lock
if [ ! -d "/tmp/ds/" ]; then
    mkdir -p /tmp/ds/
fi
$DMS12 /usr/lib/libdolbyms12.so /tmp/ds/0x4d_0x5331_0x32.so
mkdir -p /vendor/lib/
rm /vendor/lib/libdolbyms12.so
ln -s /tmp/ds/0x4d_0x5331_0x32.so /vendor/lib/libdolbyms12.so
rm /tmp/dms12.lock

@echo off
if %1.==. goto no_port

if not exist output\enc-bt.bin (
echo encrypted bootloader not found in output folder or need to rename it
goto end
)

if not exist output\enc-pt.bin (
echo encrypted partition table not found in output folder or need to rename it
goto end
)

if not exist output\enc-ota-init.bin (
echo encrypted OTA partition not found in output folder or need to rename it
goto end
)

if not exist output\enc-fw.bin (
echo encrypted mlt-sw-v2.bin not found in binaries folder or need to rename it
goto end
)

esptool -p %1 -b 921600 write_flash 0x1000 output\enc-bt.bin 0xc000 output\enc-pt.bin 0x16000 output\enc-ota-init.bin 0x20000 output\enc-fw.bin --force

goto end
:no_port
    echo COM_PORT shouldn't be empty
:end
pause

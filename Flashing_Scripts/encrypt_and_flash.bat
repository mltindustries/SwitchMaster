@echo off
if %1.==. goto no_port

if not exist binaries\bootloader.bin (
echo bootloader.bin not found in binaries folder or need to rename it to bootloader.bin
goto end
)

if not exist binaries\partition-table.bin (
echo partition-table.bin not found in binaries folder or need to rename it to partition-table.bin
goto end
)

if not exist binaries\ota_data_initial.bin (
echo ota_data_initial.bin not found in binaries folder or need to rename it to ota_data_initial.bin
goto end
)

if not exist binaries\mlt-sw-v2.bin (
echo mlt-sw-v2.bin not found in binaries folder or need to rename it to mlt-sw-v2.bin
goto end
)

if not exist keys\SMv2_flash_encryption_key.bin (
echo FLASH ENCRYPTION KEY not found in key folder or need to rename it to sm_flash_encryption_key.bin
goto end
)
espsecure encrypt_flash_data --keyfile keys\SMv2_flash_encryption_key.bin --address 0x1000 --output output\enc-bt.bin binaries\bootloader.bin
espsecure encrypt_flash_data --keyfile keys\SMv2_flash_encryption_key.bin --address 0xc000 --output output\enc-pt.bin binaries\partition-table.bin
espsecure encrypt_flash_data --keyfile keys\SMv2_flash_encryption_key.bin --address 0x16000 --output output\enc-ota-init.bin binaries\ota_data_initial.bin
espsecure encrypt_flash_data --keyfile keys\SMv2_flash_encryption_key.bin --address 0x20000 --output output\enc-fw.bin binaries\mlt-sw-v2.bin

esptool -p %1 -b 921600 write_flash 0x1000 output\enc-bt.bin 0xc000 output\enc-pt.bin 0x16000 output\enc-ota-init.bin 0x20000 output\enc-fw.bin --force

goto end
:no_port
    echo COM_PORT shouldn't be empty
:end
pause

@echo off
if %1.==. goto no_port

if not exist keys/smv2_flash_encryption_key.bin (
echo FLASH ENCRYPTION KEY not found in key folder or need to rename it to smv2_flash_encryption_key.bin
goto end
)

echo *** BURNING FLASH ENCRYPTION KEY ***
espefuse --port %1 burn_key flash_encryption keys/smv2_flash_encryption_key.bin
echo BURNT FLASH ENCRYPTION KEY. Wait 3 seconds before next action...
pause
echo ***SETTING FLASH_CRYPT_CNT ***
espefuse --port %1 burn_efuse FLASH_CRYPT_CNT 127
echo SET FLASH_CRYPT_CNT. Wait 3 seconds before next action...
pause
echo ***SETTING FLASH_CRYPT_CONFIG ***
espefuse --port %1 burn_efuse FLASH_CRYPT_CONFIG 0xF
echo SET FLASH_CRYPT_CONFIG. Wait 3 seconds before next action....
pause
espefuse --chip esp32 --port %1 summary

goto end
:no_port
    echo COM_PORT shouldn't be empty
:end

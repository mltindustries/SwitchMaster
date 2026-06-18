@echo off
if %1.==. goto no_port

if not exist keys\SMv2_SBv2_public_digest.bin (
echo FLASH ENCRYPTION KEY not found in key folder or need to rename it to sm_flash_encryption_key.bin
goto end
)

echo *** BURNING SECUREBOOT V2 KEY ***
espefuse --port %1 --chip esp32 burn_key secure_boot_v2 keys/SMv2_SBv2_public_digest.bin
echo BURNT SECUREBOOT V2 KEY. Wait 3 seconds before next action...
pause
echo *** ENABLING SECURE BOOT ***
espefuse --port COM3 --chip esp32 burn_efuse ABS_DONE_1
echo SecureBoot v2 Enabled. 
espefuse --chip esp32 --port %1 summary

goto end
:no_port
    echo COM_PORT shouldn't be empty
:end

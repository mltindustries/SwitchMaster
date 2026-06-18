# SwitchMaster Firmware

SwitchMaster is an ESP32-based Bluetooth switch controller developed by MLT Industries.

The project has been open sourced to ensure the platform remains available to the community and can continue to evolve regardless of future commercial direction.

This repository contains:

* Firmware source code
* Flash Encryption keys
* Secure Boot V2 keys
* Manufacturing and provisioning scripts
* Firmware flashing utilities

Everything required to build, modify, provision and maintain both existing and future SwitchMaster hardware has been included.

---

# Philosophy

SwitchMaster was originally released as a commercial product.

By open sourcing the firmware, Flash Encryption keys, Secure Boot V2 keys and manufacturing tooling, both existing and future hardware can continue to be maintained independently of MLT Industries.

If you own a SwitchMaster, you own it completely.

Modify it.
Repair it.
Improve it.
Share it.

---

# Development Environment

The project was developed using:

* ESP-IDF v5.1.4
* Arduino Core for ESP-IDF
* NimBLE Bluetooth Library

All commands in this document should be run from an **ESP-IDF Command Prompt** unless otherwise specified.

---

# Compilation

If you encounter compilation errors relating to Arduino libraries, apply the supplied patch before building.

Open Git Bash in the project root and run:

```bash
git apply --directory=components/arduino ./patches/arduino_lib.patch
```

Build the firmware:

```bash
idf.py build
```

---

# New Community Hardware Builds

For new hardware builds, standard ESP-IDF flashing is generally sufficient:

```bash
idf.py build
idf.py flash
```

Builders are free to:

* Use their own Flash Encryption keys
* Use their own Secure Boot V2 keys
* Use alternative security models
* Disable security features entirely

---

# Existing SwitchMaster Hardware

Original production SwitchMaster devices were provisioned with:

* Flash Encryption
* Secure Boot V2

Because of this, firmware intended for existing production hardware must be:

1. Signed
2. Encrypted
3. Flashed

All original keys and supporting scripts have been included.

They can be found in:

```text
Flashing_Scripts/
```

This allows existing production hardware to continue operating indefinitely without requiring replacement hardware.

---

# Build Outputs

After a successful build the following files will be generated:

| File                 | Location               |
| -------------------- | ---------------------- |
| bootloader.bin       | build/bootloader/      |
| partition-table.bin  | build/partition_table/ |
| ota_initial_data.bin | build/                 |
| mlt-sw-v2.bin        | build/                 |

---

# Flash Layout

| Binary               | Address |
| -------------------- | ------- |
| bootloader.bin       | 0x1000  |
| partition-table.bin  | 0xC000  |
| ota_initial_data.bin | 0x16000 |
| mlt-sw-v2.bin        | 0x20000 |

---

# Legacy Production Flashing Workflow

Copy the following files into:

```text
Flashing_Scripts/binaries/
```

Required files:

```text
bootloader.bin
partition-table.bin
ota_initial_data.bin
mlt-sw-v2.bin
```

---

# Encrypt Firmware

From an ESP-IDF Command Prompt:

```cmd
encrypt_only.bat COM6
```

---

# Encrypt and Flash

```cmd
encrypt_and_flash.bat COM6
```

---

# Flash Existing Encrypted Images

```cmd
flash_only.bat COM6
```

Replace COM6 with the appropriate serial port.

---

# Check Device Security Status

Display Flash Encryption, Secure Boot and eFuse configuration:

```cmd
espefuse.py --chip esp32 -p COM3 summary
```

---

# Provisioning Existing Hardware Security

## Burn Flash Encryption Key

```cmd
espefuse.py --port COM3 burn_key flash_encryption sm_flash_encryption_key.bin
```

## Burn Secure Boot V2 Key

```cmd
espefuse.py --port COM3 --chip esp32 burn_key secure_boot_v2 SM_SB_v2_public_digest.bin
```

## Enable Flash Encryption

```cmd
espefuse.py --port COM3 burn_efuse FLASH_CRYPT_CNT 127
```

```cmd
espefuse.py --port COM3 burn_efuse FLASH_CRYPT_CONFIG 0xF
```

## Enable Secure Boot V2

```cmd
espefuse.py --port COM3 --chip esp32 burn_efuse ABS_DONE_1
```

Verify configuration:

```cmd
espefuse.py --chip esp32 -p COM3 summary
```

Warning: eFuse operations are generally irreversible.

Do not run these commands unless you understand exactly what they do.

---

# Secure Boot V2

Secure Boot V2 is configured within ESP-IDF through Menuconfig.

Firmware images are signed during the build process and validated during boot.

For existing production devices, firmware must be correctly signed before deployment.

---

# Manual Firmware Encryption

ESP-IDF reference:

https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/flash-encryption.html

Example:

```cmd
espsecure.py encrypt_flash_data --keyfile key.bin --address 0x20000 --output my-app-ciphertext.bin build/my-app.bin
```

The resulting ciphertext image can then be flashed to address:

```text
0x20000
```

If the firmware does not boot:

* Verify the correct encryption key is being used
* Verify flash offsets are correct
* Verify FLASH_CRYPT_CONFIG matches the device configuration

---

# Manual Firmware Signing

Example:

```cmd
espsecure.py sign_data --keyfile private_signing_key.pem --output signed-firmware.bin firmware.bin
```

For existing production hardware, firmware must be signed with the correct Secure Boot V2 signing key.

---

# Useful References

General ESP-IDF Reading:

https://www.esp32.com/viewtopic.php?t=24751

Flash Encryption:

https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/flash-encryption.html

Host-Based Security Workflow:

https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/security/host-based-security-workflows.html


---

# Licence

Licensed under the Apache License 2.0.

See the LICENSE file for details.

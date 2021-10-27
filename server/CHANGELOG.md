# Changelog

All notable changes by HENSOLDT Cyber GmbH to this 3rd party module included in
the TRENTOS SDK will be documented in this file.

For more details it is recommended to compare the 3rd party module at hand with
the previous versions of the TRENTOS SDK or the baseline version.

## [1.3]

### Added

- Support bcm2711.

### Fixed

- Use the SEL4_PRI_word macro instead of %u modifier to account for 64-bit arch
cases.

## [1.1]

### Changed

- Use OS_Error_t and rename server according to interface.
- Limit to 8 clients.

## [1.0]

### Changed

- Support the platform Raspberry Pi 3 Model B+ platform.
- Apply our coding style / formatting.

### Added

- Start development based on commit a76489 of
<https://github.com/seL4/global-components/tree/master/components/TimeServer>.

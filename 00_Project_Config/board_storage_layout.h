/**
 ******************************************************************************
 *@file               :   board_storage_layout.h
 *@brief              :   External nonvolatile storage partition layout.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef BOARD_STORAGE_LAYOUT_H
#define BOARD_STORAGE_LAYOUT_H

#define BOARD_STORAGE_CAPACITY_BYTES             0x01000000UL

#define BOARD_STORAGE_FILESYSTEM_BASE_ADDRESS    0x00000000UL
#define BOARD_STORAGE_FILESYSTEM_SIZE_BYTES      0x00E00000UL

#define BOARD_STORAGE_OTA_BASE_ADDRESS           0x00E00000UL
#define BOARD_STORAGE_OTA_SIZE_BYTES             0x00200000UL

/* Shared APP/Boot staging format: four little-endian words, commit written last. */
#define BOARD_OTA_APP_BASE_ADDRESS (0x08008000U)
#define BOARD_OTA_APP_LIMIT_ADDRESS (0x080FF800U)
#define BOARD_OTA_APP_SIZE_BYTES (BOARD_OTA_APP_LIMIT_ADDRESS - BOARD_OTA_APP_BASE_ADDRESS)
#define BOARD_OTA_COMMIT_V1 (0x3141544FU)

_Static_assert(BOARD_STORAGE_OTA_BASE_ADDRESS ==
                   (BOARD_STORAGE_FILESYSTEM_BASE_ADDRESS +
                    BOARD_STORAGE_FILESYSTEM_SIZE_BYTES),
               "Filesystem and OTA regions are not contiguous");
_Static_assert((BOARD_STORAGE_OTA_BASE_ADDRESS +
                BOARD_STORAGE_OTA_SIZE_BYTES) ==
                   BOARD_STORAGE_CAPACITY_BYTES,
               "Storage partition layout does not cover the Flash");

#endif /* BOARD_STORAGE_LAYOUT_H */

#ifndef SD_DISK_STATE_H
#define SD_DISK_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

/* SD/FatFs adapter lifecycle hook. Only its serialized file owner may call
 * this after unmount or before a fresh mount; never during another SD I/O.
 * Clears cached initialization without exposing FatFs/MCU internals. */
void sd_disk_invalidate(void);

#ifdef __cplusplus
}
#endif
#endif

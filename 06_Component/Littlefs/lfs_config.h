/*
 * Project configuration for littlefs.
 *
 * Keep these options local to the component instead of exporting them as
 * global compiler definitions.
 */
#ifndef LFS_PROJECT_CONFIG_H
#define LFS_PROJECT_CONFIG_H

/* All filesystem and file caches are supplied by the application. */
#define LFS_NO_MALLOC

/* Do not route diagnostics through the C library printf/stdio backend. */
#define LFS_NO_DEBUG
#define LFS_NO_WARN
#define LFS_NO_ERROR

/* Configuration invariants are checked by the integration layer. */
#define LFS_NO_ASSERT

#endif /* LFS_PROJECT_CONFIG_H */

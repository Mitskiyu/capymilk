#ifndef PLATFORM_H
#define PLATFORM_H

internal void plat_init(void);
internal u64 plat_time_usec(void);
internal char *plat_file_read(const char *file_name);

#endif

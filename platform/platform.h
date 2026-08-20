#ifndef PLATFORM_H
#define PLATFORM_H

internal void plat_init(void);

internal u64 plat_time_usec(void);

internal void plat_fatal_err(const char *msg, i32 code);

internal char *plat_file_read(mem_arena *arena, const char *file_name);

internal void *plat_mem_reserve(u64 reserve_size);
internal b32 plat_mem_release(void *mem, u64 size);
internal b32 plat_mem_commit(void *mem, u64 size);
internal b32 plat_mem_decommit(void *mem, u64 size);

internal u32 plat_page_size();

#endif

#ifndef BASE_ARENA_H
#define BASE_ARENA_H

#define ARENA_ALIGN (sizeof(void*))

typedef struct {
    u64 reserve_size;
    u64 commit_size;

    u64 commit_pos;
    u64 pos;
} mem_arena;

#define PUSH_STRUCT(arena, T) (T*)arena_push((arena), sizeof(T), false)
#define PUSH_STRUCT_NZ(arena, T) (T*)arena_push((arena), sizeof(T), true)
#define PUSH_ARRAY(arena, T, n) (T*)arena_push((arena), sizeof(T) * (u64)(n), false)
#define PUSH_ARRAY_NZ(arena, T, n) (T*)arena_push((arena), sizeof(T) * (u64)(n), true)

internal mem_arena *arena_alloc(u64 reserve_size, u64 commit_size);
internal void arena_release(mem_arena *arena);
internal u64 arena_get_pos(mem_arena *arena);
internal void *arena_push(mem_arena *arena, u64 size, b32 non_zero);
internal void arena_pop(mem_arena *arena, u64 size);
internal void arena_pop_to(mem_arena *arena, u64 pos);
internal void arena_clear(mem_arena *arena);

#endif

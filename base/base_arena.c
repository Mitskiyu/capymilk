internal mem_arena *arena_alloc(u64 reserve_size, u64 commit_size) {
    u32 page_size = plat_page_size();

    reserve_size = ALIGN_UP_POW2(reserve_size, page_size);
    commit_size = ALIGN_UP_POW2(commit_size, page_size);

    mem_arena *arena = plat_mem_reserve(reserve_size);

    if (!plat_mem_commit(arena, commit_size)) {
        plat_mem_release(arena, reserve_size);
        return NULL; // fatal
    }

    arena->reserve_size = reserve_size;
    arena->commit_size = commit_size;

    arena->commit_pos = commit_size;
    arena->pos = sizeof(mem_arena);

    return arena;
}

internal void arena_release(mem_arena *arena) {
    plat_mem_release(arena, arena->reserve_size);
}

internal u64 arena_get_pos(mem_arena *arena) {
    return arena->pos;
}

internal void *arena_push(mem_arena *arena, u64 size) {
    u64 pos_aligned = ALIGN_UP_POW2(arena->pos, ARENA_ALIGN);
    u64 new_pos = pos_aligned + size;

    if (new_pos > arena->reserve_size) { return NULL; } // fatal

    if (new_pos > arena->commit_pos) {
        u64 new_commit_pos = new_pos;
        new_commit_pos += arena->commit_size - 1;
        new_commit_pos -= new_commit_pos % arena->commit_size;        
        new_commit_pos = MIN(new_commit_pos, arena->reserve_size);

        u64 commit_size = new_commit_pos - arena->commit_pos;

        u8 *commit_ptr = (u8*)arena + arena->commit_pos;

        // grow
        if (!plat_mem_commit(commit_ptr, commit_size)) { return NULL; } // fatal

        arena->commit_pos = new_commit_pos;
    }

    arena->pos = new_pos;
    u8* out = (u8*)arena + pos_aligned;

    return out;
}

internal void arena_pop(mem_arena *arena, u64 size) {
    size = MIN(size, arena->pos - sizeof(mem_arena));
    arena->pos -= size;
}

internal void arena_pop_to(mem_arena *arena, u64 pos) {
    u64 cur_pos = arena_get_pos(arena);
    pos = MIN(pos, cur_pos);

    arena_pop(arena, cur_pos - pos);
}

internal void arena_clear(mem_arena *arena) {
    arena_pop_to(arena, sizeof(mem_arena));
}

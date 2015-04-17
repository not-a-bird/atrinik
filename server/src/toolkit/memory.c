/*************************************************************************
 *           Atrinik, a Multiplayer Online Role Playing Game             *
 *                                                                       *
 *   Copyright (C) 2009-2014 Alex Tokar and Atrinik Development Team     *
 *                                                                       *
 * Fork from Crossfire (Multiplayer game for X-windows).                 *
 *                                                                       *
 * This program is free software; you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program; if not, write to the Free Software           *
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.             *
 *                                                                       *
 * The author can be reached at admin@atrinik.org                        *
 ************************************************************************/

/**
 * @file
 * Memory API.
 */

#ifndef __CPROTO__

#include <global.h>

/**
 * Name of the API.
 */
#define API_NAME memory

/**
 * If 1, the API has been initialized.
 */
static uint8_t did_init = 0;

#ifndef NDEBUG

#ifdef HAVE_VALGRIND_H
#   include <valgrind/valgrind.h>
#else
#define RUNNING_ON_VALGRIND (0)
#endif

#define CHUNK_BEFORE_VAL 0x539A4C3F659C5B2EULL
#define CHUNK_AFTER_VAL  0x659C5B2E539A4C3FULL

typedef struct memory_chunk {
    struct memory_chunk *next; ///< Next memory chunk.
    struct memory_chunk *prev; ///< Previous memory chunk.

    size_t size; ///< Number of the bytes in 'data'.
    memory_status_t status; ///< Status of the memory chunk.
    const char *file; ///< File the memory chunk was allocated in.
    uint32_t line; ///< Line number the chunk was allocated in.

    uint64_t before; ///< Value before the data to check for underruns.
    char data[1]; ///< The allocated data.
} memory_chunk_t;

#define MEM_CHUNK_SIZE(_n) \
    (sizeof(memory_chunk_t) - 1 + (_n) + sizeof(uint64_t))
#define MEM_DATA(_chunk) ((void *) &((_chunk)->data[0]))
#define MEM_CHUNK(_ptr) \
    ((memory_chunk_t *) ((char *) _ptr - offsetof(memory_chunk_t, data[0])))

static memory_chunk_t *memory_chunks;
static ssize_t memory_chunks_num;
static ssize_t memory_chunks_allocated;
static ssize_t memory_chunks_allocated_max;
static uint64_t after_data;

static const char *chunk_get_str(memory_chunk_t *chunk);

#else
#define _malloc(_size, _file, _line) malloc(_size)
#define _free(_ptr, _file, _line) free(_ptr)
#define _calloc(_nmemb, _size, _file, _line) calloc(_nmemb, _size)
#define _realloc(_ptr, _size, _file, _line) realloc(_ptr, _size)
#endif

/**
 * Initialize the memory API.
 * @internal
 */
void toolkit_memory_init(void)
{
    TOOLKIT_INIT_FUNC_START(memory)
    {
        memory_chunks = NULL;
        memory_chunks_num = 0;
        memory_chunks_allocated = 0;
        memory_chunks_allocated_max = 0;
        after_data = CHUNK_AFTER_VAL;
    }
    TOOLKIT_INIT_FUNC_END()
}

/**
 * Deinitialize the memory API.
 * @internal
 */
void toolkit_memory_deinit(void)
{
    TOOLKIT_DEINIT_FUNC_START(memory)
    {
#ifndef NDEBUG
        memory_chunk_t *chunk;

        DL_FOREACH(memory_chunks, chunk) {
            log(LOG(ERROR), "Unfreed pointer: %s", chunk_get_str(chunk));
        }

        log(LOG(INFO), "Maximum number of bytes allocated: %" PRIu64,
                (uint64_t) memory_chunks_allocated_max);

        if (memory_chunks_num != 0) {
            log(LOG(ERROR), "Number of pointers still allocated: %" PRIu64,
                    (uint64_t) memory_chunks_num);
        }

        if (memory_chunks_allocated != 0) {
            log(LOG(ERROR), "Number of bytes still allocated: %" PRIu64,
                    (uint64_t) memory_chunks_allocated);
        }
#endif
    }
    TOOLKIT_DEINIT_FUNC_END()
}

#ifndef NDEBUG
static const char *chunk_get_str(memory_chunk_t *chunk)
{
    static char buf[MAX_BUF];

    snprintf(VS(buf), "Chunk %p (%" PRId64 " bytes) allocated in %s:%u", chunk,
             (uint64_t) chunk->size, chunk->file, chunk->line);

    return buf;
}

static void chunk_check(memory_chunk_t *chunk)
{
    /* Check for underrun */
    if (chunk->before != CHUNK_BEFORE_VAL) {
        log_error("Pointer underrun detected: %s", chunk_get_str(chunk));
        abort();
    }

    /* Check for overrun */
    if (*(uint64_t *) (&chunk->data[chunk->size]) != CHUNK_AFTER_VAL) {
        log_error("Pointer overrun detected: %s", chunk_get_str(chunk));
        abort();
    }
}

static memory_chunk_t *chunk_checkptr(void *ptr)
{
    memory_chunk_t *chunk;

    DL_FOREACH(memory_chunks, chunk) {
        if (ptr >= (void *) &chunk->data[0] && ptr < (void *) ((char *) chunk +
                MEM_CHUNK_SIZE(chunk->size))) {
            break;
        }
    }

    if (chunk == NULL) {
        return NULL;
    }

    chunk_check(chunk);
    return chunk;
}

static void chunk_check_all(void)
{
    memory_chunk_t *chunk;

    DL_FOREACH(memory_chunks, chunk) {
        chunk_check(chunk);
    }
}

static void *_malloc(size_t size, const char *file, uint32_t line)
{
    memory_chunk_t *chunk;

    chunk = malloc(MEM_CHUNK_SIZE(size));

    if (chunk == NULL) {
        logger_print(LOG(ERROR), "OOM (size: %" PRIu64 ").",
                (uint64_t) MEM_CHUNK_SIZE(size));
        abort();
    }

    chunk->size = size;
    chunk->file = file;
    chunk->line = line;
    chunk->status = MEMORY_STATUS_OK;
    chunk->before = CHUNK_BEFORE_VAL;
    chunk->next = NULL;
    chunk->prev = NULL;
    *(uint64_t *) (&chunk->data[chunk->size]) = CHUNK_AFTER_VAL;

    DL_APPEND(memory_chunks, chunk);

    memory_chunks_num++;
    memory_chunks_allocated += size;

    if (memory_chunks_allocated > memory_chunks_allocated_max) {
        memory_chunks_allocated_max = memory_chunks_allocated;
    }

    if (!RUNNING_ON_VALGRIND) {
        memset(MEM_DATA(chunk), 0xEE, size);
    }

    printf("Allocated pointer: %p\n", MEM_DATA(chunk));

    return MEM_DATA(chunk);
}

static void _free(void *ptr, const char *file, uint32_t line)
{
    memory_chunk_t *chunk;

    if (ptr == NULL) {
        return;
    }

    if (memory_chunks_num <= 0) {
        log_error("More frees than allocs (%" PRId64 "), free called from: "
                  "%s:%u", (int64_t) memory_chunks_num, file, line);
        abort();
    }

    chunk = chunk_checkptr(ptr);

    if (chunk == NULL) {
        log(LOG(INFO), "Invalid pointer: %p", ptr);
    }

    /* Check for underrun */
    if (chunk->before != CHUNK_BEFORE_VAL) {
        log_error("Pointer underrun detected: %s, free called from: %s:%u",
                  chunk_get_str(chunk), file, line);
        abort();
    }

    /* Check for overrun */
    if (*(uint64_t *) (&chunk->data[chunk->size]) != CHUNK_AFTER_VAL) {
        log_error("Pointer overrun detected: %s, free called from: %s:%u",
                  chunk_get_str(chunk), file, line);
        abort();
    }

    memory_chunks_allocated -= chunk->size;

    if (memory_chunks_allocated < 0) {
        log_error("Freed more bytes than what should be possible: %s, now "
                  "allocated: %" PRId64 ", free called from: %s:%u",
                  chunk_get_str(chunk), (int64_t) memory_chunks_allocated,
                  file, line);
        abort();
    }

    DL_DELETE(memory_chunks, chunk);
    memory_chunks_num--;

    if (!RUNNING_ON_VALGRIND) {
        memset(chunk, 0x7A, MEM_CHUNK_SIZE(chunk->size));
    }

    free(chunk);
}

static void *_calloc(size_t nmemb, size_t size, const char *file, uint32_t line)
{
    void *ptr;

    ptr = _malloc(size * nmemb, file, line);
    memset(ptr, 0, size * nmemb);

    return ptr;
}

static void *_realloc(void *ptr, size_t size, const char *file, uint32_t line)
{
    memory_chunk_t *chunk;
    void *new_ptr;

    new_ptr = _malloc(size, file, line);

    if (ptr != NULL) {
        chunk = MEM_CHUNK(ptr);

        if (chunk->size < size) {
            size = chunk->size;
        }

        printf("copying %d bytes\n", size);
        memcpy(new_ptr, ptr, size);
        _free(ptr, file, line);
    }

    return new_ptr;
}
#endif

void memory_check_all(void)
{
    TOOLKIT_FUNC_PROTECTOR(API_NAME);

#ifndef NDEBUG
    chunk_check_all();
#endif
}

bool memory_check(void *ptr)
{
    TOOLKIT_FUNC_PROTECTOR(API_NAME);

#ifndef NDEBUG
    memory_chunk_t *chunk;

    chunk = chunk_checkptr(ptr);

    if (chunk == NULL) {
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool memory_get_status(void *ptr, memory_status_t *status)
{
    TOOLKIT_FUNC_PROTECTOR(API_NAME);

#ifndef NDEBUG
    memory_chunk_t *chunk;

    chunk = chunk_checkptr(ptr);

    if (chunk == NULL) {
        return false;
    }

    *status = chunk->status;

    return true;
#else
    return false;
#endif
}

bool memory_get_size(void *ptr, size_t *size)
{
    TOOLKIT_FUNC_PROTECTOR(API_NAME);

#ifndef NDEBUG
    memory_chunk_t *chunk;

    chunk = chunk_checkptr(ptr);

    if (chunk == NULL) {
        return false;
    }

    *size = chunk->size;

    return true;
#else
    return false;
#endif
}

/**
 * Like malloc(), but performs error checking.
 * @param size Number of bytes to allocate.
 * @return Allocated pointer, never NULL.
 * @note Will abort() in case the pointer can't be allocated.
 */
#ifndef NDEBUG
void *memory_emalloc(size_t size, const char *file, uint32_t line)
#else
void *memory_emalloc(size_t size)
#endif
{
    TOOLKIT_FUNC_PROTECTOR(API_NAME);

    void *ptr;

    ptr = _malloc(size, file, line);

    if (ptr == NULL) {
        logger_print(LOG(ERROR), "OOM (size: %"PRIu64").", (uint64_t) size);
        abort();
    }

    return ptr;
}

/**
 * Like free(), but performs error checking.
 * @param ptr Pointer to free.
 * @note Will abort() in case the pointer is NULL.
 */
#ifndef NDEBUG
void memory_efree(void *ptr, const char *file, uint32_t line)
#else
void memory_efree(void *ptr)
#endif
{
    TOOLKIT_FUNC_PROTECTOR(API_NAME);

    SOFT_ASSERT(ptr != NULL, "Freeing NULL pointer.");
    _free(ptr, file, line);
}

/**
 * Like calloc(), but performs error checking.
 * @param nmemb Elements.
 * @param size Number of bytes.
 * @return Allocated pointer, never NULL.
 * @note Will abort() in case the pointer can't be allocated.
 */
#ifndef NDEBUG
void *memory_ecalloc(size_t nmemb, size_t size, const char *file, uint32_t line)
#else
void *memory_ecalloc(size_t nmemb, size_t size)
#endif
{
    TOOLKIT_FUNC_PROTECTOR(API_NAME);

    void *ptr;

    ptr = _calloc(nmemb, size, file, line);

    if (ptr == NULL) {
        logger_print(LOG(ERROR), "OOM (nmemb: %"PRIu64", size: %"PRIu64").",
                (uint64_t) nmemb, (uint64_t) size);
        abort();
    }

    return ptr;
}

/**
 * Like realloc(), but performs error checking.
 * @param ptr Pointer to resize.
 * @param size New number of bytes.
 * @return Resized pointer, never NULL.
 * @note Will abort() in case the pointer can't be resized.
 */
#ifndef NDEBUG
void *memory_erealloc(void *ptr, size_t size, const char *file, uint32_t line)
#else
void *memory_erealloc(void *ptr, size_t size)
#endif
{
    TOOLKIT_FUNC_PROTECTOR(API_NAME);

    void *newptr;

    newptr = _realloc(ptr, size, file, line);

    if (newptr == NULL && size != 0) {
        logger_print(LOG(ERROR), "OOM (ptr: %p, size: %"PRIu64".", ptr,
                (uint64_t) size);
        abort();
    }

    return newptr;
}

/**
 * Like realloc(), but if more bytes are being allocated, they get set to
 * 0 using memset().
 * @param ptr Original pointer.
 * @param old_size Size of the pointer.
 * @param new_size New size the pointer should have.
 * @return Resized pointer, NULL on failure. */
#ifndef NDEBUG
void *memory_reallocz(void *ptr, size_t old_size, size_t new_size,
        const char *file, uint32_t line)
#else
void *memory_reallocz(void *ptr, size_t old_size, size_t new_size)
#endif
{
    TOOLKIT_FUNC_PROTECTOR(API_NAME);

    void *new_ptr;

#ifndef NDEBUG
    new_ptr = memory_erealloc(ptr, new_size, file, line);
#else
    new_ptr = realloc(ptr, new_size);
#endif

    if (new_ptr && new_size > old_size) {
        memset(((char *) new_ptr) + old_size, 0, new_size - old_size);
    }

    return new_ptr;
}

#endif

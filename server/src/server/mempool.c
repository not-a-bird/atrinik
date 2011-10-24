/************************************************************************
*            Atrinik, a Multiplayer Online Role Playing Game            *
*                                                                       *
*    Copyright (C) 2009-2011 Alex Tokar and Atrinik Development Team    *
*                                                                       *
* Fork from Daimonin (Massive Multiplayer Online Role Playing Game)     *
* and Crossfire (Multiplayer game for X-windows).                       *
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
 * Basic pooling memory management system.
 *
 * The mempool system never frees memory back to the system, but is
 * extremely efficient when it comes to allocating and returning pool
 * chunks.
 *
 * Always use the get_poolchunk() and return_poolchunk() functions for
 * getting and returning memory chunks. expand_mempool() is used
 * internally.
 *
 * Be careful if you want to use the internal chunk or pool data, its
 * semantics and format might change in the future. */

#include <global.h>

/**
 * The removedlist is not ended by NULL, but by a pointer to the end_marker.
 *
 * Only used as an end marker for the lists */
struct mempool_chunk end_marker;

int nrof_mempools = 0;
struct mempool *mempools[MAX_NROF_MEMPOOLS];
struct mempool *pool_object, *pool_objectlink, *pool_player, *pool_bans, *pool_parties;

/**
 * Return the exponent exp needed to round n up to the nearest power of two, so that
 * (1 << exp) >= n and (1 << (exp -1)) \< n */
uint32 nearest_pow_two_exp(uint32 n)
{
	static const uint32 exp_lookup[65] =
	{
		0, 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6
	};
	uint32 i;

	if (n <= 64)
	{
		return exp_lookup[n];
	}

	for (i = 7; (uint32) (1 << i) < n; i++)
	{
	}

	return i;
}

/**
 * A tiny little function to set up the constructors/destructors to
 * functions that may reside outside the library.
 * @param pool Pool.
 * @param constructor Constructor function.
 * @param destructor Destructor function. */
void setup_poolfunctions(struct mempool *pool, chunk_constructor constructor, chunk_destructor destructor)
{
	pool->constructor = constructor;
	pool->destructor = destructor;
}

/**
 * Create a memory pool.
 * @param description
 * @param expand
 * @param size
 * @param flags
 * @param initialisator
 * @param deinitialisator
 * @param constructor
 * @param destructor
 * @return The created memory pool. */
struct mempool *create_mempool(const char *description, uint32 expand, uint32 size, uint32 flags, chunk_initialisator initialisator, chunk_deinitialisator deinitialisator, chunk_constructor constructor, chunk_destructor destructor)
{
	int i;
	struct mempool *pool;

	if (nrof_mempools >= MAX_NROF_MEMPOOLS)
	{
		LOG(llevError, "Too many memory pools registered. Please increase the MAX_NROF_MEMPOOLS constant in mempools.h\n");
	}

	pool = calloc(1, sizeof(struct mempool));

	mempools[nrof_mempools] = pool;

	pthread_mutex_init(&pool->mutex, NULL);
	pool->chunk_description = description;
	pool->expand_size = expand;
	pool->chunksize = size;
	pool->flags = flags;
	pool->initialisator = initialisator;
	pool->deinitialisator = deinitialisator;
	pool->constructor = constructor;
	pool->destructor = destructor;

#if MEMORY_DEBUG
	pool->flags |= MEMPOOL_BYPASS_POOLS;
#endif

	for (i = 0; i < MEMPOOL_NROF_FREELISTS; i++)
	{
		pool->freelist[i] = &end_marker;
		pool->nrof_free[i] = 0;
		pool->nrof_allocated[i] = 0;
	}

	nrof_mempools++;

	return pool;
}

/**
 * Initialize the mempools lists and related data structures. */
void init_mempools(void)
{
	pool_object = create_mempool("objects", OBJECT_EXPAND, sizeof(object), 0, NULL, NULL, (chunk_constructor) initialize_object, NULL);
	pool_player = create_mempool("players", 25, sizeof(player), MEMPOOL_BYPASS_POOLS, NULL, NULL, NULL, NULL);
	pool_objectlink = create_mempool("object links", 500, sizeof(objectlink), 0, NULL, NULL, NULL, NULL);
	pool_bans = create_mempool("bans", 25, sizeof(_ban_struct), 0, NULL, NULL, NULL, NULL);
	pool_parties = create_mempool("parties", 25, sizeof(party_struct), 0, NULL, NULL, NULL, NULL);
}

/**
 * Free a mempool.
 * @param pool The mempool to free. */
static void free_mempool(struct mempool *pool)
{
	free(pool);
}

/**
 * Free all the mempools previously initialized by init_mempools(). */
void free_mempools(void)
{
	LOG(llevDebug, "Freeing memory pools.\n");
	free_mempool(pool_object);
	free_mempool(pool_player);
	free_mempool(pool_objectlink);
	free_mempool(pool_bans);
	free_mempool(pool_parties);
}

/**
 * Expands the memory pool with MEMPOOL_EXPAND new chunks. All new chunks
 * are put into the pool's freelist for future use.
 * expand_mempool is only meant to be used from get_poolchunk().
 * @param pool Pool to expand.
 * @param arraysize_exp The exponent for the array size, for example 3
 * for arrays of length 8 (2^3 = 8) */
static void expand_mempool(struct mempool *pool, uint32 arraysize_exp)
{
	uint32 i;
	struct mempool_chunk *first, *ptr;
	int chunksize_real, nrof_arrays;

	if (pool->nrof_free[arraysize_exp] > 0)
	{
		LOG(llevBug, "expand_mempool() called with chunks still available in pool\n");
	}

	nrof_arrays = pool->expand_size >> arraysize_exp;

	if (nrof_arrays == 0)
	{
		LOG(llevDebug, "expand_mempool() called with too big array size for its expand_size\n");
		nrof_arrays = 1;
	}

	chunksize_real = sizeof(struct mempool_chunk) + (pool->chunksize << arraysize_exp);
	first = (struct mempool_chunk *) calloc(1, nrof_arrays * chunksize_real);

	if (first == NULL)
	{
		LOG(llevError, "expand_mempool(): Out of memory.\n");
	}

	pool->freelist[arraysize_exp] = first;
	pool->nrof_allocated[arraysize_exp] += nrof_arrays;
	pool->nrof_free[arraysize_exp] = nrof_arrays;

	/* Set up the linked list */
	ptr = first;

	for (i = 0; (int) i < nrof_arrays - 1; i++)
	{
		if (pool->initialisator)
		{
			pool->initialisator(MEM_USERDATA(ptr));
		}

		ptr = ptr->next = (struct mempool_chunk *) (((char *) ptr) + chunksize_real);
	}

	/* And the last element */
	ptr->next = &end_marker;

	if (pool->initialisator)
	{
		pool->initialisator(MEM_USERDATA(ptr));
	}
}

/**
 * Get a chunk from the selected pool. The pool will be expanded if
 * necessary.
 * @param pool
 * @param arraysize_exp
 * @return  */
void *get_poolchunk_array_real(struct mempool *pool, uint32 arraysize_exp)
{
	struct mempool_chunk *new_obj;

	pthread_mutex_lock(&pool->mutex);

	if (pool->flags & MEMPOOL_BYPASS_POOLS)
	{
		new_obj = calloc(1, sizeof(struct mempool_chunk) + (pool->chunksize << arraysize_exp));
		pool->nrof_allocated[arraysize_exp]++;
	}
	else
	{
		if (pool->nrof_free[arraysize_exp] == 0)
		{
			expand_mempool(pool, arraysize_exp);
		}

		new_obj = pool->freelist[arraysize_exp];
		pool->freelist[arraysize_exp] = new_obj->next;
		pool->nrof_free[arraysize_exp]--;
	}

	new_obj->next = NULL;

	if (pool->constructor)
	{
		pool->constructor(MEM_USERDATA(new_obj));
	}

	pthread_mutex_unlock(&pool->mutex);

	return MEM_USERDATA(new_obj);
}

/**
 * Return a chunk to the selected pool. Don't return memory to the wrong
 * pool!
 *
 * Returned memory will be reused, so be careful about those stale pointers
 * @param data
 * @param arraysize_exp
 * @param pool  */
void return_poolchunk_array_real(void *data, uint32 arraysize_exp, struct mempool *pool)
{
	struct mempool_chunk *old = MEM_POOLDATA(data);

	if (CHUNK_FREE(data))
	{
		LOG(llevBug, "return_poolchunk_array_real() on already free chunk (pool \"%s\")\n", pool->chunk_description);
		return;
	}

	pthread_mutex_lock(&pool->mutex);

	if (pool->destructor)
	{
		pool->destructor(data);
	}

	if (pool->flags & MEMPOOL_BYPASS_POOLS)
	{
		if (pool->deinitialisator)
		{
			pool->deinitialisator(MEM_USERDATA(old));
		}

		free(old);
		pool->nrof_allocated[arraysize_exp]--;
	}
	else
	{
		old->next = pool->freelist[arraysize_exp];
		pool->freelist[arraysize_exp] = old;
		pool->nrof_free[arraysize_exp]++;
	}

	pthread_mutex_unlock(&pool->mutex);
}

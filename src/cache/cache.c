/**************************************************************************
  * C S 429 system emulator
 * 
 * cache.c - A cache simulator that can replay traces from Valgrind
 *     and output statistics such as number of hits, misses, and
 *     evictions, both dirty and clean.  The replacement policy is LRU. 
 *     The cache is a writeback cache. 
 * 
 * Copyright (c) 2021, 2023, 2024, 2025. 
 * Authors: M. Hinton, Z. Leeper.
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/ 
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include "cache.h"

#define ADDRESS_LENGTH 64

/* Counters used to record cache statistics in printSummary().
   test-cache uses these numbers to verify correctness of the cache. */

//Increment when a miss occurs
int miss_count = 0;

//Increment when a hit occurs
int hit_count = 0;

//Increment when a dirty eviction occurs
int dirty_eviction_count = 0;

//Increment when a clean eviction occurs
int clean_eviction_count = 0;

/* STUDENT TO-DO: add more globals, structs, macros if necessary */
uword_t next_lru;

static size_t _log(size_t x) {
  size_t result = 0;
  while(x>>=1)  {
    result++;
  }
  return result;
}

unsigned extract_bitfield(unsigned src, unsigned frompos, unsigned width) {
    return (unsigned) ((src >> frompos) & ((1 << width) - 1));
}

cache_line_t *get_lru_line(cache_set_t *set, unsigned A) {
    unsigned int lru_min = set->lines[0].lru;
    cache_line_t *lru_line = &set->lines[0];
    for (unsigned int i = 1; i < A; i++) {
        if (set->lines[i].lru < lru_min) {
            lru_min = set->lines[i].lru;
            lru_line = &set->lines[i];
        }
    }
    return lru_line;
}

/*
 * Initialize the cache according to specified arguments
 * Called by cache-runner so do not modify the function signature
 *
 * The code provided here shows you how to initialize a cache structure
 * defined above. It's not complete and feel free to modify/add code.
 */
cache_t *create_cache(int A_in, int B_in, int C_in, int d_in) {
    /* see cache-runner for the meaning of each argument */
    cache_t *cache = malloc(sizeof(cache_t));
    cache->A = A_in;
    cache->B = B_in;
    cache->C = C_in;
    cache->d = d_in;
    unsigned int S = cache->C / (cache->A * cache->B);

    cache->sets = (cache_set_t*) calloc(S, sizeof(cache_set_t));
    for (unsigned int i = 0; i < S; i++){
        cache->sets[i].lines = (cache_line_t*) calloc(cache->A, sizeof(cache_line_t));
        for (unsigned int j = 0; j < cache->A; j++){
            cache->sets[i].lines[j].valid = 0;
            cache->sets[i].lines[j].tag   = 0;
            cache->sets[i].lines[j].lru   = 0;
            cache->sets[i].lines[j].dirty = 0;
            cache->sets[i].lines[j].data  = calloc(cache->B, sizeof(byte_t));
        }
    }

    /* TODO: add more code for initialization */
    next_lru = 0;
    return cache;
}

cache_t *create_checkpoint(cache_t *cache) {
    unsigned int S = (unsigned int) cache->C / (cache->A * cache->B);
    cache_t *copy_cache = malloc(sizeof(cache_t));
    memcpy(copy_cache, cache, sizeof(cache_t));
    copy_cache->sets = (cache_set_t*) calloc(S, sizeof(cache_set_t));
    for (unsigned int i = 0; i < S; i++) {
        copy_cache->sets[i].lines = (cache_line_t*) calloc(cache->A, sizeof(cache_line_t));
        for (unsigned int j = 0; j < cache->A; j++) {
            memcpy(&copy_cache->sets[i].lines[j], &cache->sets[i].lines[j], sizeof(cache_line_t));
            copy_cache->sets[i].lines[j].data = calloc(cache->B, sizeof(byte_t));
            memcpy(copy_cache->sets[i].lines[j].data, cache->sets[i].lines[j].data, sizeof(byte_t));
        }
    }
    
    return copy_cache;
}

void display_set(cache_t *cache, unsigned int set_index) {
    unsigned int S = (unsigned int) cache->C / (cache->A * cache->B);
    if (set_index < S) {
        cache_set_t *set = &cache->sets[set_index];
        for (unsigned int i = 0; i < cache->A; i++) {
            printf ("Valid: %d Tag: %llx Lru: %lld Dirty: %d\n", set->lines[i].valid, 
                set->lines[i].tag, set->lines[i].lru, set->lines[i].dirty);
        }
    } else {
        printf ("Invalid Set %d. 0 <= Set < %d\n", set_index, S);
    }
}

/*
 * Free allocated memory. Feel free to modify it
 */
void free_cache(cache_t *cache) {
    unsigned int S = (unsigned int) cache->C / (cache->A * cache->B);
    for (unsigned int i = 0; i < S; i++){
        for (unsigned int j = 0; j < cache->A; j++) {
            free(cache->sets[i].lines[j].data);
        }
        free(cache->sets[i].lines);
    }
    free(cache->sets);
    free(cache);
}

/* STUDENT TO-DO:
 * Get the line for address contained in the cache
 * On hit, return the cache line holding the address
 * On miss, returns NULL
 */
cache_line_t *get_line(cache_t *cache, uword_t addr) {
    // Student TODO
    unsigned S = cache->C / (cache->A * cache->B);
    unsigned s = _log(S);
    unsigned b = _log(cache->B);
    unsigned set_index = extract_bitfield(addr, b, s);
    unsigned int tag = addr >> (b+s);
    cache_set_t *set = &cache->sets[set_index];
    for (size_t i = 0; i < cache->A; i++) {
        if (set->lines[i].valid == true && set->lines[i].tag == tag) {
            return(&set->lines[i]);
        }
    }
    return NULL;
}

/* STUDENT TO-DO:
 * Select the line to fill with the new cache line
 * Return the cache line selected to filled in by addr
 */
cache_line_t *select_line(cache_t *cache, uword_t addr) {
    unsigned S = cache->C / (cache->A * cache->B);
    unsigned s = _log(S);
    unsigned b = _log(cache->B);
    unsigned set_index = extract_bitfield(addr, b, s);
    unsigned tag = addr >> (b+s);
    cache_set_t *set = &cache->sets[set_index];
    for (size_t i = 0; i < cache->A; i++) {
        if (set->lines[i].valid == false) {
            set->lines[i].tag = tag;
            set->lines[i].lru = next_lru;
            next_lru++;
            return &set->lines[i];
        }
    }
    return get_lru_line(set, cache->A);
}

/*  STUDENT TO-DO:
 *  Check if the address is hit in the cache, updating hit and miss data.
 *  Return true if pos hits in the cache.
 */
bool check_hit(cache_t *cache, uword_t addr, operation_t operation) {
    // Student TODO
    cache_line_t *line = get_line(cache, addr);
    if (line != NULL) {
        hit_count++;
        line->lru = next_lru;
        next_lru++;
        if (operation == WRITE) {
            line->dirty = true;
        }
        return true;
    }
    else {
        miss_count++;
        return false;
    }
}

/*  STUDENT TO-DO:
 *  Handles Misses, evicting from the cache if necessary.
 *  Fill out the evicted_line_t struct with info regarding the evicted line.
 */
evicted_line_t *handle_miss(cache_t *cache, uword_t addr, operation_t operation, byte_t *incoming_data) {
    evicted_line_t *evicted_line = malloc(sizeof(evicted_line_t));
    evicted_line->data = (byte_t *) calloc(cache->B, sizeof(byte_t));
    cache_line_t *line = select_line(cache, addr);
    unsigned S = cache->C / (cache->A * cache->B);
    unsigned s = _log(S);
    unsigned b = _log(cache->B);
    unsigned set_index = extract_bitfield(addr, b, s);
    unsigned tag = addr >> (b+s);
    
    if (line->valid == true) {
        if (line->dirty == true) {
            dirty_eviction_count++;
            evicted_line->valid = true;
            evicted_line->dirty = true;
        } else {
            clean_eviction_count++;
            evicted_line->dirty = false;
            evicted_line->valid = true;
        }
        evicted_line->addr = (line->tag << (b+s)) | set_index;
        memcpy(evicted_line->data, line->data, cache->B);
    }
    else {
        evicted_line->valid = false;
        evicted_line->dirty = false;
    }
    line->tag = tag;
    line->valid = true;
    if (operation == WRITE) {
        line->dirty = true;

    }
    else {
        line->dirty = false;
    }

    line->lru = next_lru;
    next_lru++;
    if (incoming_data) {
        memcpy(line->data, incoming_data, cache->B);
    }
    return evicted_line;
}

/* STUDENT TO-DO:
 * Get 8 bytes from the cache and write it to dest.
 * Preconditon: addr is contained within the cache.
 */
void get_word_cache(cache_t *cache, uword_t addr, word_t *dest) {
    unsigned S = cache->C / (cache->A * cache->B);
    unsigned s = _log(S);
    unsigned b = _log(cache->B);
    unsigned set_index = extract_bitfield(addr, b, s);
    unsigned tag = addr >> (b+s);
    unsigned offset = addr & ((1 << b) - 1);
    cache_set_t *set = &cache->sets[set_index];
    for (size_t i = 0; i < cache->A; i++) {
        if (set->lines[i].valid == true && set->lines[i].tag == tag) {
            memcpy(dest, set->lines[i].data + offset, sizeof(word_t));
            return;
        }
    }
}

/* STUDENT TO-DO:
 * Set 8 bytes in the cache to val at pos.
 * Preconditon: addr is contained within the cache.
 */
void set_word_cache(cache_t *cache, uword_t addr, word_t val) {
    // Student TODO
    unsigned S = cache->C / (cache->A * cache->B);
    unsigned s = _log(S);
    unsigned b = _log(cache->B);
    unsigned set_index = extract_bitfield(addr, b, s);
    unsigned offset = extract_bitfield(addr, 0, b);
    unsigned tag = addr >> (b+s);
    cache_set_t *set = &cache->sets[set_index];
    for (size_t i = 0; i < cache->A; i++) {
        if (set->lines[i].valid == true && set->lines[i].tag == tag) {
            memcpy(set->lines[i].data + offset, &val, sizeof(val));
            return;
        }
    }
}

/*
 * Access data at memory address addr
 * If it is already in cache, increase hit_count
 * If it is not in cache, bring it in cache, increase miss count
 * Also increase eviction_count if a line is evicted
 *
 * Called by cache-runner; no need to modify it if you implement
 * check_hit() and handle_miss()
 */
void access_data(cache_t *cache, uword_t addr, operation_t operation)
{
    if(!check_hit(cache, addr, operation))
        free(handle_miss(cache, addr, operation, NULL));
}

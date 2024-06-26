#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int s, E, b, t;
int m = 64;
int S, B;
int hits, misses, evictions;
int current_index;

struct block {
    int index;
    int valid;
    long long tag;
};

void process_instruction(struct block** sets, long long t_bits, long long s_bits, long long b_bits) {
    struct block* current_set = sets[s_bits];

    int free_spot = -1;
    for (int i = 0; i < E; i++) {
        if (current_set[i].valid && current_set[i].tag == t_bits) {
            current_set[i].index = current_index;
            hits++;
            return;
        }
        if (!current_set[i].valid) {
            free_spot = i;
        }
    }
    misses++;

    // don't have to evict
    if (free_spot != -1) {
        current_set[free_spot].valid = 1;
        current_set[free_spot].tag = t_bits;
        current_set[free_spot].index = current_index;
        return;
    }

    // have to evict
    int min_index = current_index + 1;
    int min_index_pos = 0;
    for (int i = 0; i < E; i++) {
        if (current_set[i].index < min_index) {
            min_index = current_set[i].index;
            min_index_pos = i;
        }
    }
    current_set[min_index_pos].tag = t_bits;
    current_set[min_index_pos].index = current_index;
    evictions++;
}

long long hex_to_dec(char* address) {
    int len = strlen(address);
    long long total = 0;
    for (int i = 0; i < len; i++) {
        int power = len - i - 1;
        char c = address[i];
        long long add;
        if (c >= '0' && c <= '9') {
            add = c - '0';
        } else {
            add = c - 'a' + 10;
        }
        total += (add * (1LL << (4 * power)));
    }
    return total;
}

void process_address(char* address, long long* t_bits, long long* s_bits, long long* b_bits) {
    long long total = hex_to_dec(address);
    *t_bits = (total >> (s + b)) & ~((1LL << (m - 1)) >> (m - t - 1));
    *s_bits = (total >> b) & ~((1LL << (m - 1)) >> (m - s - 1));
    *b_bits = total & ~((1LL << (m - 1)) >> (m - b - 1));
}

int main(int argc, char* argv[])
{
    char* file_name;
    for (int i = 1; i < argc; i += 2) {
        if (argv[i][1] == 's') {
            s = atoi(argv[i + 1]);
        } else if (argv[i][1] == 'E') {
            E = atoi(argv[i + 1]);
        } else if (argv[i][1] == 'b') {
            b = atoi(argv[i + 1]);
        } else {
            file_name = argv[i + 1];
        }
    }

    t = m - (s + b);
    S = 1 << s;
    B = 1 << b;
    struct block** sets = (struct block**)malloc(S * sizeof(struct block*));
    for (int i = 0; i < S; i++) {
        sets[i] = (struct block*)malloc(E * sizeof(struct block));
        for (int j = 0; j < E; j++) {
            sets[i][j].index = -1;
            sets[i][j].valid = 0;
            sets[i][j].tag = 0;
        }
    }

    FILE* file = fopen(file_name, "r");
    char instruction;
    char address[16];
    int size;

    hits = misses = evictions = 0;
    current_index = 0;

    while (fscanf(file, " %c %[^,],%d\n", &instruction, address, &size) > 1) {
        if (instruction != 'I') {
            long long t_bits, s_bits, b_bits;
            process_address(address, &t_bits, &s_bits, &b_bits);
            process_instruction(sets, t_bits, s_bits, b_bits);
            if (instruction == 'M') {
                process_instruction(sets, t_bits, s_bits, b_bits);
            }
            current_index++;
        }
    }
    fclose(file);

    printSummary(hits, misses, evictions);

    for (int i = 0; i < S; i++) {
        free(sets[i]);
    }
    free(sets);
    
    return 0;
}
#define _CRT_SECURE_NO_WARNINGS =
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>

#define MAX_WORD 100
#define HASH_SIZE 50000

struct Node {
    char word[MAX_WORD];
    int count;
    struct Node* next;
};

struct Node* hash_table[HASH_SIZE] = { 0 };

// Hash function
unsigned int hash(const char* str) {
    unsigned int h = 0;
    while (*str)
        h = (h * 131) + *str++;
    return h % HASH_SIZE;
}

// œ«·…  » ”„Õ »≈÷«›… ⁄œœ „⁄Ì‰
void insert_word_with_count(struct Node** table, const char* word, int count_val) {
    unsigned int h = hash(word);
    struct Node* n = table[h];

    while (n != NULL) {
        if (strcmp(n->word, word) == 0) {
            n->count += count_val;
            return;
        }
        n = n->next;
    }

    struct Node* newn = (struct Node*)malloc(sizeof(struct Node));
    if (!newn) { printf("Memory allocation failed!\n"); exit(1); }

    strcpy(newn->word, word);
    newn->count = count_val;
    newn->next = table[h];
    table[h] = newn;
}

// «·œ«·… «·⁄«œÌ…
void insert_word(struct Node** table, const char* word) {
    insert_word_with_count(table, word, 1);
}

// Clean a word
void clean_word(char* word) {
    int i, j = 0;
    for (i = 0; word[i]; i++)
        if (isalpha((unsigned char)word[i]))
            word[j++] = tolower((unsigned char)word[i]);
    word[j] = '\0';
}

// Serial word count
void wordcount_serial(char* text, struct Node** table) {
    char* token = strtok(text, " ");
    char cleaned[MAX_WORD];

    while (token != NULL) {
        strcpy(cleaned, token);
        clean_word(cleaned);
        if (strlen(cleaned) > 0)
            insert_word(table, cleaned);
        token = strtok(NULL, " ");
    }
}

// Parallel word count ( „ ≈’·«Õ  ⁄—Ì› «·„ €Ì—«  Â‰«)
void wordcount_parallel(char** words, long n, struct Node** table, int threads) {
    omp_set_num_threads(threads);

#pragma omp parallel
    {
        struct Node* private_table[HASH_SIZE] = { 0 };
        long i; // <--- ·«“„ ‰⁄—› «·„ €Ì— Â‰« »—Â «··Ê» ⁄‘«‰ Visual Studio

#pragma omp for
        for (i = 0; i < n; i++) {
            char cleaned[MAX_WORD];
            strcpy(cleaned, words[i]);
            clean_word(cleaned);
            if (strlen(cleaned) > 0)
                insert_word(private_table, cleaned);
        }

#pragma omp critical
        {
            int h; // <--- ÊÂ‰« ﬂ„«‰ »—Â «··Ê»
            for (h = 0; h < HASH_SIZE; h++) {
                struct Node* n = private_table[h];
                while (n) {
                    insert_word_with_count(table, n->word, n->count);
                    n = n->next;
                }
            }
        }
    }
}

// Read file safely
char* read_file(const char* path, long* file_size) {
    FILE* f = fopen(path, "r");
    if (!f) {
        printf("File not found: %s\n", path);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    *file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buffer = (char*)malloc(*file_size + 1);
    if (!buffer) { printf("Memory allocation failed!\n"); exit(1); }

    fread(buffer, 1, *file_size, f);
    buffer[*file_size] = '\0';
    fclose(f);

    return buffer;
}

// Split text into words array
char** split_words(char* text, long* count) {
    long cap = 500000;
    char** arr = (char**)malloc(cap * sizeof(char*));
    if (!arr) { printf("Memory allocation failed!\n"); exit(1); }
    *count = 0;

    char* token = strtok(text, " ");
    while (token) {
        if (*count >= cap) {
            cap *= 2;
            arr = (char**)realloc(arr, cap * sizeof(char*));
            if (!arr) { printf("Memory allocation failed!\n"); exit(1); }
        }
        arr[*count] = token;
        (*count)++;
        token = strtok(NULL, " ");
    }
    return arr;
}

// Compare two hash tables
int tables_match(struct Node** t1, struct Node** t2) {
    int h; //  ⁄—Ì› «·„ €Ì— »—Â ⁄‘«‰ ‰»ﬁÏ ›Ì «·”·Ì„
    for (h = 0; h < HASH_SIZE; h++) {
        struct Node* a = t1[h];
        while (a) {
            unsigned int hb = hash(a->word);
            struct Node* b = t2[hb];

            int found = 0;
            while (b) {
                if (strcmp(a->word, b->word) == 0 &&
                    a->count == b->count) {
                    found = 1;
                    break;
                }
                b = b->next;
            }
            if (!found) return 0;
            a = a->next;
        }
    }
    return 1;
}

int main() {
    long size;
    //  √ﬂœ ≈‰ „·› «· ﬂ”  „ÊÃÊœ Ã‰» „·› «·ﬂÊœ
    char* text = read_file("wordcount_sample_2MB.txt", &size);

    char* text_serial = (char*)malloc(size + 1);
    char* text_parallel = (char*)malloc(size + 1);
    if (!text_serial || !text_parallel) { printf("Memory allocation failed!\n"); exit(1); }

    strcpy(text_serial, text);
    strcpy(text_parallel, text);

    long nwords;
    char** words = split_words(text_parallel, &nwords);

    printf("Processing... (Make sure file is large enough)\n");
    printf("Words: %ld\n", nwords);

    double t0 = omp_get_wtime();
    wordcount_serial(text_serial, hash_table);
    double t_serial = omp_get_wtime() - t0;

    int threads = omp_get_max_threads();
    struct Node* table_parallel[HASH_SIZE] = { 0 };

    double tp0 = omp_get_wtime();
    wordcount_parallel(words, nwords, table_parallel, threads);
    double t_parallel = omp_get_wtime() - tp0;

    int match = tables_match(hash_table, table_parallel);

    double speedup = t_serial / t_parallel;
    double efficiency = speedup / threads;

    printf("\n----- RESULTS -----\n");
    printf("Serial Time:   %.4f s\n", t_serial);
    printf("Parallel Time: %.4f s (%d threads)\n", t_parallel, threads);
    printf("Speedup:       %.2f\n", speedup);
    printf("Efficiency:    %.2f\n", efficiency);
    printf("Correctness:   %s\n", match ? "OK" : "Mismatch!");

    return 0;
}
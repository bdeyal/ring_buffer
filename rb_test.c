#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

#include "ring_buffer.h"

#define RB_SIZE 101

void test_full_empty()
{
    unsigned int i = 0, data;

    struct ring_buffer* rb = ring_buffer_create(RB_SIZE, sizeof(i));

    while (!ring_buffer_full(rb)) {
        ring_buffer_put(rb, &i);
        printf("Added %lu, Size now: %lu\n",
               (unsigned long)i,
               (unsigned long)ring_buffer_size(rb));
        i++;
    }

    while (!ring_buffer_empty(rb)) {
        ring_buffer_get(rb, &data);
        printf("Data read %lu\n", (unsigned long)data);
    }

    ring_buffer_destroy(rb);
}

#if defined (HAVE_INT128)
typedef __uint128_t rb_uint_t;
#else
typedef uint64_t rb_uint_t;
#endif

void test_continuous()
{
    /* see above typedef */
    rb_uint_t i = 0;

    struct ring_buffer* rb = ring_buffer_create(RB_SIZE, sizeof(i));

    puts("Test Continuous");

    for (i = 0; i < 1000000; ++i) {
        rb_uint_t j=i, k=i+1, l=i+2, m=i+3, n=i+4, o=i+5, p=i+6, tmp;

        ring_buffer_put(rb, &j);
        ring_buffer_put(rb, &k);
        ring_buffer_put(rb, &l);
        ring_buffer_put(rb, &m);
        ring_buffer_put(rb, &n);
        ring_buffer_put(rb, &o);
        ring_buffer_put(rb, &p);

        ring_buffer_get(rb, &tmp);
        assert(tmp == j);

        ring_buffer_get(rb, &tmp);
        assert(tmp == k);

        ring_buffer_get(rb, &tmp);
        assert(tmp == l);

        ring_buffer_get(rb, &tmp);
        assert(tmp == m);

        ring_buffer_get(rb, &tmp);
        assert(tmp == n);

        ring_buffer_get(rb, &tmp);
        assert(tmp == o);

        ring_buffer_get(rb, &tmp);
        assert(tmp == p);
    }

    ring_buffer_destroy(rb);
}

/*
 *  A more real-life test
 *
 *  1. open an existing file for read
 *  2. create a new file for write
 *  3. read a random number of bytes from (1)
 *  4. put bytes in a ring buffer
 *  5. get another random number of bytes from ring buffer
 *  6. write those bytes to new file
 *  7. repeat 3-6 until end of input.
 *  8. close files and ring buffer
 *  9. compare two files. Must be identical.
 *
 */
void test_file_copy()
{
    FILE* fin;
    FILE* fout;
    char c;
    int num_to_put, num_to_get, index, rc;
    const char* fname_in;
    const char* fname_out;
    struct ring_buffer* rb;
    char diff_cmd[FILENAME_MAX];

    srand(time(NULL));

    rb = ring_buffer_create(RB_SIZE, sizeof(char));
    assert(rb != NULL);

    fname_in = "/usr/share/dict/words";
    fname_out = "/tmp/words_copy";

    fin = fopen(fname_in, "r");
    assert(fin != NULL);

    fout = fopen(fname_out, "w");
    assert(fout != NULL);

    for (;;) {
        num_to_put = 1 + (rand() % RB_SIZE);
        num_to_get = 1 + (rand() % RB_SIZE);

        for (index = 0; index < num_to_put; ++index) {
            if (ring_buffer_full(rb))
                break;

            if ((c = fgetc(fin)) == EOF)
                break;

            ring_buffer_put(rb, &c);
        }

        for (index = 0; index < num_to_get; ++index) {
            if (ring_buffer_empty(rb))
                break;

            ring_buffer_get(rb, &c);

            fputc(c, fout);
        }

        if (feof(fin) && ring_buffer_empty(rb))
            break;
    }

    fclose(fin);
    fclose(fout);
    ring_buffer_destroy(rb);

    snprintf(diff_cmd, sizeof diff_cmd, "diff -s \"%s\" \"%s\"", fname_in, fname_out);
    puts(diff_cmd);
    rc = system(diff_cmd);
    assert(rc == 0);
}


int main()
{
    printf("Sizeof rb = %lu\n", sizeof(struct ring_buffer));
    test_full_empty();
    test_continuous();
    test_file_copy();
    return 0;
}

#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "check.h"
#include "Framer.h"

#ifdef LENGTH_FRAMER
typedef struct {
    const uint8_t *wire;
    size_t length, position, fail_at;
    unsigned failures;
    int error;
} FaultInput;

static ssize_t fault_read(void *cookie, char *buffer, size_t size)
{
    FaultInput *input = cookie;
    if (size == 0) return 0;
    if (input->position == input->fail_at && input->failures != 0) {
        --input->failures;
        errno = input->error;
        return -1;
    }
    if (input->position == input->length) return 0;
    /* One byte per callback makes both prefix positions independently testable. */
    buffer[0] = (char)input->wire[input->position++];
    return 1;
}

#ifdef __APPLE__
static int fault_read_macos(void *cookie, char *buffer, int size)
{
    return (int)fault_read(cookie, buffer, (size_t)size);
}
#endif

static FILE *fault_stream(FaultInput *input)
{
#ifdef __APPLE__
    FILE *stream = funopen(input, fault_read_macos, NULL, NULL, NULL);
#else
    cookie_io_functions_t functions = {.read = fault_read};
    FILE *stream = fopencookie(input, "r", functions);
#endif
    CHECK(stream != NULL);
    CHECK(setvbuf(stream, NULL, _IONBF, 0) == 0);
    return stream;
}

static void check_interrupted_frames(void)
{
    const uint8_t wire[] = {0, 3, 'a', 0, 'b', 0, 0, 0, 2, 'o', 'k'};
    const size_t offsets[] = {0, 1, 2, 5, 6, 7, 8};
    uint8_t buffer[8];
    for (size_t i = 0; i < sizeof offsets / sizeof offsets[0]; ++i) {
        FaultInput input = {wire, sizeof wire, 0, offsets[i], 3, EINTR};
        FILE *stream = fault_stream(&input);
        CHECK(GetNextMsg(stream, buffer, sizeof buffer) == 3 && memcmp(buffer, wire + 2, 3) == 0);
        CHECK(GetNextMsg(stream, buffer, sizeof buffer) == 0);
        CHECK(GetNextMsg(stream, buffer, sizeof buffer) == 2 && memcmp(buffer, "ok", 2) == 0);
        CHECK(input.failures == 0 && !ferror(stream));
        CHECK(GetNextMsg(stream, buffer, sizeof buffer) == -1 && feof(stream) && !ferror(stream));
        CHECK(fclose(stream) == 0);
    }

    const uint8_t oversized[] = {0, 5, 'a', 'b', 'c', 'd', 'e', 0, 2, 'o', 'k'};
    const size_t drain_offsets[] = {1, 4};
    for (size_t i = 0; i < sizeof drain_offsets / sizeof drain_offsets[0]; ++i) {
        FaultInput input = {oversized, sizeof oversized, 0, drain_offsets[i], 3, EINTR};
        FILE *stream = fault_stream(&input);
        CHECK(GetNextMsg(stream, buffer, 2) == -5 && memcmp(buffer, "ab", 2) == 0);
        CHECK(GetNextMsg(stream, buffer, 2) == 2 && memcmp(buffer, "ok", 2) == 0);
        CHECK(input.failures == 0 && !ferror(stream));
        CHECK(fclose(stream) == 0);
    }
}

static void check_header_errors(void)
{
    const uint8_t wire[] = {0, 1, 'x'};
    const int errors[] = {EIO, EAGAIN};
    uint8_t buffer[8];
    for (size_t i = 0; i < sizeof errors / sizeof errors[0]; ++i) {
        for (size_t offset = 0; offset < 2; ++offset) {
            FaultInput input = {wire, sizeof wire, 0, offset, 1, errors[i]};
            FILE *stream = fault_stream(&input);
            errno = 0;
            CHECK(GetNextMsg(stream, buffer, sizeof buffer) == -1 && errno == errors[i]);
            CHECK(input.failures == 0 && ferror(stream) && !feof(stream));
            CHECK(fclose(stream) == 0);
        }
    }
    for (size_t length = 0; length < 2; ++length) {
        FaultInput input = {wire, length, 0, 0, 0, 0};
        FILE *stream = fault_stream(&input);
        errno = 0;
        CHECK(GetNextMsg(stream, buffer, sizeof buffer) == -1);
        CHECK(feof(stream) && !ferror(stream));
        if (length == 1) CHECK(errno == EPROTO);
        CHECK(fclose(stream) == 0);
    }
}
#endif

int main(void)
{
    FILE *stream = tmpfile(); CHECK(stream != NULL);
    uint8_t message[] = {'a', 0, 'b'}, buffer[8] = {0};
    CHECK(PutMsg(message, sizeof message, stream) == 3);
    CHECK(PutMsg(NULL, 0, stream) == 0);
    CHECK(fseek(stream, 0, SEEK_SET) == 0);
    CHECK(GetNextMsg(stream, buffer, sizeof buffer) == 3 && memcmp(buffer, message, 3) == 0);
    CHECK(GetNextMsg(stream, buffer, sizeof buffer) == 0);
    CHECK(GetNextMsg(stream, buffer, sizeof buffer) == -1);
    fclose(stream);

    stream = tmpfile(); CHECK(stream != NULL);
    uint8_t *large = malloc(60000); CHECK(large != NULL); memset(large, 'A', 60000);
    CHECK(PutMsg(large, 60000, stream) == 60000);
    CHECK(PutMsg((uint8_t *)"ok", 2, stream) == 2);
    CHECK(fseek(stream, 0, SEEK_SET) == 0);
    CHECK(GetNextMsg(stream, buffer, 0) == -1 && errno == EINVAL);
    CHECK(ftell(stream) == 0);
    CHECK(GetNextMsg(stream, buffer, sizeof buffer) == -60000);
    CHECK(GetNextMsg(stream, buffer, sizeof buffer) == 2 && memcmp(buffer, "ok", 2) == 0);
    fclose(stream); free(large);

    stream = tmpfile(); CHECK(stream != NULL);
#ifdef LENGTH_FRAMER
    const uint8_t broken[] = {0, 5, 'x', 'y'};
    CHECK(fwrite(broken, 1, sizeof broken, stream) == sizeof broken);
#else
    CHECK(fwrite("broken", 1, 6, stream) == 6);
#endif
    CHECK(fseek(stream, 0, SEEK_SET) == 0);
    CHECK(GetNextMsg(stream, buffer, sizeof buffer) == -1 && errno == EPROTO);
    fclose(stream);

    stream = tmpfile(); CHECK(stream != NULL);
    CHECK(PutMsg(message, (size_t)UINT16_MAX + 1, stream) == -1);
    CHECK(ftell(stream) == 0);
#ifndef LENGTH_FRAMER
    CHECK(PutMsg((uint8_t *)"a\nb", 3, stream) == -1 && ftell(stream) == 0);
#endif
    fclose(stream);
    stream = fopen("/dev/null", "r"); CHECK(stream != NULL);
    CHECK(PutMsg(message, sizeof message, stream) == -1);
    fclose(stream);
#ifdef LENGTH_FRAMER
    check_interrupted_frames();
    check_header_errors();
#endif
    return EXIT_SUCCESS;
}

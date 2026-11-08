#include <errno.h>
#include <string.h>
#include "check.h"
#include "Framer.h"

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
    return EXIT_SUCCESS;
}

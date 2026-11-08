#include <errno.h>
#include <string.h>
#include "check.h"
#include "VoteEncoding.h"

static void expect_invalid(const uint8_t *bytes, size_t length)
{
    VoteInfo sentinel = {UINT64_C(123456), 17, true, true};
    CHECK(!Decode(bytes, length, &sentinel));
    CHECK(sentinel.count == 123456 && sentinel.condidate == 17 && sentinel.isInquiry && sentinel.isResponse);
}

int main(void)
{
    const uint64_t counts[] = {0, 1, UINT64_C(0xffffffff), UINT64_C(0x100000000), UINT64_MAX};
    for (int inquiry = 0; inquiry <= 1; ++inquiry) for (int response = 0; response <= 1; ++response) {
        for (size_t i = 0; i < sizeof counts / sizeof counts[0]; ++i) {
            VoteInfo value = {counts[i], 23, inquiry != 0, response != 0}, decoded = {0};
            uint8_t guarded[128], original[128]; memset(guarded, 0xa5, sizeof guarded);
            size_t length = Encode(&value, guarded + 1, sizeof guarded - 2);
            CHECK(length > 0 && guarded[0] == 0xa5 && guarded[length + 1] == 0xa5);
            memcpy(original, guarded, sizeof guarded);
            CHECK(Decode(guarded + 1, length, &decoded));
            CHECK(memcmp(original, guarded, sizeof guarded) == 0);
            CHECK(decoded.condidate == value.condidate && decoded.isInquiry == value.isInquiry && decoded.isResponse == value.isResponse);
            CHECK(decoded.count == (response ? value.count : 0));
            CHECK(Encode(&value, guarded + 1, length) == length);
            CHECK(Encode(&value, guarded + 1, length - 1) == 0 && errno == EMSGSIZE);
        }
    }
    VoteInfo invalid = {0, MAX_CANDIDATE + 1, false, false};
    uint8_t buffer[128]; CHECK(Encode(&invalid, buffer, sizeof buffer) == 0);
    expect_invalid(NULL, 0);
#ifdef BINARY_CODEC
    VoteInfo value = {UINT64_C(0xfedcba9876543210), 23, false, true};
    const uint8_t expected[] = {0x56, 0, 0, 23, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};
    CHECK(Encode(&value, buffer, sizeof buffer) == sizeof expected && memcmp(buffer, expected, sizeof expected) == 0);
    expect_invalid(expected, sizeof expected - 1);
    memcpy(buffer, expected, sizeof expected); buffer[1] = 1; expect_invalid(buffer, sizeof expected);
    const uint8_t bad_candidate[] = {0x54, 0, 0xff, 0xff}; expect_invalid(bad_candidate, sizeof bad_candidate);
    const uint8_t extra[] = {0x54, 0, 0, 1, 0}; expect_invalid(extra, sizeof extra);
#else
    const char *invalid_messages[] = {"Voting s 1", "Voting v -1", "Voting i 1001", "Voting v 1 extra",
        "Voting v R 1 18446744073709551616", "Voting v R 1 -1", "Voting v R 1", "Wrong v 1", ""};
    for (size_t i = 0; i < sizeof invalid_messages / sizeof invalid_messages[0]; ++i)
        expect_invalid((const uint8_t *)invalid_messages[i], strlen(invalid_messages[i]));
    const uint8_t embedded[] = {'V', 0, 'x'}; expect_invalid(embedded, sizeof embedded);
    const uint8_t bounded[] = {'V','o','t','i','n','g',' ','i',' ','1'};
    VoteInfo value = {0}; CHECK(Decode(bounded, sizeof bounded, &value) && value.isInquiry);
#endif
    return EXIT_SUCCESS;
}

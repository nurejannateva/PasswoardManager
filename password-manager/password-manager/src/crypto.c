#include <stdio.h>
#include <string.h>

#include "password_manager/app.h"

char *hash_text(const char *text)
{
    return g_compute_checksum_for_string(G_CHECKSUM_SHA256, text, -1);
}

static void xor_with_pin_stream(guchar *data,
                                gsize len,
                                const char *pin,
                                const char *purpose)
{
    guint counter = 0;
    gsize offset = 0;

    while (offset < len)
    {
        char seed[256];
        char *digest;

        g_snprintf(seed, sizeof(seed), "%s:%s:%u", pin, purpose, counter++);
        digest = hash_text(seed);

        for (int i = 0; i < 64 && offset < len; i += 2)
        {
            guint byte = 0;
            sscanf(digest + i, "%2x", &byte);
            data[offset++] ^= (guchar)byte;
        }

        g_free(digest);
    }
}

char *encode_field(const char *value, const char *purpose)
{
    gsize len = strlen(value);
    guchar *buffer = g_memdup2(value, len);
    char *encoded;

    xor_with_pin_stream(buffer, len, current_pin, purpose);
    encoded = g_base64_encode(buffer, len);
    g_free(buffer);

    return encoded;
}

char *decode_field(const char *value, const char *purpose)
{
    gsize len = 0;
    guchar *buffer = g_base64_decode(value, &len);
    char *decoded;

    xor_with_pin_stream(buffer, len, current_pin, purpose);
    decoded = g_strndup((const char *)buffer, len);
    g_free(buffer);

    return decoded;
}

#include <stdio.h>
#include <string.h>

#include <glib/gstdio.h>

#include "password_manager/app.h"

static void ensure_pin_folder(void)
{
    char *folder = g_path_get_dirname(PIN_FILE);

    g_mkdir_with_parents(folder, 0700);
    g_free(folder);
}

void load_pin(void)
{
    char *contents = NULL;

    if (g_file_get_contents(PIN_FILE, &contents, NULL, NULL))
    {
        g_strstrip(contents);
        if (*contents)
        {
            saved_pin = contents;
            return;
        }
        g_free(contents);
    }

    saved_pin = g_strdup(DEFAULT_PIN);
    save_pin();
}

void save_pin(void)
{
    if (saved_pin)
    {
        ensure_pin_folder();
        g_file_set_contents(PIN_FILE, saved_pin, -1, NULL);
    }
}

gboolean verify_pin(const char *pin)
{
    return g_strcmp0(pin, saved_pin) == 0;
}

void load_credentials(void)
{
    if (credentials)
        g_ptr_array_free(credentials, TRUE);

    credentials = g_ptr_array_new_with_free_func(credential_free);

    FILE *f = fopen(DATA_FILE, "r");
    if (!f)
        return;

    char line[512];

    while (fgets(line, sizeof(line), f))
    {
        Credential *c = g_new0(Credential, 1);
        g_strchomp(line);

        char *p = strtok(line, "|");
        if (!p)
        {
            g_free(c);
            continue;
        }

        gboolean encrypted = g_strcmp0(p, "v1") == 0;

        if (encrypted)
        {
            p = strtok(NULL, "|");
            if (!p || !current_pin)
            {
                credential_free(c);
                continue;
            }
            c->platform = decode_field(p, "platform");
        }
        else
        {
            c->platform = g_strdup(p);
        }

        p = strtok(NULL, "|");
        if (!p)
        {
            credential_free(c);
            continue;
        }
        c->username = encrypted ? decode_field(p, "username") : g_strdup(p);

        p = strtok(NULL, "|\n");
        if (!p)
        {
            credential_free(c);
            continue;
        }
        c->password = encrypted ? decode_field(p, "password") : g_strdup(p);

        g_ptr_array_add(credentials, c);
    }

    fclose(f);
}

void save_credentials(void)
{
    if (!current_pin)
        return;

    FILE *f = fopen(DATA_FILE, "w");
    if (!f)
        return;

    for (guint i = 0; i < credentials->len; i++)
    {
        Credential *c = g_ptr_array_index(credentials, i);
        char *platform = encode_field(c->platform, "platform");
        char *username = encode_field(c->username, "username");
        char *password = encode_field(c->password, "password");

        fprintf(f, "v1|%s|%s|%s\n", platform, username, password);

        g_free(platform);
        g_free(username);
        g_free(password);
    }

    fclose(f);
}

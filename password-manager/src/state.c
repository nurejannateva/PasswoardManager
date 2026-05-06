#include "password_manager/app.h"

GtkWidget *stack = NULL;
GtkWidget *pin_entry = NULL;
GtkWidget *old_pin_entry = NULL;
GtkWidget *new_pin_entry = NULL;
GtkWidget *toast_revealer = NULL;
GtkWidget *toast_label = NULL;
GtkWidget *cred_list = NULL;
GtkWidget *search_entry = NULL;
GtkWidget *platform_entry = NULL;
GtkWidget *username_entry = NULL;
GtkWidget *password_entry = NULL;

char *saved_pin = NULL;
char *current_pin = NULL;
guint auto_lock_timeout_id = 0;
guint clipboard_clear_timeout_id = 0;
guint toast_timeout_id = 0;
GPtrArray *credentials = NULL;
int edit_index = -1;

const char *DATA_FILE = "data.txt";
const char *PIN_FILE = "config/pin.txt";
const char *DEFAULT_PIN = "1234";

void credential_free(gpointer data)
{
    Credential *c = data;

    if (!c)
        return;

    g_free(c->platform);
    g_free(c->username);
    g_free(c->password);
    g_free(c);
}

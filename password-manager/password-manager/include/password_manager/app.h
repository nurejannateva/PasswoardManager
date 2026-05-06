#ifndef PASSWORD_MANAGER_APP_H
#define PASSWORD_MANAGER_APP_H

#include <adwaita.h>
#include <gtk/gtk.h>

typedef struct {
    char *platform;
    char *username;
    char *password;
} Credential;

extern GtkWidget *stack;
extern GtkWidget *pin_entry;
extern GtkWidget *old_pin_entry;
extern GtkWidget *new_pin_entry;
extern GtkWidget *toast_revealer;
extern GtkWidget *toast_label;
extern GtkWidget *cred_list;
extern GtkWidget *search_entry;
extern GtkWidget *platform_entry;
extern GtkWidget *username_entry;
extern GtkWidget *password_entry;

extern char *saved_pin;
extern char *current_pin;
extern guint auto_lock_timeout_id;
extern guint clipboard_clear_timeout_id;
extern guint toast_timeout_id;
extern GPtrArray *credentials;
extern int edit_index;

extern const char *DATA_FILE;
extern const char *PIN_FILE;
extern const char *DEFAULT_PIN;

void credential_free(gpointer data);

char *hash_text(const char *text);
char *encode_field(const char *value, const char *purpose);
char *decode_field(const char *value, const char *purpose);

void load_pin(void);
void save_pin(void);
gboolean verify_pin(const char *pin);
void load_credentials(void);
void save_credentials(void);

void load_css(void);
GtkWidget *create_toast_revealer(void);
void show_toast(const char *message);

void setup_visibility_toggle(GtkWidget *entry);
GtkWidget *make_icon_button(const char *icon_name, const char *label);

void refresh_list(void);
void open_editor(int index);
void schedule_auto_lock(void);
void lock_app(void);
gboolean clear_clipboard_cb(gpointer data);

void open_main(GtkWidget *widget, gpointer data);
void go_lock(GtkWidget *widget, gpointer data);
void open_settings(GtkWidget *widget, gpointer data);
void change_pin(GtkWidget *widget, gpointer data);

void on_add_clicked(GtkWidget *btn, gpointer data);
void on_search_changed(GtkEditable *editable, gpointer data);

#endif

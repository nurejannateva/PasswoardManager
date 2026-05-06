#include <string.h>

#include "password_manager/app.h"

#define MODAL_WIDTH 420
#define MODAL_HEIGHT 340

static gboolean credential_matches_search(Credential *c, const char *query);
static void copy_to_clipboard(const char *text, const char *toast);
static void on_copy_clicked(GtkWidget *btn, gpointer data);
static void on_copy_username_clicked(GtkWidget *btn, gpointer data);
static void on_copy_password_clicked(GtkWidget *btn, gpointer data);
static void on_delete_clicked(GtkWidget *btn, gpointer data);
static void on_delete_cancel_clicked(GtkWidget *btn, gpointer data);
static void on_delete_confirm_clicked(GtkWidget *btn, gpointer data);
static void on_edit_clicked(GtkWidget *btn, gpointer data);
static void on_save_clicked(GtkWidget *btn, gpointer data);
static void on_cancel_editor_clicked(GtkWidget *btn, gpointer data);

static gboolean credential_matches_search(Credential *c, const char *query)
{
    gboolean matches;
    char *needle;
    char *platform;
    char *username;

    if (!query || !*query)
        return TRUE;

    needle = g_utf8_casefold(query, -1);
    platform = g_utf8_casefold(c->platform, -1);
    username = g_utf8_casefold(c->username, -1);

    matches = strstr(platform, needle) || strstr(username, needle);

    g_free(needle);
    g_free(platform);
    g_free(username);

    return matches;
}

void refresh_list(void)
{
    GtkWidget *child = gtk_widget_get_first_child(GTK_WIDGET(cred_list));
    const char *query = search_entry
        ? gtk_editable_get_text(GTK_EDITABLE(search_entry))
        : "";
    guint visible_count = 0;

    while (child)
    {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(cred_list), child);
        child = next;
    }

    for (guint i = 0; i < credentials->len; i++)
    {
        Credential *c = g_ptr_array_index(credentials, i);

        if (!credential_matches_search(c, query))
            continue;

        visible_count++;

        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_set_margin_top(box, 7);
        gtk_widget_set_margin_bottom(box, 7);
        gtk_widget_set_margin_start(box, 12);
        gtk_widget_set_margin_end(box, 12);
        gtk_widget_set_hexpand(box, TRUE);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
        gtk_widget_add_css_class(row, "credential-row");

        gtk_widget_set_hexpand(row, TRUE);
        gtk_widget_set_halign(row, GTK_ALIGN_FILL);

        GtkWidget *left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_widget_set_hexpand(left, TRUE);
        gtk_widget_set_halign(left, GTK_ALIGN_FILL);

        GtkWidget *platform = gtk_label_new(c->platform);
        GtkWidget *username = gtk_label_new(c->username);
        gtk_widget_add_css_class(platform, "platform-label");
        gtk_widget_add_css_class(username, "muted");
        gtk_label_set_xalign(GTK_LABEL(platform), 0.0);
        gtk_label_set_xalign(GTK_LABEL(username), 0.0);
        gtk_label_set_ellipsize(GTK_LABEL(platform), PANGO_ELLIPSIZE_END);
        gtk_label_set_ellipsize(GTK_LABEL(username), PANGO_ELLIPSIZE_END);

        gtk_box_append(GTK_BOX(left), platform);
        gtk_box_append(GTK_BOX(left), username);

        GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_halign(actions, GTK_ALIGN_END);
        gtk_widget_set_valign(actions, GTK_ALIGN_CENTER);

        GtkWidget *edit = make_icon_button("document-edit-symbolic", "Edit");
        GtkWidget *copy = make_icon_button("edit-copy-symbolic", "Copy");
        GtkWidget *del = make_icon_button("user-trash-symbolic", "Delete");
        gtk_widget_add_css_class(edit, "small-action");
        gtk_widget_add_css_class(copy, "small-action");
        gtk_widget_add_css_class(del, "small-action");
        gtk_widget_set_hexpand(edit, TRUE);
        gtk_widget_set_hexpand(copy, TRUE);
        gtk_widget_set_hexpand(del, TRUE);

        g_signal_connect(edit, "clicked",
                         G_CALLBACK(on_edit_clicked),
                         GINT_TO_POINTER((int)i));
        g_signal_connect(copy, "clicked",
                         G_CALLBACK(on_copy_clicked),
                         GINT_TO_POINTER((int)i));
        g_signal_connect(del, "clicked",
                         G_CALLBACK(on_delete_clicked),
                         GINT_TO_POINTER((int)i));

        gtk_box_append(GTK_BOX(actions), edit);
        gtk_box_append(GTK_BOX(actions), copy);
        gtk_box_append(GTK_BOX(actions), del);

        gtk_box_append(GTK_BOX(box), left);
        gtk_box_append(GTK_BOX(box), actions);

        gtk_list_box_append(GTK_LIST_BOX(cred_list), row);
    }

    if (visible_count == 0)
    {
        const char *message = credentials->len == 0
            ? "No credentials saved yet"
            : "No credentials match your search";
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *empty = gtk_label_new(message);

        gtk_widget_add_css_class(empty, "muted");
        gtk_widget_set_margin_top(empty, 40);
        gtk_widget_set_margin_bottom(empty, 40);
        gtk_label_set_xalign(GTK_LABEL(empty), 0.5);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), empty);
        gtk_list_box_append(GTK_LIST_BOX(cred_list), row);
    }
}

static void copy_to_clipboard(const char *text, const char *toast)
{
    GdkDisplay *display = gdk_display_get_default();
    GdkClipboard *clipboard = gdk_display_get_clipboard(display);

    gdk_clipboard_set_text(clipboard, text);
    show_toast(toast);

    if (clipboard_clear_timeout_id)
        g_source_remove(clipboard_clear_timeout_id);
    clipboard_clear_timeout_id = g_timeout_add_seconds(30, clear_clipboard_cb, NULL);

    schedule_auto_lock();
}

static void on_copy_clicked(GtkWidget *btn, gpointer data)
{
    int index = GPOINTER_TO_INT(data);
    Credential *c = g_ptr_array_index(credentials, index);
    GtkRoot *root = gtk_widget_get_root(btn);

    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Copy Credential");
    gtk_window_set_default_size(GTK_WINDOW(dialog), MODAL_WIDTH, MODAL_HEIGHT);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(root));

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(content, "modal-dialog");
    gtk_widget_add_css_class(content, "copy-dialog");
    gtk_widget_set_margin_top(content, 22);
    gtk_widget_set_margin_bottom(content, 18);
    gtk_widget_set_margin_start(content, 22);
    gtk_widget_set_margin_end(content, 22);

    GtkWidget *inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_margin_top(inner, 24);
    gtk_widget_set_margin_bottom(inner, 24);
    gtk_widget_set_margin_start(inner, 24);
    gtk_widget_set_margin_end(inner, 24);

    GtkWidget *icon = gtk_image_new_from_icon_name("edit-copy-symbolic");
    gtk_widget_add_css_class(icon, "copy-dialog-icon");
    gtk_widget_set_halign(icon, GTK_ALIGN_CENTER);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 38);

    GtkWidget *title = gtk_label_new("Copy credential");
    gtk_widget_add_css_class(title, "copy-dialog-title");
    gtk_label_set_xalign(GTK_LABEL(title), 0.5);

    GtkWidget *target = gtk_label_new(c->platform);
    gtk_widget_add_css_class(target, "copy-dialog-target");
    gtk_label_set_xalign(GTK_LABEL(target), 0.5);
    gtk_label_set_ellipsize(GTK_LABEL(target), PANGO_ELLIPSIZE_END);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(actions, "copy-dialog-actions");
    gtk_widget_set_margin_top(actions, 8);

    GtkWidget *username = make_icon_button("avatar-default-symbolic", "Copy Username");
    GtkWidget *password = make_icon_button("dialog-password-symbolic", "Copy Password");
    gtk_widget_add_css_class(username, "secondary-button");
    gtk_widget_add_css_class(password, "secondary-button");

    g_signal_connect(username, "clicked",
                     G_CALLBACK(on_copy_username_clicked),
                     GINT_TO_POINTER(index));
    g_signal_connect(password, "clicked",
                     G_CALLBACK(on_copy_password_clicked),
                     GINT_TO_POINTER(index));

    gtk_box_append(GTK_BOX(actions), username);
    gtk_box_append(GTK_BOX(actions), password);
    gtk_box_append(GTK_BOX(inner), icon);
    gtk_box_append(GTK_BOX(inner), title);
    gtk_box_append(GTK_BOX(inner), target);
    gtk_box_append(GTK_BOX(inner), actions);
    gtk_box_append(GTK_BOX(content), inner);

    gtk_window_set_child(GTK_WINDOW(dialog), content);

    gtk_window_present(GTK_WINDOW(dialog));
    schedule_auto_lock();
}

static void on_copy_username_clicked(GtkWidget *btn, gpointer data)
{
    int index = GPOINTER_TO_INT(data);
    Credential *c = g_ptr_array_index(credentials, index);
    GtkRoot *root = gtk_widget_get_root(btn);

    copy_to_clipboard(c->username, "Username copied");
    gtk_window_destroy(GTK_WINDOW(root));
}

static void on_copy_password_clicked(GtkWidget *btn, gpointer data)
{
    int index = GPOINTER_TO_INT(data);
    Credential *c = g_ptr_array_index(credentials, index);
    GtkRoot *root = gtk_widget_get_root(btn);

    copy_to_clipboard(c->password, "Password copied");
    gtk_window_destroy(GTK_WINDOW(root));
}

static void on_delete_clicked(GtkWidget *btn, gpointer data)
{
    int index = GPOINTER_TO_INT(data);
    Credential *c = g_ptr_array_index(credentials, index);
    GtkRoot *root = gtk_widget_get_root(btn);

    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Delete Credential");
    gtk_window_set_default_size(GTK_WINDOW(dialog), MODAL_WIDTH, MODAL_HEIGHT);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(root));

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(content, "modal-dialog");
    gtk_widget_add_css_class(content, "delete-dialog");
    gtk_widget_set_margin_top(content, 22);
    gtk_widget_set_margin_bottom(content, 18);
    gtk_widget_set_margin_start(content, 22);
    gtk_widget_set_margin_end(content, 22);

    GtkWidget *inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_margin_top(inner, 24);
    gtk_widget_set_margin_bottom(inner, 24);
    gtk_widget_set_margin_start(inner, 24);
    gtk_widget_set_margin_end(inner, 24);

    GtkWidget *icon = gtk_image_new_from_icon_name("user-trash-symbolic");
    gtk_widget_add_css_class(icon, "delete-dialog-icon");
    gtk_widget_set_halign(icon, GTK_ALIGN_CENTER);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 42);

    GtkWidget *title = gtk_label_new("Delete this credential?");
    gtk_widget_add_css_class(title, "delete-dialog-title");
    gtk_label_set_xalign(GTK_LABEL(title), 0.5);

    GtkWidget *target = gtk_label_new(c->platform);
    gtk_widget_add_css_class(target, "delete-dialog-target");
    gtk_label_set_xalign(GTK_LABEL(target), 0.5);
    gtk_label_set_ellipsize(GTK_LABEL(target), PANGO_ELLIPSIZE_END);

    GtkWidget *body = gtk_label_new(
        "This will permanently remove the saved username and password from your vault."
    );
    gtk_widget_add_css_class(body, "delete-dialog-body");
    gtk_label_set_wrap(GTK_LABEL(body), TRUE);
    gtk_label_set_justify(GTK_LABEL(body), GTK_JUSTIFY_CENTER);
    gtk_label_set_xalign(GTK_LABEL(body), 0.5);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 24);
    gtk_widget_add_css_class(actions, "delete-dialog-actions");
    gtk_widget_set_halign(actions, GTK_ALIGN_FILL);
    gtk_widget_set_margin_top(actions, 6);

    GtkWidget *cancel = gtk_button_new_with_label("Cancel");
    GtkWidget *delete = gtk_button_new_with_label("Delete");
    gtk_widget_add_css_class(cancel, "secondary-button");
    gtk_widget_add_css_class(delete, "danger-button");
    gtk_widget_set_hexpand(cancel, TRUE);
    gtk_widget_set_hexpand(delete, TRUE);

    g_signal_connect(cancel, "clicked",
                     G_CALLBACK(on_delete_cancel_clicked),
                     NULL);
    g_signal_connect(delete, "clicked",
                     G_CALLBACK(on_delete_confirm_clicked),
                     GINT_TO_POINTER(index));

    gtk_box_append(GTK_BOX(actions), cancel);
    gtk_box_append(GTK_BOX(actions), delete);

    gtk_box_append(GTK_BOX(inner), icon);
    gtk_box_append(GTK_BOX(inner), title);
    gtk_box_append(GTK_BOX(inner), target);
    gtk_box_append(GTK_BOX(inner), body);
    gtk_box_append(GTK_BOX(inner), actions);
    gtk_box_append(GTK_BOX(content), inner);

    gtk_window_set_child(GTK_WINDOW(dialog), content);

    gtk_window_present(GTK_WINDOW(dialog));
    schedule_auto_lock();
}

static void on_delete_cancel_clicked(GtkWidget *btn, gpointer data)
{
    (void)data;

    GtkRoot *win = gtk_widget_get_root(btn);
    gtk_window_destroy(GTK_WINDOW(win));
    schedule_auto_lock();
}

static void on_delete_confirm_clicked(GtkWidget *btn, gpointer data)
{
    int index = GPOINTER_TO_INT(data);

    g_ptr_array_remove_index(credentials, index);
    save_credentials();
    refresh_list();
    show_toast("Credential deleted");
    schedule_auto_lock();

    GtkRoot *win = gtk_widget_get_root(btn);
    gtk_window_destroy(GTK_WINDOW(win));
}

static void on_save_clicked(GtkWidget *btn, gpointer data)
{
    (void)data;

    if (!platform_entry || !username_entry || !password_entry)
        return;

    const char *p = gtk_editable_get_text(GTK_EDITABLE(platform_entry));
    const char *u = gtk_editable_get_text(GTK_EDITABLE(username_entry));
    const char *pw = gtk_editable_get_text(GTK_EDITABLE(password_entry));

    if (!*p || !*u || !*pw)
    {
        show_toast("Fill in platform, username, and password");
        return;
    }

    if (edit_index == -1)
    {
        Credential *c = g_new0(Credential, 1);
        c->platform = g_strdup(p);
        c->username = g_strdup(u);
        c->password = g_strdup(pw);
        g_ptr_array_add(credentials, c);
    }
    else
    {
        Credential *c = g_ptr_array_index(credentials, edit_index);

        g_free(c->platform);
        g_free(c->username);
        g_free(c->password);

        c->platform = g_strdup(p);
        c->username = g_strdup(u);
        c->password = g_strdup(pw);
    }

    save_credentials();
    refresh_list();
    show_toast(edit_index == -1 ? "Credential added" : "Credential updated");
    schedule_auto_lock();

    GtkRoot *win = gtk_widget_get_root(GTK_WIDGET(btn));
    gtk_window_destroy(GTK_WINDOW(win));
}

static void on_cancel_editor_clicked(GtkWidget *btn, gpointer data)
{
    (void)data;

    GtkRoot *win = gtk_widget_get_root(GTK_WIDGET(btn));
    gtk_window_destroy(GTK_WINDOW(win));
}

void on_add_clicked(GtkWidget *btn, gpointer data)
{
    (void)btn;
    (void)data;

    open_editor(-1);
}

static void on_edit_clicked(GtkWidget *btn, gpointer data)
{
    (void)btn;

    int index = GPOINTER_TO_INT(data);
    open_editor(index);
}

void open_editor(int index)
{
    edit_index = index;
    schedule_auto_lock();

    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog),
                         index == -1 ? "Add Credential" : "Edit Credential");
    gtk_window_set_default_size(GTK_WINDOW(dialog), MODAL_WIDTH, MODAL_HEIGHT);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                 GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(stack))));

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(content, "modal-dialog");
    gtk_widget_add_css_class(content, "dialog-card");
    gtk_widget_set_margin_top(content, 22);
    gtk_widget_set_margin_bottom(content, 18);
    gtk_widget_set_margin_start(content, 22);
    gtk_widget_set_margin_end(content, 22);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);

    GtkWidget *form_title = gtk_label_new(index == -1 ? "Add Credential" : "Edit Credential");
    gtk_widget_add_css_class(form_title, "section-title");
    gtk_label_set_xalign(GTK_LABEL(form_title), 0.0);

    platform_entry = gtk_entry_new();
    username_entry = gtk_entry_new();
    password_entry = gtk_entry_new();

    gtk_entry_set_placeholder_text(GTK_ENTRY(platform_entry), "Platform");
    gtk_entry_set_placeholder_text(GTK_ENTRY(username_entry), "Username");
    gtk_entry_set_placeholder_text(GTK_ENTRY(password_entry), "Password");
    setup_visibility_toggle(password_entry);

    if (index != -1)
    {
        Credential *c = g_ptr_array_index(credentials, index);

        gtk_editable_set_text(GTK_EDITABLE(platform_entry), c->platform);
        gtk_editable_set_text(GTK_EDITABLE(username_entry), c->username);
        gtk_editable_set_text(GTK_EDITABLE(password_entry), c->password);
    }

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 24);
    gtk_widget_set_halign(actions, GTK_ALIGN_FILL);
    gtk_widget_set_margin_top(actions, 6);

    GtkWidget *cancel = make_icon_button("window-close-symbolic", "Cancel");
    GtkWidget *save = make_icon_button("document-save-symbolic", "Save");
    gtk_widget_add_css_class(cancel, "secondary-button");
    gtk_widget_set_hexpand(cancel, TRUE);
    gtk_widget_set_hexpand(save, TRUE);

    g_signal_connect(save, "clicked",
                     G_CALLBACK(on_save_clicked),
                     NULL);
    g_signal_connect(cancel, "clicked",
                     G_CALLBACK(on_cancel_editor_clicked),
                     NULL);

    gtk_box_append(GTK_BOX(actions), cancel);
    gtk_box_append(GTK_BOX(actions), save);

    GtkWidget *platform_label = gtk_label_new("Platform");
    GtkWidget *username_label = gtk_label_new("Username");
    GtkWidget *password_label = gtk_label_new("Password");
    gtk_widget_add_css_class(platform_label, "muted");
    gtk_widget_add_css_class(username_label, "muted");
    gtk_widget_add_css_class(password_label, "muted");
    gtk_label_set_xalign(GTK_LABEL(platform_label), 0.0);
    gtk_label_set_xalign(GTK_LABEL(username_label), 0.0);
    gtk_label_set_xalign(GTK_LABEL(password_label), 0.0);

    gtk_box_append(GTK_BOX(box), form_title);
    gtk_box_append(GTK_BOX(box), platform_label);
    gtk_box_append(GTK_BOX(box), platform_entry);
    gtk_box_append(GTK_BOX(box), username_label);
    gtk_box_append(GTK_BOX(box), username_entry);
    gtk_box_append(GTK_BOX(box), password_label);
    gtk_box_append(GTK_BOX(box), password_entry);
    gtk_box_append(GTK_BOX(box), actions);

    gtk_box_append(GTK_BOX(content), box);

    gtk_window_set_child(GTK_WINDOW(dialog), content);
    gtk_window_present(GTK_WINDOW(dialog));
}

void on_search_changed(GtkEditable *editable, gpointer data)
{
    (void)editable;
    (void)data;

    refresh_list();
    schedule_auto_lock();
}

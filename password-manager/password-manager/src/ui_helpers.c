#include "password_manager/app.h"

static void on_visibility_icon_pressed(GtkEntry *entry,
                                       GtkEntryIconPosition icon_pos,
                                       gpointer data);
static gboolean hide_toast_cb(gpointer data);
static gboolean auto_lock_cb(gpointer data);

void setup_visibility_toggle(GtkWidget *entry)
{
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry),
                                      GTK_ENTRY_ICON_SECONDARY,
                                      "view-reveal-symbolic");
    gtk_entry_set_icon_tooltip_text(GTK_ENTRY(entry),
                                    GTK_ENTRY_ICON_SECONDARY,
                                    "Show");
    g_signal_connect(entry, "icon-press",
                     G_CALLBACK(on_visibility_icon_pressed),
                     NULL);
}

GtkWidget *make_icon_button(const char *icon_name, const char *label)
{
    GtkWidget *button = gtk_button_new();
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name);
    GtkWidget *text = gtk_label_new(label);

    gtk_widget_set_valign(content, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(content, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(content), icon);
    gtk_box_append(GTK_BOX(content), text);
    gtk_button_set_child(GTK_BUTTON(button), content);

    return button;
}

static void on_visibility_icon_pressed(GtkEntry *entry,
                                       GtkEntryIconPosition icon_pos,
                                       gpointer data)
{
    (void)data;

    if (icon_pos != GTK_ENTRY_ICON_SECONDARY)
        return;

    gboolean visible = gtk_entry_get_visibility(entry);
    gtk_entry_set_visibility(entry, !visible);
    gtk_entry_set_icon_from_icon_name(entry,
                                      GTK_ENTRY_ICON_SECONDARY,
                                      visible ? "view-reveal-symbolic" : "view-conceal-symbolic");
    gtk_entry_set_icon_tooltip_text(entry,
                                    GTK_ENTRY_ICON_SECONDARY,
                                    visible ? "Show" : "Hide");

    schedule_auto_lock();
}

void show_toast(const char *message)
{
    if (!toast_revealer || !toast_label)
        return;

    gtk_label_set_text(GTK_LABEL(toast_label), message);
    gtk_revealer_set_reveal_child(GTK_REVEALER(toast_revealer), TRUE);

    if (toast_timeout_id)
        g_source_remove(toast_timeout_id);

    toast_timeout_id = g_timeout_add_seconds(3, hide_toast_cb, NULL);
}

static gboolean hide_toast_cb(gpointer data)
{
    (void)data;

    if (toast_revealer)
        gtk_revealer_set_reveal_child(GTK_REVEALER(toast_revealer), FALSE);

    toast_timeout_id = 0;

    return G_SOURCE_REMOVE;
}

GtkWidget *create_toast_revealer(void)
{
    GtkWidget *revealer = gtk_revealer_new();
    GtkWidget *toast = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    toast_label = gtk_label_new("");
    gtk_widget_add_css_class(toast, "custom-toast");
    gtk_label_set_ellipsize(GTK_LABEL(toast_label), PANGO_ELLIPSIZE_END);

    gtk_box_append(GTK_BOX(toast), toast_label);

    gtk_revealer_set_child(GTK_REVEALER(revealer), toast);
    gtk_revealer_set_transition_type(GTK_REVEALER(revealer),
                                     GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT);
    gtk_revealer_set_transition_duration(GTK_REVEALER(revealer), 180);

    gtk_widget_set_halign(revealer, GTK_ALIGN_END);
    gtk_widget_set_valign(revealer, GTK_ALIGN_END);
    gtk_widget_set_margin_bottom(revealer, 12);
    gtk_widget_set_margin_end(revealer, 12);

    return revealer;
}

gboolean clear_clipboard_cb(gpointer data)
{
    (void)data;

    GdkDisplay *display = gdk_display_get_default();
    GdkClipboard *clipboard = gdk_display_get_clipboard(display);

    gdk_clipboard_set_text(clipboard, "");
    clipboard_clear_timeout_id = 0;

    return G_SOURCE_REMOVE;
}

static gboolean auto_lock_cb(gpointer data)
{
    (void)data;

    auto_lock_timeout_id = 0;
    lock_app();
    show_toast("Vault locked");

    return G_SOURCE_REMOVE;
}

void schedule_auto_lock(void)
{
    if (auto_lock_timeout_id)
        g_source_remove(auto_lock_timeout_id);

    auto_lock_timeout_id = g_timeout_add_seconds(300, auto_lock_cb, NULL);
}

void lock_app(void)
{
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "lock");
    gtk_editable_set_text(GTK_EDITABLE(pin_entry), "");

    if (search_entry)
        gtk_editable_set_text(GTK_EDITABLE(search_entry), "");

    if (old_pin_entry)
        gtk_editable_set_text(GTK_EDITABLE(old_pin_entry), "");

    if (new_pin_entry)
        gtk_editable_set_text(GTK_EDITABLE(new_pin_entry), "");

    if (credentials)
        g_ptr_array_set_size(credentials, 0);

    if (clipboard_clear_timeout_id)
    {
        g_source_remove(clipboard_clear_timeout_id);
        clipboard_clear_timeout_id = 0;
    }
    clear_clipboard_cb(NULL);

    g_clear_pointer(&current_pin, g_free);

    if (auto_lock_timeout_id)
    {
        g_source_remove(auto_lock_timeout_id);
        auto_lock_timeout_id = 0;
    }
}

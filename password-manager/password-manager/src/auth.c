#include "password_manager/app.h"

void open_main(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    const char *entered = gtk_editable_get_text(GTK_EDITABLE(pin_entry));

    if (verify_pin(entered))
    {
        g_free(current_pin);
        current_pin = g_strdup(entered);
        load_credentials();
        save_credentials();
        refresh_list();
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "main");
        schedule_auto_lock();
    }
    else
    {
        gtk_editable_set_text(GTK_EDITABLE(pin_entry), "");
        show_toast("Wrong PIN");
    }
}

void go_lock(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    lock_app();
}

void open_settings(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    gtk_stack_set_visible_child_name(GTK_STACK(stack), "settings");
}

void change_pin(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    const char *old_pin = gtk_editable_get_text(GTK_EDITABLE(old_pin_entry));
    const char *new_pin = gtk_editable_get_text(GTK_EDITABLE(new_pin_entry));

    if (verify_pin(old_pin))
    {
        if (!*new_pin)
        {
            show_toast("New PIN cannot be empty");
            return;
        }

        g_free(current_pin);
        current_pin = g_strdup(old_pin);
        load_credentials();

        g_free(saved_pin);
        saved_pin = g_strdup(new_pin);
        save_pin();

        g_free(current_pin);
        current_pin = g_strdup(new_pin);
        save_credentials();

        show_toast("PIN changed successfully");
        lock_app();
    }
    else
    {
        gtk_editable_set_text(GTK_EDITABLE(old_pin_entry), "");
        show_toast("Old PIN is incorrect");
    }
}

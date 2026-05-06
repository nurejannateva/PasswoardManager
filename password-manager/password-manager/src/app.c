#include "password_manager/app.h"

static void activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    load_css();
    load_pin();
    credentials = g_ptr_array_new_with_free_func(credential_free);

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Password Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *header = gtk_header_bar_new();
    gtk_window_set_titlebar(GTK_WINDOW(window), header);

    GtkWidget *settings_btn = gtk_button_new_from_icon_name("emblem-system-symbolic");
    gtk_widget_set_tooltip_text(settings_btn, "Change PIN");
    g_signal_connect(settings_btn, "clicked", G_CALLBACK(open_settings), NULL);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), settings_btn);

    stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(stack), 200);

    GtkWidget *overlay = gtk_overlay_new();
    gtk_overlay_set_child(GTK_OVERLAY(overlay), stack);

    toast_revealer = create_toast_revealer();
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), toast_revealer);

    gtk_window_set_child(GTK_WINDOW(window), overlay);

    GtkWidget *lock_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(lock_container, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(lock_container, GTK_ALIGN_CENTER);

    GtkWidget *lock_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(lock_card, "card");
    gtk_widget_set_size_request(lock_card, 360, -1);

    GtkWidget *title = gtk_label_new("Unlock your Vault");

    pin_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(pin_entry), "PIN");
    setup_visibility_toggle(pin_entry);

    GtkWidget *unlock_btn = gtk_button_new_with_label("Unlock");
    g_signal_connect(unlock_btn, "clicked", G_CALLBACK(open_main), NULL);

    gtk_box_append(GTK_BOX(lock_card), title);
    gtk_box_append(GTK_BOX(lock_card), pin_entry);
    gtk_box_append(GTK_BOX(lock_card), unlock_btn);
    gtk_box_append(GTK_BOX(lock_container), lock_card);
    gtk_stack_add_titled(GTK_STACK(stack), lock_container, "lock", "Lock");

    GtkWidget *main_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(main_container, 16);
    gtk_widget_set_margin_bottom(main_container, 16);
    gtk_widget_set_margin_start(main_container, 16);
    gtk_widget_set_margin_end(main_container, 16);
    gtk_widget_set_size_request(main_container, 560, -1);
    gtk_widget_set_valign(main_container, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(main_container, GTK_ALIGN_CENTER);

    GtkWidget *top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(top, GTK_ALIGN_FILL);

    GtkWidget *back_btn = make_icon_button("go-previous-symbolic", "Back");
    g_signal_connect(back_btn, "clicked", G_CALLBACK(go_lock), NULL);
    gtk_widget_set_size_request(back_btn, 130, -1);

    GtkWidget *add_btn = make_icon_button("list-add-symbolic", "Add New");
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_clicked), NULL);
    gtk_widget_set_size_request(add_btn, 130, -1);
    gtk_widget_set_hexpand(add_btn, TRUE);
    gtk_widget_set_halign(add_btn, GTK_ALIGN_END);

    gtk_box_append(GTK_BOX(top), back_btn);
    gtk_box_append(GTK_BOX(top), add_btn);

    search_entry = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Search credentials");
    g_signal_connect(search_entry, "search-changed",
                     G_CALLBACK(on_search_changed),
                     NULL);

    cred_list = gtk_list_box_new();
    gtk_widget_add_css_class(cred_list, "credential-list");

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_add_css_class(scrolled, "card");
    gtk_widget_set_size_request(scrolled, 560, 380);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), cred_list);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);

    gtk_box_append(GTK_BOX(main_container), top);
    gtk_box_append(GTK_BOX(main_container), search_entry);
    gtk_box_append(GTK_BOX(main_container), scrolled);
    gtk_stack_add_titled(GTK_STACK(stack), main_container, "main", "Main");

    GtkWidget *settings_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(settings_container, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(settings_container, GTK_ALIGN_CENTER);

    GtkWidget *settings_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(settings_card, "card");
    gtk_widget_set_size_request(settings_card, 360, -1);

    GtkWidget *settings_title = gtk_label_new("Change PIN");

    old_pin_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(old_pin_entry), "Old PIN");
    setup_visibility_toggle(old_pin_entry);

    new_pin_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(new_pin_entry), "New PIN");
    setup_visibility_toggle(new_pin_entry);

    GtkWidget *change_btn = gtk_button_new_with_label("Update PIN");
    g_signal_connect(change_btn, "clicked", G_CALLBACK(change_pin), NULL);

    GtkWidget *settings_back_btn = make_icon_button("go-previous-symbolic", "Back");
    g_signal_connect(settings_back_btn, "clicked", G_CALLBACK(go_lock), NULL);

    gtk_box_append(GTK_BOX(settings_card), settings_title);
    gtk_box_append(GTK_BOX(settings_card), old_pin_entry);
    gtk_box_append(GTK_BOX(settings_card), new_pin_entry);
    gtk_box_append(GTK_BOX(settings_card), change_btn);
    gtk_box_append(GTK_BOX(settings_card), settings_back_btn);
    gtk_box_append(GTK_BOX(settings_container), settings_card);
    gtk_stack_add_titled(GTK_STACK(stack), settings_container, "settings", "Settings");

    gtk_widget_set_vexpand(stack, TRUE);
    gtk_widget_set_hexpand(stack, TRUE);
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "lock");

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv)
{
    g_autoptr(AdwApplication) app = adw_application_new(
        "org.gtk.passwordmanager",
        G_APPLICATION_DEFAULT_FLAGS
    );

    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    return g_application_run(G_APPLICATION(app), argc, argv);
}

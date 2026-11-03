/* MIT License
 *
 * Copyright (c) 2026 Vitor Figueiredo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "rh_pim-application.h"
#include "rh_pim-ui.h"

struct _RhPimApplication {
  GtkApplication parent_instance;
};

G_DEFINE_FINAL_TYPE(RhPimApplication, rh_pim_application, GTK_TYPE_APPLICATION)

static void rh_pim_application_activate(GApplication *app) {
  GtkWindow *window;

  g_assert(RH_PIM_IS_APPLICATION(app));

  window = gtk_application_get_active_window(GTK_APPLICATION(app));

  if (window == NULL)
    window = GTK_WINDOW(rh_pim_ui_criar_janela(GTK_APPLICATION(app)));

  gtk_window_present(window);
}

static void rh_pim_application_about_action(GSimpleAction *action,
                                            GVariant *parameter,
                                            gpointer user_data) {
  static const char *authors[] = {"Vitor Figueiredo", NULL};
  RhPimApplication *self = user_data;
  GtkWindow *window;
  GtkWidget *about;

  g_assert(RH_PIM_IS_APPLICATION(self));

  window = gtk_application_get_active_window(GTK_APPLICATION(self));

  about = gtk_about_dialog_new();
  gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about), "Rh_pim");
  gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about),
                                      "org.gnome.Example");
  gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), authors);
  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), "0.1.0");
  gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about),
                                 "© 2026 Vitor Figueiredo");
  gtk_window_set_transient_for(GTK_WINDOW(about), window);
  gtk_window_set_modal(GTK_WINDOW(about), TRUE);
  gtk_window_present(GTK_WINDOW(about));
}

static void rh_pim_application_quit_action(GSimpleAction *action,
                                           GVariant *parameter,
                                           gpointer user_data) {
  RhPimApplication *self = user_data;
  g_assert(RH_PIM_IS_APPLICATION(self));
  g_application_quit(G_APPLICATION(self));
}

static const GActionEntry app_actions[] = {
    {"quit", rh_pim_application_quit_action},
    {"about", rh_pim_application_about_action},
};

static void rh_pim_application_class_init(RhPimApplicationClass *klass) {
  GApplicationClass *app_class = G_APPLICATION_CLASS(klass);
  app_class->activate = rh_pim_application_activate;
}

static void rh_pim_application_init(RhPimApplication *self) {
  g_action_map_add_action_entries(G_ACTION_MAP(self), app_actions,
                                  G_N_ELEMENTS(app_actions), self);
  gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.quit",
                                        (const char *[]){"<control>q", NULL});
}

RhPimApplication *rh_pim_application_new(const char *application_id,
                                         GApplicationFlags flags) {
  g_return_val_if_fail(application_id != NULL, NULL);

  return g_object_new(RH_PIM_TYPE_APPLICATION, "application-id", application_id,
                      "flags", flags, "resource-base-path",
                      "/org/gnome/Example", NULL);
}

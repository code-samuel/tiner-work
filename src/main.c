#include "config.h"
#include "rh_pim-application.h"
#include <gio/gio.h>
#include <glib/gi18n.h>

int main(int argc, char *argv[]) {
  g_autoptr(RhPimApplication) app = NULL;
  int ret;

  bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  
  textdomain(GETTEXT_PACKAGE);
  app = rh_pim_application_new(
      "org.gnome.Example",
      G_APPLICATION_DEFAULT_FLAGS); // Nome da função atualizado
  ret = g_application_run(G_APPLICATION(app), argc, argv);

  return ret;
}
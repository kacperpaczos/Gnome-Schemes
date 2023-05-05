/* schemes-application.c
 *
 * Copyright 2021 Christian Hergert
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtksourceview/gtksource.h>
#include <glib/gi18n.h>
#include <libpanel.h>

#include "build-ident.h"

#include "schemes-application.h"
#include "schemes-window.h"

struct _SchemesApplication
{
  AdwApplication parent_instance;
  GSettings *settings;
};

G_DEFINE_TYPE (SchemesApplication, schemes_application, ADW_TYPE_APPLICATION)

static const char *authors[] = {
  "Christian Hergert",
  "Kacper Paczos",
  NULL
};

SchemesApplication *
schemes_application_new (const char        *application_id,
                         GApplicationFlags  flags)
{
  return g_object_new (SCHEMES_TYPE_APPLICATION,
                       "application-id", application_id,
                       "flags", flags,
                       "resource-base-path", "/me/hergert/Schemes",
                       NULL);
}

static void
schemes_application_finalize (GObject *object)
{
  SchemesApplication *self = (SchemesApplication *)object;

  g_clear_object (&self->settings);

  G_OBJECT_CLASS (schemes_application_parent_class)->finalize (object);
}

static void
schemes_application_activate (GApplication *app)
{
  GtkWindow *window;

  g_assert (GTK_IS_APPLICATION (app));

  window = gtk_application_get_active_window (GTK_APPLICATION (app));

  if (window == NULL)
    window = g_object_new (SCHEMES_TYPE_WINDOW,
                           "application", app,
                           NULL);

  gtk_window_present (window);
}

static gboolean
style_variant_to_color_scheme (GValue   *value,
                               GVariant *variant,
                               gpointer  user_data)
{
  if (g_strcmp0 (g_variant_get_string (variant, NULL), "default") == 0)
    g_value_set_enum (value, ADW_COLOR_SCHEME_PREFER_LIGHT);
  else if (g_strcmp0 (g_variant_get_string (variant, NULL), "dark") == 0)
    g_value_set_enum (value, ADW_COLOR_SCHEME_FORCE_DARK);
  else
    g_value_set_enum (value, ADW_COLOR_SCHEME_FORCE_LIGHT);

  return TRUE;
}

static void
about_cb (GSimpleAction *action,
          GVariant      *param,
          gpointer       user_data)
{
  SchemesApplication *self = user_data;

  g_assert (G_IS_SIMPLE_ACTION (action));
  g_assert (SCHEMES_IS_APPLICATION (self));

  adw_show_about_window (gtk_application_get_active_window (GTK_APPLICATION (self)),
                         "application-name", _("Schemes"),
                         "application-icon", PACKAGE_ICON_NAME,
                         "developer-name", "Christian Hergert",
                         "developers", authors,
#if DEVELOPMENT
                         "version", BUILD_IDENTIFIER,
#else
                         "version", PACKAGE_VERSION,
#endif
                         "copyright", "© 2021-2022 Christian Hergert, et al.",
                         "license-type", GTK_LICENSE_GPL_3_0,
                         "website", PACKAGE_WEBSITE,
                         "issue-url", PACKAGE_ISSUE_URL,
                         "translator-credits", _("translator-credits"),
                         NULL);
}

static const GActionEntry action_entries[] = {
  { "about", about_cb, }
};

static void
schemes_application_startup (GApplication *app)
{
  SchemesApplication *self = (SchemesApplication *)app;
  g_autoptr(GAction) theme = NULL;
  AdwStyleManager *style_manager;

  g_assert (SCHEMES_IS_APPLICATION (self));

  G_APPLICATION_CLASS (schemes_application_parent_class)->startup (app);

  gtk_source_init ();
  panel_init ();

  self->settings = g_settings_new ("me.hergert.Schemes");

  theme = g_settings_create_action (self->settings, "style-variant");
  g_action_map_add_action (G_ACTION_MAP (self), theme);
  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   action_entries,
                                   G_N_ELEMENTS (action_entries),
                                   self);

  style_manager = adw_style_manager_get_default ();
  g_settings_bind_with_mapping (self->settings, "style-variant",
                                style_manager, "color-scheme",
                                G_SETTINGS_BIND_GET,
                                style_variant_to_color_scheme,
                                NULL, NULL, NULL);
}

static void
schemes_application_open (GApplication  *app,
                          GFile        **files,
                          int            n_files,
                          const char    *hint)
{
  SchemesApplication *self = (SchemesApplication *)app;

  g_assert (SCHEMES_IS_APPLICATION (self));
  g_assert (files != NULL || n_files == 0);

  if (n_files <= 0)
    return;

  for (guint i = 0; i < n_files; i++)
    {
      g_autoptr(SchemesScheme) scheme = NULL;
      g_autoptr(GError) error = NULL;
      SchemesWindow *window;
      GFile *file = files[i];

      scheme = schemes_scheme_new ();

      if (!schemes_scheme_load_from_file (scheme, file, &error))
        {
          g_warning ("%s", error->message);
          continue;
        }

      window = g_object_new (SCHEMES_TYPE_WINDOW,
                             "application", app,
                             "scheme", scheme,
                             NULL);

      gtk_window_present (GTK_WINDOW (window));
    }
}

static void
schemes_application_class_init (SchemesApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  object_class->finalize = schemes_application_finalize;

  app_class->activate = schemes_application_activate;
  app_class->startup = schemes_application_startup;
  app_class->open = schemes_application_open;
}

static void
schemes_application_init (SchemesApplication *self)
{
  static const struct {
    const char *action_name;
    const char *accels[4];
  } accels[] = {
    { "scheme.open", { "<Control>o", NULL }},
    { "scheme.save", { "<Control>s", NULL }},
    { "scheme.save-as", { "<Control><Shift>s", NULL }},
    { "scheme.new", { "<Control>n", NULL }},
  };

  for (guint i = 0; i < G_N_ELEMENTS (accels); i++)
    gtk_application_set_accels_for_action (GTK_APPLICATION (self),
                                           accels[i].action_name,
                                           accels[i].accels);

}

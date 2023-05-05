/* schemes-window.c
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

#include <glib/gi18n.h>
#include <gtksourceview/gtksource.h>
#include <libpanel.h>

#include "schemes-color-row.h"
#include "schemes-scheme.h"
#include "schemes-style-row.h"
#include "schemes-window.h"

struct _SchemesWindow
{
  AdwApplicationWindow parent_instance;

  SchemesScheme       *scheme;

  AdwEntryRow         *author;
  AdwEntryRow         *name;
  AdwEntryRow         *id;
  AdwEntryRow         *description;
  GtkSourceBuffer     *preview;
  GtkSourceView       *view;
  GtkListBox          *colors;
  AdwEntryRow         *color_name;
  AdwEntryRow         *color_rgba;
  GtkButton           *add_color;
  AdwPreferencesGroup *colors_group;
  PanelThemeSelector  *theme_selector;
  GtkMenuButton       *primary_menu_button;
  GMenu               *primary_menu;
  GtkSwitch           *dark;
  AdwEntryRow         *alternate;
  AdwPreferencesPage  *styles_page;
  AdwViewStack        *stack;
  AdwViewStackPage    *styles;
  GMenu               *doc_types_menu;
  AdwPreferencesGroup *lang_group;

  GHashTable          *style_groups;
  guint                preview_timeout;
};

G_DEFINE_TYPE (SchemesWindow, schemes_window, ADW_TYPE_APPLICATION_WINDOW)

enum {
  PROP_0,
  PROP_DRAW_SPACES,
  PROP_SCHEME,
  PROP_LANGUAGE,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static int
compare_section (gconstpointer a,
                 gconstpointer b)
{
  return g_strcmp0 (*(char **)a, *(char **)b);
}

static void
load_doc_types (SchemesWindow *self)
{
  GtkSourceLanguageManager *lm = gtk_source_language_manager_get_default ();
  const char * const *ids = gtk_source_language_manager_get_language_ids (lm);
  g_autoptr(GHashTable) sections = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  g_autofree char **keys = NULL;
  guint len;

  for (guint i = 0; ids[i]; i++)
    {
      GtkSourceLanguage *lang = gtk_source_language_manager_get_language (lm, ids[i]);
      g_autofree char *action = NULL;
      const char *section;
      const char *name;
      GMenu *parent;

      if (gtk_source_language_get_hidden (lang))
        continue;

      if (!(section = gtk_source_language_get_section (lang)))
        continue;

      if (!(parent = g_hash_table_lookup (sections, section)))
        {
          parent = g_menu_new ();
          g_hash_table_insert (sections, g_strdup (section), parent);
        }

      name = gtk_source_language_get_name (lang);
      action = g_strdup_printf ("win.language::%s", ids[i]);

      g_menu_append (parent, name, action);
    }

  keys = (char **)g_hash_table_get_keys_as_array (sections, &len);
  qsort (keys, g_strv_length (keys), sizeof (char *), compare_section);

  for (guint i = 0; keys[i]; i++)
    g_menu_append_submenu (self->doc_types_menu,
                           keys[i],
                           g_hash_table_lookup (sections, keys[i]));
}

static void
load_scheme_styles (SchemesWindow *self)
{
  GtkSourceLanguageManager *lm;
  GtkSourceLanguage *def;
  g_auto(GStrv) style_ids = NULL;
  GHashTableIter iter;
  GtkWidget *group;
  GtkWidget *row;
  gpointer k, v;

  g_assert (SCHEMES_IS_WINDOW (self));

  if (self->style_groups == NULL)
    self->style_groups = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_iter_init (&iter, self->style_groups);
  while (g_hash_table_iter_next (&iter, &k, &v))
    {
      adw_preferences_page_remove (self->styles_page, v);
      g_hash_table_iter_remove (&iter);
    }

  if (self->scheme == NULL)
    return;

#define SCHEMES_STYLE(group_name, name, title, subtitle, flags)              \
  G_STMT_START {                                                             \
    SchemesStyle *style;                                                     \
                                                                             \
    if (!(group = g_hash_table_lookup (self->style_groups, group_name)))     \
      {                                                                      \
        group = adw_preferences_group_new ();                                \
        g_hash_table_insert (self->style_groups, (char *)group_name, group); \
        adw_preferences_page_add (self->styles_page,                         \
                                  ADW_PREFERENCES_GROUP (group));            \
      }                                                                      \
                                                                             \
    style = schemes_scheme_get_style (self->scheme, name);                   \
    row = schemes_style_row_new (title, subtitle, flags, style);             \
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (group), row);          \
  } G_STMT_END
# include "schemes-styles.defs"
#undef SCHEMES_STYLE

  lm = gtk_source_language_manager_get_default ();
  def = gtk_source_language_manager_get_language (lm, "def");
  style_ids = gtk_source_language_get_style_ids (def);
  qsort (style_ids, g_strv_length (style_ids), sizeof (char *), compare_section);

  group = adw_preferences_group_new ();
  adw_preferences_group_set_title (ADW_PREFERENCES_GROUP (group), _("Common Styles"));
  adw_preferences_page_add (self->styles_page,
                            ADW_PREFERENCES_GROUP (group));

  for (guint i = 0; style_ids[i]; i++)
    {
      const char *name = gtk_source_language_get_style_name (def, style_ids[i]);
      g_autoptr(SchemesStyle) style = schemes_scheme_get_style (self->scheme, style_ids[i]);
      row = schemes_style_row_new (style_ids[i], name, SCHEMES_STYLE_OPTIONS_HAS_ALL, style);
      adw_preferences_group_add (ADW_PREFERENCES_GROUP (group), row);
    }

  self->lang_group = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_page_add (self->styles_page, self->lang_group);
}

static void
update_add_color (SchemesWindow *self)
{
  const char *text;

  g_assert (SCHEMES_IS_WINDOW (self));

  if (!gtk_widget_has_css_class (GTK_WIDGET (self->color_rgba), "error") &&
      (text = gtk_editable_get_text (GTK_EDITABLE (self->color_rgba))) &&
      text[0] != 0 &&
      (text = gtk_editable_get_text (GTK_EDITABLE (self->color_name))) &&
      text[0] != 0)
    gtk_widget_set_sensitive (GTK_WIDGET (self->add_color), TRUE);
  //else
    //gtk_widget_set_sensitive (GTK_WIDGET (self->add_color), FALSE);
}

static void
validate_color_cb (GtkEditable   *editable,
                   SchemesWindow *self)
{
  const char *text;
  GdkRGBA color;

  g_assert (GTK_IS_EDITABLE (editable));

  text = gtk_editable_get_text (editable);

  if (text && text[0] && !gdk_rgba_parse (&color, text))
    gtk_widget_add_css_class (GTK_WIDGET (editable), "error");
  else
    gtk_widget_remove_css_class (GTK_WIDGET (editable), "error");

  update_add_color (self);
}

static void
on_id_changed_cb (SchemesWindow *self,
                  AdwEntryRow   *row)
{
  const char *text;

  g_assert (SCHEMES_IS_WINDOW (self));
  g_assert (ADW_IS_ENTRY_ROW (row));

  text = gtk_editable_get_text (GTK_EDITABLE (row));

  for (const char *c = text; *c; c = g_utf8_next_char (c))
    {
      if (g_unichar_isspace (g_utf8_get_char (c)))
        {
          gtk_widget_add_css_class (GTK_WIDGET (row), "error");
          return;
        }
    }

  gtk_widget_remove_css_class (GTK_WIDGET (row), "error");
}

static void
do_add_color (SchemesWindow *self)
{
  g_autoptr(SchemesColor) color = NULL;
  const char *name;
  const char *title;
  const char *rgb_color;
  GdkRGBA rgba;

  g_assert (SCHEMES_IS_WINDOW (self));

  //if (!gtk_widget_get_sensitive (GTK_WIDGET (self->add_color)))
  //  return;

  g_object_get(GTK_EDITABLE (self->color_name), "title", &title, NULL);
  name = gtk_editable_get_text (GTK_EDITABLE (self->color_name));

  // If the user doesn't give a name, use the default value for from title, "Name" for English, "Nazwa" for Polish, etc.
  if (name == NULL || name[0] == '\0') {
      name = title;
  }

  rgb_color = gtk_editable_get_text (GTK_EDITABLE (self->color_rgba));
  if (rgb_color == NULL || rgb_color[0] == '\0') {
      // TODO make default white or last color.
      rgb_color = "white";

  }
  gdk_rgba_parse (&rgba, rgb_color);

  color = schemes_color_new (name, &rgba);
  schemes_scheme_add_color (self->scheme, color);

  gtk_editable_set_text (GTK_EDITABLE (self->color_name), "");
  gtk_editable_set_text (GTK_EDITABLE (self->color_rgba), "");

  gtk_widget_grab_focus (GTK_WIDGET (self->color_name));
}

static void
add_color_clicked_cb (SchemesWindow *self,
                      GtkButton     *button)
{
  g_assert (SCHEMES_IS_WINDOW (self));

  do_add_color (self);
}

static void
on_color_activate_cb (SchemesWindow *self,
                      AdwEntryRow   *row)
{
  g_assert (SCHEMES_IS_WINDOW (self));

  do_add_color (self);
}

static void
import_palette_response_cb (SchemesWindow        *self,
                            int                   response_code,
                            GtkFileChooserNative *dialog)
{
  g_autoptr(GFile) file = NULL;
  g_autoptr(GError) error = NULL;
  g_autofree char *contents = NULL;
  gsize len;

  g_assert (SCHEMES_IS_WINDOW (self));
  g_assert (GTK_IS_FILE_CHOOSER_NATIVE (dialog));

  if (response_code != GTK_RESPONSE_ACCEPT)
    return;

  if (!(file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog))))
    return;

  if (!g_file_load_contents (file, NULL, &contents, &len, NULL, &error))
    {
      g_warning ("Failed to load file: %s", error->message);
      return;
    }

  if (!schemes_scheme_import_palette (self->scheme, contents, len, &error))
    {
      g_warning ("Failed to import palette: %s", error->message);
      return;
    }
}

static void
import_palette_cb (GtkWidget  *widget,
                   const char *action_name,
                   GVariant   *param)
{
  SchemesWindow *self = (SchemesWindow *)widget;
  g_autoptr(GtkFileFilter) filter = NULL;
  GtkFileChooserNative *dialog;

  g_assert (SCHEMES_IS_WINDOW (self));

  dialog = gtk_file_chooser_native_new (_("Import Color Palette"),
                                        GTK_WINDOW (self),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        _("Import"),
                                        _("Cancel"));

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("GIMP Palette (*.gpl)"));
  gtk_file_filter_add_pattern (filter, "*.gpl");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), g_steal_pointer (&filter));

  g_signal_connect_object (dialog,
                           "response",
                           G_CALLBACK (import_palette_response_cb),
                           self,
                           G_CONNECT_SWAPPED);

  gtk_native_dialog_show (GTK_NATIVE_DIALOG (dialog));
}

static void
do_save (SchemesWindow *self,
         GFile         *file,
         SchemesScheme *scheme)
{
  g_autoptr(GError) error = NULL;
  g_autofree char *contents = NULL;
  gsize len;

  g_assert (SCHEMES_IS_WINDOW (self));
  g_assert (G_IS_FILE (file));
  g_assert (SCHEMES_IS_SCHEME (scheme));

  contents = schemes_scheme_to_string (scheme);
  len = strlen (contents);

  if (!g_file_replace_contents (file, contents, len, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL, &error))
    g_warning ("Failed to save file: %s", error->message);
}

static void
save_as_response_cb (SchemesWindow        *self,
                     int                   response_code,
                     GtkFileChooserNative *dialog)
{
  g_autoptr(GFile) file = NULL;

  g_assert (SCHEMES_IS_WINDOW (self));
  g_assert (GTK_IS_FILE_CHOOSER_NATIVE (dialog));

  if (response_code != GTK_RESPONSE_ACCEPT)
    goto failure;

  if (!(file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog))))
    goto failure;

  schemes_scheme_set_file (self->scheme, file);
  do_save (self, file, self->scheme);

failure:
  gtk_native_dialog_destroy (GTK_NATIVE_DIALOG (dialog));
}

static void
save_as_cb (GtkWidget  *widget,
            const char *action_name,
            GVariant   *param)
{
  SchemesWindow *self = (SchemesWindow *)widget;
  GtkFileChooserNative *dialog;

  g_assert (SCHEMES_IS_WINDOW (self));

  dialog = gtk_file_chooser_native_new (_("Save As"),
                                        GTK_WINDOW (self),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        _("Save"),
                                        _("Cancel"));
  g_signal_connect_object (dialog,
                           "response",
                           G_CALLBACK (save_as_response_cb),
                           self,
                           G_CONNECT_SWAPPED);
  gtk_native_dialog_show (GTK_NATIVE_DIALOG (dialog));
}

static void
save_cb (GtkWidget  *widget,
         const char *action_name,
         GVariant   *param)
{
  SchemesWindow *self = (SchemesWindow *)widget;
  GFile *file;

  g_assert (SCHEMES_IS_WINDOW (self));

  if (!(file = schemes_scheme_get_file (self->scheme)))
    save_as_cb (widget, NULL, param);
  else
    do_save (self, file, self->scheme);
}

static void
new_cb (GtkWidget  *widget,
        const char *action_name,
        GVariant   *param)
{
  SchemesWindow *new_window;

  g_assert (SCHEMES_IS_WINDOW (widget));

  new_window = g_object_new (SCHEMES_TYPE_WINDOW,
                             "application", g_application_get_default (),
                             NULL);
  gtk_window_present (GTK_WINDOW (new_window));
}

static void
open_response_cb (SchemesWindow        *self,
                  int                   response_code,
                  GtkFileChooserNative *dialog)
{
  SchemesWindow *new_window;
  g_autoptr(GFile) file = NULL;
  g_autoptr(SchemesScheme) scheme = NULL;
  g_autoptr(GError) error = NULL;
  gboolean close_window = FALSE;

  g_assert (SCHEMES_IS_WINDOW (self));
  g_assert (GTK_IS_FILE_CHOOSER_NATIVE (dialog));

  if (response_code != GTK_RESPONSE_ACCEPT)
    goto failure;

  if (!(file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog))))
    goto failure;

  scheme = schemes_scheme_new ();

  if (!schemes_scheme_load_from_file (scheme, file, &error))
    {
      g_warning ("%s", error->message);
      goto failure;
    }

  new_window = g_object_new (SCHEMES_TYPE_WINDOW,
                             "application", g_application_get_default (),
                             "scheme", scheme,
                             NULL);
  adw_view_stack_set_visible_child (new_window->stack,
                                    adw_view_stack_page_get_child (new_window->styles));
  gtk_window_present (GTK_WINDOW (new_window));

  close_window = schemes_scheme_is_pristine (self->scheme);

failure:
  gtk_native_dialog_destroy (GTK_NATIVE_DIALOG (dialog));

  if (close_window)
    gtk_window_destroy (GTK_WINDOW (self));
}

static void
open_cb (GtkWidget  *widget,
         const char *action_name,
         GVariant   *param)
{
  SchemesWindow *self = (SchemesWindow *)widget;
  GtkFileChooserNative *dialog;
  GtkFileFilter *filter;

  g_assert (SCHEMES_IS_WINDOW (self));

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Style Schemes"));
  gtk_file_filter_add_mime_type (filter, "application/xml");

  dialog = gtk_file_chooser_native_new (_("Open"),
                                        GTK_WINDOW (self),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        _("Open"),
                                        _("Cancel"));
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  g_signal_connect_object (dialog,
                           "response",
                           G_CALLBACK (open_response_cb),
                           self,
                           G_CONNECT_SWAPPED);
  gtk_native_dialog_show (GTK_NATIVE_DIALOG (dialog));
}

static void
on_notify_language_cb (SchemesWindow   *self,
                       GParamSpec      *pspec,
                       GtkSourceBuffer *buffer)
{
  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_LANGUAGE]);
}

static void
schemes_window_set_language (SchemesWindow *self,
                             const char    *language)
{
  GtkSourceLanguageManager *lm = gtk_source_language_manager_get_default ();
  GtkSourceLanguage *l = language ? gtk_source_language_manager_get_language (lm, language) : NULL;
  g_autofree char *resource_path = g_strdup_printf ("/examples/%s", language);
  g_autoptr(GBytes) bytes = NULL;
  g_auto(GStrv) style_ids = NULL;

  if ((bytes = g_resources_lookup_data (resource_path, 0, NULL)))
    {
      gtk_source_buffer_set_highlight_syntax (self->preview, FALSE);
      gtk_text_buffer_set_text (GTK_TEXT_BUFFER (self->preview),
                                (const char *)g_bytes_get_data (bytes, NULL),
                                -1);
      gtk_source_buffer_set_language (self->preview, l);
      gtk_source_buffer_set_highlight_syntax (self->preview, TRUE);
    }
  else
    {
      gtk_text_buffer_set_text (GTK_TEXT_BUFFER (self->preview), "", -1);
      gtk_source_buffer_set_language (self->preview, l);
    }

  adw_preferences_page_remove (self->styles_page, self->lang_group);
  self->lang_group = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_page_add (self->styles_page, self->lang_group);

  if (l == NULL)
    return;

  adw_preferences_group_set_title (self->lang_group, gtk_source_language_get_name (l));

  style_ids = gtk_source_language_get_style_ids (l);
  qsort (style_ids, g_strv_length (style_ids), sizeof (char*), compare_section);

  for (guint i = 0; style_ids[i]; i++)
    {
      const SchemesStyleOptions flags = SCHEMES_STYLE_OPTIONS_HAS_ALL;
      const char *name = style_ids[i];
      const char *fallback = gtk_source_language_get_style_fallback (l, style_ids[i]);
      SchemesStyle *style = schemes_scheme_get_style (self->scheme, name);
      const char *subtitle = gtk_source_language_get_style_name (l, name);
      g_autoptr(GString) title = g_string_new (name);
      GtkWidget *row;

      if (fallback)
        g_string_append_printf (title, " <span fgalpha=\"32767\">→ %s</span>", fallback);

      row = schemes_style_row_new (title->str, subtitle, flags, style);
      adw_preferences_group_add (self->lang_group, row);
    }
}

static void
schemes_window_dispose (GObject *object)
{
  SchemesWindow *self = (SchemesWindow *)object;

  g_clear_object (&self->scheme);
  g_clear_handle_id (&self->preview_timeout, g_source_remove);
  g_clear_pointer (&self->style_groups, g_hash_table_unref);

  G_OBJECT_CLASS (schemes_window_parent_class)->dispose (object);
}

static void
schemes_window_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  SchemesWindow *self = SCHEMES_WINDOW (object);

  switch (prop_id)
    {
    case PROP_DRAW_SPACES:
      g_value_set_boolean (value,
        gtk_source_space_drawer_get_enable_matrix (
          gtk_source_view_get_space_drawer (self->view)));
      break;

    case PROP_LANGUAGE:
      {
        GtkSourceLanguage *l = gtk_source_buffer_get_language (self->preview);
        if (l != NULL)
          g_value_set_string (value, gtk_source_language_get_id (l));
        else
          g_value_set_static_string (value, "");
      }
      break;

    case PROP_SCHEME:
      g_value_set_object (value, schemes_window_get_scheme (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
schemes_window_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  SchemesWindow *self = SCHEMES_WINDOW (object);

  switch (prop_id)
    {
    case PROP_DRAW_SPACES:
        gtk_source_space_drawer_set_enable_matrix (
          gtk_source_view_get_space_drawer (self->view),
          g_value_get_boolean (value));
      break;

    case PROP_LANGUAGE:
      schemes_window_set_language (self, g_value_get_string (value));
      break;

    case PROP_SCHEME:
      schemes_window_set_scheme (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
schemes_window_class_init (SchemesWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = schemes_window_dispose;
  object_class->get_property = schemes_window_get_property;
  object_class->set_property = schemes_window_set_property;

  properties [PROP_DRAW_SPACES] =
    g_param_spec_boolean ("draw-spaces", NULL, NULL,
                         FALSE,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_LANGUAGE] =
    g_param_spec_string ("language", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_SCHEME] =
    g_param_spec_object ("scheme",
                         "Scheme",
                         "The scheme to edit",
                         SCHEMES_TYPE_SCHEME,
                         (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/ui/schemes-window.ui");
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, add_color);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, alternate);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, author);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, colors);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, color_name);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, color_rgba);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, colors_group);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, dark);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, description);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, doc_types_menu);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, name);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, id);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, preview);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, primary_menu);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, primary_menu_button);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, stack);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, styles);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, styles_page);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, theme_selector);
  gtk_widget_class_bind_template_child (widget_class, SchemesWindow, view);
  gtk_widget_class_bind_template_callback (widget_class, add_color_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_color_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_id_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_notify_language_cb);
  gtk_widget_class_bind_template_callback (widget_class, validate_color_cb);
  gtk_widget_class_bind_template_callback (widget_class, update_add_color);

  gtk_widget_class_install_action (widget_class, "scheme.import-palette", NULL, import_palette_cb);
  gtk_widget_class_install_action (widget_class, "scheme.open", NULL, open_cb);
  gtk_widget_class_install_action (widget_class, "scheme.save", NULL, save_cb);
  gtk_widget_class_install_action (widget_class, "scheme.save-as", NULL, save_as_cb);
  gtk_widget_class_install_action (widget_class, "scheme.new", NULL, new_cb);

  g_type_ensure (SCHEMES_TYPE_COLOR_ROW);
}

static void
schemes_window_init (SchemesWindow *self)
{
  GtkPopover *popover;

  gtk_widget_init_template (GTK_WIDGET (self));

#if DEVELOPMENT
  gtk_widget_add_css_class (GTK_WIDGET (self), "devel");
#endif

  gtk_window_set_default_size (GTK_WINDOW (self), 1280, 768);

  gtk_source_buffer_set_style_scheme (self->preview, NULL);
  load_doc_types (self);

  popover = gtk_menu_button_get_popover (self->primary_menu_button);
  gtk_popover_menu_add_child (GTK_POPOVER_MENU (popover),
                              GTK_WIDGET (self->theme_selector),
                              "theme_selector");
}

SchemesScheme *
schemes_window_get_scheme (SchemesWindow *self)
{
  g_return_val_if_fail (SCHEMES_IS_WINDOW (self), NULL);

  return self->scheme;
}

static void
on_colors_changed_cb (SchemesWindow *self,
                      guint          position,
                      guint          removed,
                      guint          added,
                      GListModel    *colors)
{
  if (g_list_model_get_n_items (colors) == 0)
    gtk_widget_hide (GTK_WIDGET (self->colors_group));
  else
    gtk_widget_show (GTK_WIDGET (self->colors_group));
}

static void
remove_color_row_cb (SchemesWindow   *self,
                     SchemesColorRow *row)
{
  SchemesColor *color;
  printf("usuwa\n");
  g_assert (SCHEMES_IS_WINDOW (self));
  g_assert (SCHEMES_IS_COLOR_ROW (row));

  color = schemes_color_row_get_color (row);
  schemes_scheme_remove_color (self->scheme, color);
}

static GtkWidget *
create_color_row_cb (gpointer item,
                     gpointer item_data)
{
  SchemesWindow *self = item_data;
  GtkWidget *ret;

  g_assert (SCHEMES_IS_COLOR (item));
  g_assert (SCHEMES_IS_WINDOW (self));

  ret = schemes_color_row_new (item);
  g_signal_connect_object (ret,
                           "remove",
                           G_CALLBACK (remove_color_row_cb),
                           self,
                           G_CONNECT_SWAPPED);
  return ret;
}

static gboolean
preview_cb (gpointer data)
{
  SchemesWindow *self = data;
  g_autoptr(GtkSourceStyleScheme) scheme = NULL;

  g_assert (SCHEMES_IS_WINDOW (self));

  self->preview_timeout = 0;
  scheme = schemes_scheme_preview (self->scheme);
  gtk_source_buffer_set_style_scheme (self->preview, scheme);

  return G_SOURCE_REMOVE;
}

static void
schemes_window_queue_preview (SchemesWindow *self)
{
  g_assert (SCHEMES_IS_WINDOW (self));

  if (self->preview_timeout == 0)
    self->preview_timeout = g_timeout_add (500, preview_cb, self);
}

static void
on_scheme_changed_cb (SchemesWindow *self,
                      GParamSpec    *pspec,
                      SchemesScheme *scheme)
{
  g_assert (SCHEMES_IS_WINDOW (self));
  g_assert (SCHEMES_IS_SCHEME (scheme));

  if (scheme != self->scheme)
    return;

  schemes_window_queue_preview (self);
}

static void
load_scheme_actions (SchemesWindow *self,
                     SchemesScheme *scheme)
{
  static const char *view_props[] = {
    "show-line-numbers",
    "show-right-margin",
    "background-pattern",
    "enable-snippets",
    "highlight-current-line",
    "insert-spaces-instead-of-tabs",
    "tab-width",
    "wrap-mode",
  };
  static const char *win_props[] = {
    "draw-spaces",
    "language",
  };

  for (guint i = 0; i < G_N_ELEMENTS (view_props); i++)
    {
      g_autoptr(GPropertyAction) action = g_property_action_new (view_props[i], self->view, view_props[i]);
      g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (action));
    }

  for (guint i = 0; i < G_N_ELEMENTS (win_props); i++)
    {
      g_autoptr(GPropertyAction) action = g_property_action_new (win_props[i], self, win_props[i]);
      g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (action));
    }
}

void
schemes_window_set_scheme (SchemesWindow *self,
                           SchemesScheme *scheme)
{
  g_autoptr(SchemesScheme) alt = NULL;

  g_return_if_fail (SCHEMES_IS_WINDOW (self));
  g_return_if_fail (!scheme || SCHEMES_IS_SCHEME (scheme));

  if (scheme == NULL)
    scheme = alt = schemes_scheme_new ();

  if (self->scheme == scheme)
    return;

  if (self->scheme)
    {
      if (scheme == NULL)
        gtk_list_box_bind_model (self->colors, NULL, NULL, NULL, NULL);
      g_clear_object (&self->scheme);
    }

  if (scheme)
    {
      GListModel *colors = schemes_scheme_get_colors (scheme);

      self->scheme = g_object_ref (scheme);
      g_object_bind_property (self->scheme, "alternate",
                              self->alternate, "text",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
      g_object_bind_property (self->scheme, "author",
                              self->author, "text",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
      g_object_bind_property (self->scheme, "description",
                              self->description, "text",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
      g_object_bind_property (self->scheme, "name",
                              self->name, "text",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
      g_object_bind_property (self->scheme, "id",
                              self->id, "text",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
      g_object_bind_property (self->scheme, "dark",
                              self->dark, "active",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
      g_signal_connect_object (colors,
                               "items-changed",
                               G_CALLBACK (on_colors_changed_cb),
                               self,
                               G_CONNECT_SWAPPED);
      gtk_list_box_bind_model (self->colors,
                               schemes_scheme_get_colors (scheme),
                               create_color_row_cb,
                               self, NULL);
      g_signal_connect_object (self->scheme,
                               "changed",
                               G_CALLBACK (on_scheme_changed_cb),
                               self,
                               G_CONNECT_SWAPPED);
      load_scheme_styles (self);
      load_scheme_actions (self, scheme);
      on_colors_changed_cb (self, 0, 0, 0, colors);
      schemes_window_set_language (self, "c");
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SCHEME]);
}

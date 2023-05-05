/* schemes-scheme.c
 *
 * Copyright 2021 Christian Hergert <chergert@redhat.com>
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
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

#include <glib/gstdio.h>
#include <math.h>
#include <stdlib.h>

#include "schemes-scheme.h"
#include "schemes-xml.h"

struct _SchemesScheme
{
  GObject parent_instance;
  GFile *file;
  GListStore *colors;
  GHashTable *styles;
  char *version;
  char *alternate;
  char *id;
  char *name;
  char *author;
  char *description;

  /* Parsing related data */
  const char *element_name;
  const char *property_name;
  struct {
    int line_pos;
    int char_pos;
    guint failed : 1;
  } parse_failure;

  guint dark : 1;
};

G_DEFINE_TYPE (SchemesScheme, schemes_scheme, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_AUTHOR,
  PROP_COLORS,
  PROP_DESCRIPTION,
  PROP_FILE,
  PROP_ID,
  PROP_NAME,
  PROP_DARK,
  PROP_ALTERNATE,
  N_PROPS
};

enum {
  CHANGED,
  N_SIGNALS
};

#define XML_PARSER_ERROR() \
  G_STMT_START { \
    int line_pos, char_pos; \
    if (!self->parse_failure.failed) \
      { \
        g_markup_parse_context_get_position (context, &line_pos, &char_pos); \
        self->parse_failure.line_pos = line_pos; \
        self->parse_failure.char_pos  =char_pos; \
        self->parse_failure.failed = TRUE; \
      } \
  } G_STMT_END

static void root_end_element   (GMarkupParseContext  *context,
                                const char           *element_name,
                                gpointer              user_data,
                                GError              **error);
static void root_start_element (GMarkupParseContext  *context,
                                const char           *element_name,
                                const char          **attribute_names,
                                const char          **attribute_values,
                                gpointer              user_data,
                                GError              **error);

static GParamSpec *properties [N_PROPS];
static guint signals [N_SIGNALS];

static inline gboolean
str_empty0 (const char *str)
{
  return str == NULL || str[0] == 0;
}

static void
schemes_scheme_emit_changed (SchemesScheme *self)
{
  g_assert (SCHEMES_IS_SCHEME (self));

  g_signal_emit (self, signals [CHANGED], 0);
}

static void
do_notify (SchemesScheme *self,
           guint          prop_id)
{
  g_object_notify_by_pspec (G_OBJECT (self), properties [prop_id]);
  schemes_scheme_emit_changed (self);
}

SchemesScheme *
schemes_scheme_new (void)
{
  return g_object_new (SCHEMES_TYPE_SCHEME, NULL);
}

static void
schemes_scheme_finalize (GObject *object)
{
  SchemesScheme *self = (SchemesScheme *)object;

  g_clear_pointer (&self->author, g_free);
  g_clear_pointer (&self->version, g_free);
  g_clear_pointer (&self->id, g_free);
  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->description, g_free);
  g_clear_pointer (&self->alternate, g_free);
  g_clear_pointer (&self->styles, g_hash_table_unref);
  g_clear_object (&self->colors);

  G_OBJECT_CLASS (schemes_scheme_parent_class)->finalize (object);
}

static void
schemes_scheme_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  SchemesScheme *self = SCHEMES_SCHEME (object);

  switch (prop_id)
    {
    case PROP_ALTERNATE:
      g_value_set_string (value, schemes_scheme_get_alternate (self));
      break;

    case PROP_AUTHOR:
      g_value_set_string (value, schemes_scheme_get_author (self));
      break;

    case PROP_DARK:
      g_value_set_boolean (value, schemes_scheme_get_dark (self));
      break;

    case PROP_ID:
      g_value_set_string (value, schemes_scheme_get_id (self));
      break;

    case PROP_FILE:
      g_value_set_object (value, schemes_scheme_get_file (self));
      break;

    case PROP_COLORS:
      g_value_set_object (value, schemes_scheme_get_colors (self));
      break;

    case PROP_NAME:
      g_value_set_string (value, schemes_scheme_get_name (self));
      break;

    case PROP_DESCRIPTION:
      g_value_set_string (value, schemes_scheme_get_description (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
schemes_scheme_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  SchemesScheme *self = SCHEMES_SCHEME (object);

  switch (prop_id)
    {
    case PROP_ALTERNATE:
      schemes_scheme_set_alternate (self, g_value_get_string (value));
      break;

    case PROP_DARK:
      schemes_scheme_set_dark (self, g_value_get_boolean (value));
      break;

    case PROP_AUTHOR:
      schemes_scheme_set_author (self, g_value_get_string (value));
      break;

    case PROP_ID:
      schemes_scheme_set_id (self, g_value_get_string (value));
      break;

    case PROP_FILE:
      schemes_scheme_set_file (self, g_value_get_object (value));
      break;

    case PROP_NAME:
      schemes_scheme_set_name (self, g_value_get_string (value));
      break;

    case PROP_DESCRIPTION:
      schemes_scheme_set_description (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
schemes_scheme_class_init (SchemesSchemeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = schemes_scheme_finalize;
  object_class->get_property = schemes_scheme_get_property;
  object_class->set_property = schemes_scheme_set_property;

  properties [PROP_FILE] =
    g_param_spec_object ("file", NULL, NULL,
                         G_TYPE_FILE,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_ALTERNATE] =
    g_param_spec_string ("alternate", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_AUTHOR] =
    g_param_spec_string ("author", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_COLORS] =
    g_param_spec_object ("colors", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties [PROP_DARK] =
    g_param_spec_boolean ("dark", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_ID] =
    g_param_spec_string ("id", NULL, NULL,
                         "",
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_NAME] =
    g_param_spec_string ("name", NULL, NULL,
                         "",
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_DESCRIPTION] =
    g_param_spec_string ("description", NULL, NULL,
                         "",
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  signals [CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
}

static void
schemes_scheme_init (SchemesScheme *self)
{
  self->id = g_strdup ("");
  self->name = g_strdup ("");
  self->description = g_strdup ("");
  self->colors = g_list_store_new (SCHEMES_TYPE_COLOR);
  self->author = g_strdup (g_get_real_name ());
}

const char *
schemes_scheme_get_id (SchemesScheme *self)
{
  g_return_val_if_fail (SCHEMES_IS_SCHEME (self), NULL);

  return self->id;
}

void
schemes_scheme_set_id (SchemesScheme *self,
                       const char    *id)
{
  g_return_if_fail (SCHEMES_IS_SCHEME (self));

  if (g_strcmp0 (self->id, id) != 0)
    {
      g_free (self->id);
      self->id = g_strdup (id);
      do_notify (self, PROP_ID);
    }
}

GFile *
schemes_scheme_get_file (SchemesScheme *self)
{
  g_return_val_if_fail (SCHEMES_IS_SCHEME (self), NULL);

  return self->file;
}

void
schemes_scheme_set_file (SchemesScheme *self,
                         GFile         *file)
{
  g_return_if_fail (SCHEMES_IS_SCHEME (self));
  g_return_if_fail (!file || G_IS_FILE (file));

  if (g_set_object (&self->file, file))
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_FILE]);
}

const char *
schemes_scheme_get_name (SchemesScheme *self)
{
  g_return_val_if_fail (SCHEMES_IS_SCHEME (self), NULL);

  return self->name;
}

void
schemes_scheme_set_name (SchemesScheme *self,
                         const char    *name)
{
  g_return_if_fail (SCHEMES_IS_SCHEME (self));

  if (g_strcmp0 (self->name, name) != 0)
    {
      g_free (self->name);
      self->name = g_strdup (name);
      do_notify (self, PROP_NAME);
    }
}

const char *
schemes_scheme_get_description (SchemesScheme *self)
{
  g_return_val_if_fail (SCHEMES_IS_SCHEME (self), NULL);

  return self->description;
}

void
schemes_scheme_set_description (SchemesScheme *self,
                                const char    *description)
{
  g_return_if_fail (SCHEMES_IS_SCHEME (self));

  if (g_strcmp0 (self->description, description) != 0)
    {
      g_free (self->description);
      self->description = g_strdup (description);
      do_notify (self, PROP_DESCRIPTION);
    }
}

GListModel *
schemes_scheme_get_colors (SchemesScheme *self)
{
  g_return_val_if_fail (SCHEMES_IS_SCHEME (self), NULL);

  return G_LIST_MODEL (self->colors);
}

static void
on_color_changed_cb (SchemesScheme *self,
                     const GdkRGBA *previous_color,
                     SchemesColor  *color)
{
  const GdkRGBA *new_color;
  SchemesStyle *style;
  const char *key;
  GHashTableIter iter;

  g_assert (SCHEMES_IS_SCHEME (self));
  g_assert (SCHEMES_IS_COLOR (color));

  new_color = schemes_color_get_color (color);

  g_hash_table_iter_init (&iter, self->styles);
  while (g_hash_table_iter_next (&iter, (gpointer *)&key, (gpointer *)&style))
    {
      g_assert (key != NULL);
      g_assert (style != NULL);

      schemes_style_replace_color (style, previous_color, new_color);
    }
}

void
schemes_scheme_add_color (SchemesScheme *self,
                          SchemesColor  *color)
{
  g_return_if_fail (SCHEMES_IS_SCHEME (self));
  g_return_if_fail (SCHEMES_IS_COLOR (color));

  g_signal_connect_object (color,
                           "color-changed",
                           G_CALLBACK (on_color_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_list_store_append (self->colors, color);
  printf("abba");
  schemes_scheme_emit_changed (self);
}

static void
schemes_scheme_add_color_simple (SchemesScheme *self,
                                 const char    *name,
                                 const GdkRGBA *rgba)
{
  g_autoptr(SchemesColor) color = NULL;

  g_assert (SCHEMES_IS_SCHEME (self));

  color = schemes_color_new (name, rgba);
  schemes_scheme_add_color (self, color);
}

void
schemes_scheme_remove_color (SchemesScheme *self,
                             SchemesColor  *color)
{
  guint n_items;

  g_return_if_fail (SCHEMES_IS_SCHEME (self));
  g_return_if_fail (SCHEMES_IS_COLOR (color));

  n_items = g_list_model_get_n_items (G_LIST_MODEL (self->colors));

  for (guint i = 0; i < n_items; i++)
    {
      g_autoptr(SchemesColor) item = g_list_model_get_item (G_LIST_MODEL (self->colors), i);

      if (item == color)
        {
          g_signal_handlers_disconnect_by_func (color,
                                                G_CALLBACK (on_color_changed_cb),
                                                self);
          g_list_store_remove (self->colors, i);
          schemes_scheme_emit_changed (self);
          break;
        }
    }
}

gboolean
schemes_scheme_import_palette (SchemesScheme  *self,
                               const char     *data,
                               gssize          len,
                               GError        **error)
{
  g_auto(GStrv) lines = NULL;
  gsize n_lines;
  int pos = -1;

  g_return_val_if_fail (SCHEMES_IS_SCHEME (self), FALSE);
  g_return_val_if_fail (data != NULL || len == 0, FALSE);

  if (data == NULL || len == 0)
    return TRUE;

/*
GIMP Palette
Name: Solarized
Columns: 3
#
  0  43  54	solarized-base03
  7  54  66	solarized-base02
 88 110 117	solarized-base01
101 123 131	solarized-base00
131 148 150	solarized-base0
147 161 161	solarized-base1
238 232 213	solarized-base2
253 246 227	solarized-base3
181 137   0	solarized-yellow
203  75  22	solarized-orange
220  50  47	solarized-red
211  54 130	solarized-magenta
108 113 196	solarized-violet
 38 139 210	solarized-blue
 42 161 152	solarized-cyan
133 153   0	solarized-green
*/

  if (!(lines = g_strsplit (data, "\n", 0)) ||
      g_strcmp0 (lines[0], "GIMP Palette") != 0)
    goto failure;

  n_lines = g_strv_length (lines);
  for (guint i = 0; i < n_lines; i++)
    {
      if (lines[i][0] == '#')
        {
          pos = i + 1;
          break;
        }
    }

  if (pos < 0)
    goto failure;

  for (guint i = pos; i < n_lines; i++)
    {
      g_autoptr(SchemesColor) color = NULL;
      GdkRGBA rgba;
      int r, g, b;
      char name[128];

      if (lines[i][0] == 0)
        continue;

      if (sscanf (lines[i], "%d %d %d %128[^\n]", &r, &g, &b, name) != 4)
        goto failure;

      name[sizeof name - 1] = 0;

      rgba.red = (double)r / 255.0;
      rgba.green = (double)g / 255.0;
      rgba.blue = (double)b / 255.0;
      rgba.alpha = 1;

      color = schemes_color_new (name, &rgba);
      g_list_store_append (self->colors, color);
    }

  g_signal_emit (self, signals [CHANGED], 0);

  return TRUE;

failure:
  g_set_error (error,
               G_IO_ERROR,
               G_IO_ERROR_INVALID_DATA,
               "Not a GIMP palette");

  return FALSE;
}

const char *
schemes_scheme_get_alternate (SchemesScheme *self)
{
  g_return_val_if_fail (SCHEMES_IS_SCHEME (self), NULL);

  return self->alternate ? self->alternate : "";
}

void
schemes_scheme_set_alternate (SchemesScheme *self,
                              const char    *alternate)
{
  g_return_if_fail (SCHEMES_IS_SCHEME (self));

  if (alternate != NULL && alternate[0] == 0)
    alternate = NULL;

  if (g_strcmp0 (self->alternate, alternate) != 0)
    {
      g_free (self->alternate);
      self->alternate = g_strdup (alternate);
      do_notify (self, PROP_ALTERNATE);
    }
}

const char *
schemes_scheme_get_author (SchemesScheme *self)
{
  g_return_val_if_fail (SCHEMES_IS_SCHEME (self), NULL);

  return self->author;
}

void
schemes_scheme_set_author (SchemesScheme *self,
                           const char    *author)
{
  g_return_if_fail (SCHEMES_IS_SCHEME (self));

  if (g_strcmp0 (self->author, author) != 0)
    {
      g_free (self->author);
      self->author = g_strdup (author);
      do_notify (self, PROP_AUTHOR);
    }
}

gboolean
schemes_scheme_get_dark (SchemesScheme *self)
{
  g_return_val_if_fail (SCHEMES_IS_SCHEME (self), FALSE);

  return self->dark;
}

void
schemes_scheme_set_dark (SchemesScheme *self,
                         gboolean       dark)
{
  g_return_if_fail (SCHEMES_IS_SCHEME (self));

  dark = !!dark;

  if (dark != self->dark)
    {
      self->dark = dark;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_DARK]);
    }
}

static int
sort_color_func (gconstpointer a,
                 gconstpointer b)
{
  SchemesColor *acolor = *(SchemesColor * const *)a;
  SchemesColor *bcolor = *(SchemesColor * const *)b;

  const gchar *str1 = "Hello";
    const gchar *str2 = "World";

   gint result = g_strcmp0 (schemes_color_get_name (acolor),
                    schemes_color_get_name (bcolor));

    if (result < 0) {
        printf("%s is less than %s\n", schemes_color_get_name (acolor), schemes_color_get_name (bcolor));
    } else if (result > 0) {
        printf("%s is greater than %s\n", schemes_color_get_name (acolor), schemes_color_get_name (bcolor));
    } else {
        printf("%s is equal to %s\n", schemes_color_get_name (acolor), schemes_color_get_name (bcolor));
    }

  return result;
}

static GPtrArray *
get_colors_sorted (GListStore *store)
{
  GPtrArray *ret;
  guint n_items;

  g_assert (G_IS_LIST_STORE (store));

  ret = g_ptr_array_new_with_free_func (g_object_unref);
  n_items = g_list_model_get_n_items (G_LIST_MODEL (store));
  for (guint i = 0; i < n_items; i++)
    g_ptr_array_add (ret, g_list_model_get_item (G_LIST_MODEL (store), i));
  g_ptr_array_sort (ret, sort_color_func);
  printf("dupa1");
  return ret;
}

static int
sort_styles (gconstpointer a,
             gconstpointer b)
{
  SchemesStyle *stylea = *(SchemesStyle **)a;
  SchemesStyle *styleb = *(SchemesStyle **)b;
  const char *langa = schemes_style_get_language (stylea);
  const char *langb = schemes_style_get_language (styleb);
  const char *name_a = schemes_style_get_name (stylea);
  const char *name_b = schemes_style_get_name (styleb);
  const char *use_style_a = schemes_style_get_use_style (stylea);
  const char *use_style_b = schemes_style_get_use_style (styleb);

  /* If this style references another style, it needs to be sorted
   * after that style. This only works when the language for the
   * two are similar.
   */
  if (use_style_a && g_strcmp0 (use_style_a, name_a) > 0)
    name_a = use_style_a;
  if (use_style_b && g_strcmp0 (use_style_b, name_b) > 0)
    name_a = use_style_a;

  if ((langa == NULL && langb == NULL) ||
      (g_strcmp0 (langa, "def") == 0 && g_strcmp0 (langb, "def") == 0))
    {
      int ret = g_strcmp0 (name_a, name_b);

      if (ret == 0)
        {
          if (use_style_a)
            ret = 1;
          else if (use_style_b)
            ret = -1;
        }

      return ret;
    }

  if (g_strcmp0 (langa, NULL) == 0)
    return -1;
  else if (g_strcmp0 (langb, NULL) == 0)
    return 1;

  if (g_strcmp0 (langa, "def") == 0)
    return -1;
  else if (g_strcmp0 (langb, "def") == 0)
    return 1;

  return g_strcmp0 (name_a, name_b);
}

static GPtrArray *
group_styles (GHashTable *hashtable)
{
  GHashTableIter iter;
  SchemesStyle *style;
  GPtrArray *ar;

  ar = g_ptr_array_new ();
  g_hash_table_iter_init (&iter, hashtable);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer *)&style))
    g_ptr_array_add (ar, style);
  g_ptr_array_sort (ar, sort_styles);

  return ar;
}

static char *
as_hex (const GdkRGBA *rgba)
{
  char str[8];

  g_snprintf (str, sizeof str, "#%02X%02X%02X",
              (guint)roundf (rgba->red * 255.0),
              (guint)roundf (rgba->green * 255.0),
              (guint)roundf (rgba->blue * 255.0));

  return g_strdup (str);
}

char *
schemes_scheme_to_string (SchemesScheme  *self)
{
  g_autoptr(GPtrArray) colors = NULL;
  g_autoptr(GHashTable) colors_hash = NULL;
  g_autoptr(GPtrArray) groups = NULL;
  g_autoptr(GDateTime) now = NULL;
  const char *last_lang = NULL;
  GString *string;
  GHashTableIter iter;
  gpointer v;
  gsize max_name = 0;
  int year;

  g_return_val_if_fail (SCHEMES_IS_SCHEME (self), NULL);

  colors = get_colors_sorted (self->colors);
  colors_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  now = g_date_time_new_now_local ();
  year = g_date_time_get_year (now);

  string = g_string_new ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

  /* TODO: Someday allow people to save with another license. But to
   * encourange everyone to create them as LGPL-2.1+ so we can use them
   * with GtkSourceView, just add that here.
   *
   * This is mostly problematic when saving an opened file, since the
   * copyright will be lost based on our current design. We can make that
   * work eventually, but it's more effort than I have time for right now.
   */
  g_string_append_printf (string, "\
<!--\n\
\n\
  Copyright %d %s\n\
\n\
  GtkSourceView is free software; you can redistribute it and/or\n\
  modify it under the terms of the GNU Lesser General Public\n\
  License as published by the Free Software Foundation; either\n\
  version 2.1 of the License, or (at your option) any later version.\n\
\n\
  GtkSourceView is distributed in the hope that it will be useful,\n\
  but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n\
  Lesser General Public License for more details.\n\
\n\
  You should have received a copy of the GNU Lesser General Public License\n\
  along with this library; if not, see <http://www.gnu.org/licenses/>.\n\
\n\
-->\n", year, self->author);

  /* <style-scheme/> */
  schemes_xml_writer_begin_open_element (string, "style-scheme");
  schemes_xml_writer_add_attribute (string, "id", self->id[0] ? self->id : "unknown");
  schemes_xml_writer_add_attribute (string, "_name", self->name[0] ? self->name : "unknown");
  schemes_xml_writer_add_attribute (string, "version", "1.0");
  schemes_xml_writer_end_open_element (string, TRUE);
  g_string_append_c (string, '\n');

  /* <author/> */
  g_string_append (string, "  ");
  schemes_xml_writer_open_element (string, "author");
  g_string_append (string, self->author);
  schemes_xml_writer_close_element (string, "author");
  g_string_append_c (string, '\n');

  /* <description/> */
  g_string_append (string, "  ");
  schemes_xml_writer_open_element (string, "_description");
  g_string_append (string, self->description);
  schemes_xml_writer_close_element (string, "_description");
  g_string_append_c (string, '\n');

  /* <metadata/> */
  g_string_append_c (string, '\n');
  g_string_append (string, "  <metadata>\n");
  g_string_append_printf (string, "    <property name=\"variant\">%s</property>\n", self->dark ? "dark" : "light");
  if (self->alternate)
    {
      g_string_append_printf (string, "    <property name=\"%s-variant\">",
                              self->dark ? "light" : "dark");
      schemes_xml_writer_write_escaped (string, self->alternate);
      g_string_append (string, "</property>\n");
    }
  g_string_append (string, "  </metadata>\n");
  g_string_append_c (string, '\n');

  /* Find longest color/style name to align attributes */
  for (guint i = 0; i < colors->len; i++)
    {
      SchemesColor *color = g_ptr_array_index (colors, i);
      const char *name = schemes_color_get_name (color);
      gsize len = name ? strlen (name) : 0;
      max_name = MAX (len, max_name);
    }
  g_hash_table_iter_init (&iter, self->styles);
  while (g_hash_table_iter_next (&iter, NULL, &v))
    {
      SchemesStyle *style = v;
      const char *name = schemes_style_get_name (style);

      if (!schemes_style_is_empty (style))
        max_name = MAX (name ? strlen (name) : 0, max_name);
    }

  /* Now add all of the colors */
  g_string_append (string, "  <!-- Named Colors -->\n");
  for (guint i = 0; i < colors->len; i++)
    {
      SchemesColor *color = g_ptr_array_index (colors, i);
      const char *name = schemes_color_get_name (color);
      const GdkRGBA *rgba = schemes_color_get_color (color);
      g_autofree char *value = gdk_rgba_to_string (rgba);
      g_autofree char *value_hex = as_hex (rgba);
      gsize padding = 0;

      if (name && strlen (name) < max_name)
        padding = max_name - strlen (name);

      g_string_append (string, "  ");
      schemes_xml_writer_begin_open_element (string, "color");
      schemes_xml_writer_add_attribute (string, "name", name);
      /* Add spacing to align values */
      for (guint z = 0; z < padding; z++)
        g_string_append_c (string, ' ');
      schemes_xml_writer_add_attribute (string, "value", value_hex);
      schemes_xml_writer_end_open_element (string, FALSE);
      g_string_append_c (string, '\n');

      g_hash_table_insert (colors_hash,
                           g_steal_pointer (&value),
                           g_strdup (name));
    }

  g_string_append (string, "\n  <!-- Global Styles -->\n");
  groups = group_styles (self->styles);
  for (guint i = 0; i < groups->len; i++)
    {
      SchemesStyle *style = g_ptr_array_index (groups, i);
      const char *language = schemes_style_get_language (style);

      if (schemes_style_is_empty (style))
        continue;

      if (g_strcmp0 (last_lang, language) != 0)
        {
          GtkSourceLanguageManager *lm = gtk_source_language_manager_get_default ();
          GtkSourceLanguage *l = gtk_source_language_manager_get_language (lm, language);

          if (l != NULL)
            g_string_append_printf (string,
                                    "\n  <!-- %s -->\n",
                                    gtk_source_language_get_name (l));

          last_lang = schemes_style_get_language (style);
        }

      g_string_append (string, "  ");
      schemes_style_serialize (style, string, colors_hash, max_name);
      g_string_append_c (string, '\n');
    }

  g_string_append_c (string, '\n');
  schemes_xml_writer_close_element (string, "style-scheme");

  return g_string_free (string, FALSE);
}

SchemesStyle *
schemes_scheme_get_style (SchemesScheme *self,
                          const char    *name)
{
  SchemesStyle *style;

  g_return_val_if_fail (SCHEMES_IS_SCHEME (self), NULL);

  if (self->styles == NULL)
    self->styles = g_hash_table_new_full (g_str_hash,
                                          g_str_equal,
                                          g_free,
                                          g_object_unref);

  if (!(style = g_hash_table_lookup (self->styles, name)))
    {
      style = schemes_style_new (name);
      g_signal_connect_object (style,
                               "notify",
                               G_CALLBACK (schemes_scheme_emit_changed),
                               self,
                               G_CONNECT_SWAPPED);
      g_hash_table_insert (self->styles, g_strdup (name), style);
    }

  return style;
}

GtkSourceStyleScheme *
schemes_scheme_preview (SchemesScheme *self)
{
  g_autoptr(GtkSourceStyleSchemeManager) manager = NULL;
  const char * search_path[] = { NULL, NULL };
  g_autofree char *str = NULL;
  g_autofree char *tmpdir = NULL;
  g_autofree char *path = NULL;
  GtkSourceStyleScheme *ret = NULL;
  const char * const *ids;
  g_autoptr(GError) error = NULL;

  g_return_val_if_fail (SCHEMES_IS_SCHEME (self), NULL);

  if (!(str = schemes_scheme_to_string (self)))
    goto failure;

  if (!(tmpdir = g_dir_make_tmp (".schemes-XXXXXX", &error)))
    {
      g_warning ("Fail to create tmpdir: %s", error->message);
      goto failure;
    }

  path = g_build_filename (tmpdir, "preview.xml", NULL);
  if (!g_file_set_contents (path, str, -1, NULL))
    {
      g_warning ("Fail to set contents: %s", error->message);
      goto failure;
    }

  manager = gtk_source_style_scheme_manager_new ();
  search_path[0] = tmpdir;
  gtk_source_style_scheme_manager_set_search_path (manager, (const char * const *)search_path);
  gtk_source_style_scheme_manager_force_rescan (manager);

  if (!(ids = gtk_source_style_scheme_manager_get_scheme_ids (manager)) ||
      ids[0] == NULL ||
      !(ret = gtk_source_style_scheme_manager_get_scheme (manager, ids[0])))
    {
      g_warning ("Failed to load preview.xml");
      goto failure;
    }

  g_object_ref (ret);

failure:
  if (path)
    g_unlink (path);

  if (tmpdir)
    g_unlink (tmpdir);

  return ret;
}

static gboolean
styles_empty (GHashTable *hashtable)
{
  GHashTableIter iter;
  SchemesStyle *style;

  g_hash_table_iter_init (&iter, hashtable);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer *)&style))
    {
      if (!schemes_style_is_empty (style))
        return FALSE;
    }

  return TRUE;
}

gboolean
schemes_scheme_is_pristine (SchemesScheme *self)
{
  g_return_val_if_fail (SCHEMES_IS_SCHEME (self), FALSE);

  return self->file == NULL &&
         (self->colors == NULL ||
          g_list_model_get_n_items (G_LIST_MODEL (self->colors)) == 0) &&
         (self->styles == NULL || styles_empty (self->styles)) &&
         str_empty0 (self->alternate) &&
         str_empty0 (self->id) &&
         str_empty0 (self->name) &&
         str_empty0 (self->description);
}

gboolean
schemes_scheme_get_named_color (SchemesScheme *self,
                                const char    *name,
                                GdkRGBA       *rgba)
{
  guint n_items;

  g_return_val_if_fail (SCHEMES_IS_SCHEME (self), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  n_items = g_list_model_get_n_items (G_LIST_MODEL (self->colors));

  for (guint i = 0; i < n_items; i++)
    {
      g_autoptr(SchemesColor) color = g_list_model_get_item (G_LIST_MODEL (self->colors), i);

      if (g_strcmp0 (name, schemes_color_get_name (color)) == 0)
        {
          *rgba = *schemes_color_get_color (color);
          return TRUE;
        }
    }

  return FALSE;
}

static const GMarkupParser root_parser = {
  root_start_element,
  root_end_element,
};

gboolean
schemes_scheme_load_from_file (SchemesScheme  *self,
                               GFile          *file,
                               GError        **error)
{
  g_autoptr(GMarkupParseContext) context = NULL;
  g_autofree char *contents = NULL;
  gsize len;

  g_return_val_if_fail (SCHEMES_IS_SCHEME (self), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  context = g_markup_parse_context_new (&root_parser, 0, self, NULL);

  if (!g_file_load_contents (file, NULL, &contents, &len, NULL, error) ||
      !g_markup_parse_context_parse (context, contents, len, error))
    return FALSE;

  if (self->parse_failure.failed)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_DATA,
                   "Failed to parse style-scheme at %d:%d",
                   self->parse_failure.line_pos,
                   self->parse_failure.char_pos);
      return FALSE;
    }

  if (g_set_object (&self->file, file))
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_FILE]);

  return TRUE;
}

static void
text_parser_text (GMarkupParseContext  *context,
                  const char           *text,
                  gsize                 text_len,
                  gpointer              user_data,
                  GError              **error)
{
  SchemesScheme *self = user_data;
  g_autofree char *trimmed = g_strstrip (g_strdup (text));

  if (str_empty0 (trimmed))
    return;

  if (g_strcmp0 (self->element_name, "author") == 0)
    schemes_scheme_set_author (self, trimmed);
  else if (g_strcmp0 (self->element_name, "_description") == 0 ||
           g_strcmp0 (self->element_name, "description") == 0)
    schemes_scheme_set_description (self, trimmed);
}

static const GMarkupParser text_parser = {
  .text = text_parser_text,
};

static void
metadata_text (GMarkupParseContext  *context,
               const char           *text,
               gsize                 text_len,
               gpointer              user_data,
               GError              **error)
{
  SchemesScheme *self = user_data;
  g_autofree char *trimmed = g_strstrip (g_strdup (text));

  if (str_empty0 (trimmed) || self->property_name == NULL)
    return;

  if (g_strcmp0 (self->property_name, "variant") == 0)
    schemes_scheme_set_dark (self, g_strcmp0 (trimmed, "dark") == 0);
  else if (g_strcmp0 (self->property_name, "light-variant") == 0 ||
           g_strcmp0 (self->property_name, "dark-variant") == 0)
    schemes_scheme_set_alternate (self, trimmed);
}

static void
metadata_start_element (GMarkupParseContext  *context,
                        const char           *element_name,
                        const char          **attribute_names,
                        const char          **attribute_values,
                        gpointer              user_data,
                        GError              **error)
{
  SchemesScheme *self = user_data;

  g_assert (context != NULL);
  g_assert (element_name != NULL);
  g_assert (SCHEMES_IS_SCHEME (self));

  if (g_strcmp0 (element_name, "property") == 0)
    {
      const char *name = NULL;

      if (!g_markup_collect_attributes (element_name, attribute_names, attribute_values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_INVALID))
        return;

      self->property_name = g_intern_string (name);
    }
  else
    XML_PARSER_ERROR ();
}

static void
metadata_end_element (GMarkupParseContext  *context,
                      const char           *element_name,
                      gpointer              user_data,
                      GError              **error)
{
  SchemesScheme *self = user_data;

  g_assert (context != NULL);
  g_assert (element_name != NULL);
  g_assert (SCHEMES_IS_SCHEME (self));

  if (g_strcmp0 (element_name, "property") == 0)
    {
      self->property_name = NULL;
    }
  else
    XML_PARSER_ERROR ();
}

static const GMarkupParser metadata_parser = {
  .start_element = metadata_start_element,
  .end_element = metadata_end_element,
  .text = metadata_text,
};

static gboolean
rgba_parse (GdkRGBA    *rgba,
            const char *color)
{
  if (g_str_has_prefix (color, "#rgb"))
    color++;
  return gdk_rgba_parse (rgba, color);
}

static gboolean
parse_boolean_string (gboolean   *b,
                      const char *s)
{
  if (s == NULL)
    return FALSE;

  switch (*s)
    {
    case 'y':
    case 'Y':
    case 't':
    case 'T':
    case '1':
      *b = TRUE;
      return TRUE;

    case 'n':
    case 'N':
    case 'f':
    case 'F':
    case '0':
      *b = FALSE;
      return TRUE;

    default:
      return FALSE;
    }
}

static void
parse_boolean (SchemesScheme *self,
               SchemesStyle  *style,
               const char    *property,
               const char    *value)
{
  gboolean b;

  g_assert (SCHEMES_IS_SCHEME (self));
  g_assert (SCHEMES_IS_STYLE (style));
  g_assert (property != NULL);

  if (str_empty0 (value))
    return;

  if (parse_boolean_string (&b, value))
    g_object_set (style, property, b, NULL);
  else
    g_warning ("Failed to parse boolean value: %s", value);
}

static void
parse_color (SchemesScheme *self,
             SchemesStyle  *style,
             const char    *property,
             const char    *value)
{
  GdkRGBA rgba;

  g_assert (SCHEMES_IS_SCHEME (self));
  g_assert (SCHEMES_IS_STYLE (style));
  g_assert (property != NULL);

  if (str_empty0 (value))
    return;

  if ((value[0] == '#' && rgba_parse (&rgba, value)) ||
      schemes_scheme_get_named_color (self, value, &rgba))
    g_object_set (style, property, &rgba, NULL);
  else
    g_warning ("Failed to parse color: %s", value);
}

static void
parse_scale (SchemesScheme *self,
             SchemesStyle  *style,
             const char    *property,
             const char    *value)
{
  double d;

  g_assert (SCHEMES_IS_SCHEME (self));
  g_assert (SCHEMES_IS_STYLE (style));
  g_assert (property != NULL);

  if (str_empty0 (value))
    return;

  if (strcmp (value, "large") == 0)
    d = PANGO_SCALE_LARGE;
  else if (strcmp (value, "x-large") == 0)
    d = PANGO_SCALE_X_LARGE;
  else if (strcmp (value, "xx-large") == 0)
    d = PANGO_SCALE_XX_LARGE;
  else if (strcmp (value, "small") == 0)
    d = PANGO_SCALE_SMALL;
  else if (strcmp (value, "x-small") == 0)
    d = PANGO_SCALE_X_SMALL;
  else if (strcmp (value, "xx-small") == 0)
    d = PANGO_SCALE_XX_SMALL;
  else if (strcmp (value, "medium") == 0)
    d = PANGO_SCALE_MEDIUM;
  else
    d = g_ascii_strtod (value, NULL);

  if (!isnan (d))
    g_object_set (style, property, d, NULL);
  else
    g_warning ("Failed to parse value for scale: %s", value);
}

static void
parse_enum (SchemesScheme *self,
            SchemesStyle  *style,
            GType          type,
            const char    *property,
            const char    *value)
{
  g_autoptr(GEnumClass) klass = NULL;
  const GEnumValue *info;

  g_assert (SCHEMES_IS_SCHEME (self));
  g_assert (SCHEMES_IS_STYLE (style));
  g_assert (property != NULL);

  if (str_empty0 (value))
    return;

  klass = g_type_class_ref (type);
  info = g_enum_get_value_by_nick (klass, value);

  if (info != NULL)
    {
      g_object_set (style, property, info->value, NULL);
      return;
    }

  if (type == PANGO_TYPE_UNDERLINE)
    {
      gboolean b;

      if (parse_boolean_string (&b, value))
        {
          g_object_set (style,
                        property, b ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE,
                        NULL);
          return;
        }

      g_warning ("Failed to parse %s\n",value);
    }
  else if (type == PANGO_TYPE_WEIGHT)
    {
      int ival = atoi (value);

      if (ival != 0)
        {
          g_object_set (style, "weight", ival, NULL);
          return;
        }
    }

  g_warning ("Failed to parse %s enum %s",
             G_OBJECT_CLASS_NAME (klass), value);
}

static void
scheme_start_element (GMarkupParseContext  *context,
                      const char           *element_name,
                      const char          **attribute_names,
                      const char          **attribute_values,
                      gpointer              user_data,
                      GError              **error)
{
  SchemesScheme *self = user_data;

  g_assert (context != NULL);
  g_assert (element_name != NULL);
  g_assert (SCHEMES_IS_SCHEME (self));

  if (g_strcmp0 (element_name, "author") == 0 ||
      g_strcmp0 (element_name, "_description") == 0 ||
      g_strcmp0 (element_name, "description") == 0)
    {
      self->element_name = g_intern_string (element_name);
      g_markup_parse_context_push (context, &text_parser, self);
    }
  else if (g_strcmp0 (element_name, "metadata") == 0)
    g_markup_parse_context_push (context, &metadata_parser, self);
  else if (g_strcmp0 (element_name, "color") == 0)
    {
      const char *name = NULL;
      const char *value = NULL;
      GdkRGBA rgba;

      if (!g_markup_collect_attributes (element_name, attribute_names, attribute_values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_STRING, "value", &value,
                                        G_MARKUP_COLLECT_INVALID))
        return;

      /* Skip past # for rgb. GtkSourceView doesn't support this,
       * but we can trivially in the future and this can help ensure
       * that we still process things correctly.
       */
      if (g_str_has_prefix (value, "#rgb"))
        value++;

      if (gdk_rgba_parse (&rgba, value))
        schemes_scheme_add_color_simple (self, name, &rgba);
      else
        XML_PARSER_ERROR ();
    }
  else if (g_strcmp0 (element_name, "style") == 0)
    {
      const char *name = NULL;
      const char *foreground = NULL;
      const char *background = NULL;
      const char *line_background = NULL;
      const char *bold = NULL;
      const char *italic = NULL;
      const char *weight = NULL;
      const char *underline = NULL;
      const char *underline_color = NULL;
      const char *use_style = NULL;
      const char *strikethrough = NULL;
      const char *scale = NULL;
      SchemesStyle *style;

      if (!g_markup_collect_attributes (element_name, attribute_names, attribute_values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL, "foreground", &foreground,
                                        G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL, "background", &background,
                                        G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL, "line-background", &line_background,
                                        G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL, "bold", &bold,
                                        G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL, "italic", &italic,
                                        G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL, "scale", &scale,
                                        G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL, "weight", &weight,
                                        G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL, "underline", &underline,
                                        G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL, "underline-color", &underline_color,
                                        G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL, "use-style", &use_style,
                                        G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL, "strikethrough", &strikethrough,
                                        G_MARKUP_COLLECT_INVALID))
        return;

      style = schemes_scheme_get_style (self, name);

      parse_color (self, style, "foreground", foreground);
      parse_color (self, style, "background", background);
      parse_color (self, style, "line-background", line_background);
      parse_color (self, style, "underline-color", underline_color);
      parse_boolean (self, style, "bold", bold);
      parse_boolean (self, style, "italic", italic);
      parse_boolean (self, style, "strikethrough", strikethrough);
      parse_enum (self, style, PANGO_TYPE_WEIGHT, "weight", weight);
      parse_enum (self, style, PANGO_TYPE_UNDERLINE, "underline", underline);
      parse_scale (self, style, "scale", scale);

      if (!str_empty0 (use_style))
        g_object_set (style, "use-style", use_style, NULL);
    }
  else
    XML_PARSER_ERROR ();
}

static void
scheme_end_element (GMarkupParseContext  *context,
                    const char           *element_name,
                    gpointer              user_data,
                    GError              **error)
{
  SchemesScheme *self = user_data;

  g_assert (context != NULL);
  g_assert (element_name != NULL);
  g_assert (SCHEMES_IS_SCHEME (self));

  self->element_name = NULL;

  if (g_strcmp0 (element_name, "author") == 0 ||
      g_strcmp0 (element_name, "_description") == 0 ||
      g_strcmp0 (element_name, "description") == 0 ||
      g_strcmp0 (element_name, "metadata") == 0)
    {
      g_markup_parse_context_pop (context);
      return;
    }
  else if (g_strcmp0 (element_name, "color") == 0 ||
           g_strcmp0 (element_name, "style") == 0)
    {
      return;
    }

  XML_PARSER_ERROR ();
}

static const GMarkupParser scheme_parser = {
  scheme_start_element,
  scheme_end_element,
};

static void
root_start_element (GMarkupParseContext  *context,
                    const char           *element_name,
                    const char          **attribute_names,
                    const char          **attribute_values,
                    gpointer              user_data,
                    GError              **error)
{
  SchemesScheme *self = user_data;

  g_assert (context != NULL);
  g_assert (element_name != NULL);
  g_assert (SCHEMES_IS_SCHEME (self));

  if (g_strcmp0 (element_name, "style-scheme") == 0)
    {
      const char *id = NULL;
      const char *name = NULL;
      const char *_name = NULL;
      const char *version = NULL;

      if (!g_markup_collect_attributes (element_name, attribute_names, attribute_values, error,
                                        G_MARKUP_COLLECT_STRING, "id", &id,
                                        G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL, "_name", &_name,
                                        G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL, "name", &name,
                                        G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL, "version", &version,
                                        G_MARKUP_COLLECT_INVALID))
        return;

      schemes_scheme_set_id (self, id);

      if (!str_empty0 (_name))
        schemes_scheme_set_name (self, _name);
      else if (!str_empty0 (name))
        schemes_scheme_set_name (self, name);

      if (str_empty0 (version))
        version = NULL;

      if (g_strcmp0 (version, self->version) != 0)
        {
          g_free (self->version);
          self->version = g_strdup (version);
        }

      g_markup_parse_context_push (context, &scheme_parser, self);
    }
  else
    XML_PARSER_ERROR ();
}

static void
root_end_element (GMarkupParseContext  *context,
                  const char           *element_name,
                  gpointer              user_data,
                  GError              **error)
{
  SchemesScheme *self = user_data;

  g_assert (context != NULL);
  g_assert (element_name != NULL);
  g_assert (SCHEMES_IS_SCHEME (self));

  if (g_strcmp0 (element_name, "style-scheme") == 0)
    g_markup_parse_context_pop (context);
  else
    XML_PARSER_ERROR ();
}

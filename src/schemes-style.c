/* schemes-style.c
 *
 * Copyright 2022 Christian Hergert <chergert@redhat.com>
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

#include <math.h>

#include "schemes-style.h"
#include "schemes-xml.h"

struct _SchemesStyle
{
  GObject parent_instance;
  char *name;
  char *language;
  char *use_style;
  GdkRGBA foreground;
  GdkRGBA background;
  GdkRGBA line_background;
  GdkRGBA underline_color;
  PangoUnderline underline;
  PangoWeight weight;
  double scale;

  guint bold : 1;
  guint italic : 1;
  guint strikethrough : 1;

  guint strikethrough_set : 1;
  guint background_set : 1;
  guint bold_set : 1;
  guint foreground_set : 1;
  guint italic_set : 1;
  guint line_background_set : 1;
  guint scale_set : 1;
  guint underline_color_set : 1;
  guint underline_set : 1;
  guint weight_set : 1;
  guint use_style_set : 1;
};

G_DEFINE_FINAL_TYPE (SchemesStyle, schemes_style, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_BACKGROUND,
  PROP_BACKGROUND_SET,
  PROP_BOLD,
  PROP_BOLD_SET,
  PROP_FOREGROUND,
  PROP_FOREGROUND_SET,
  PROP_IS_EMPTY,
  PROP_ITALIC,
  PROP_ITALIC_SET,
  PROP_LINE_BACKGROUND,
  PROP_LINE_BACKGROUND_SET,
  PROP_SCALE,
  PROP_SCALE_SET,
  PROP_STRIKETHROUGH,
  PROP_STRIKETHROUGH_SET,
  PROP_UNDERLINE,
  PROP_UNDERLINE_SET,
  PROP_UNDERLINE_COLOR,
  PROP_UNDERLINE_COLOR_SET,
  PROP_USE_STYLE,
  PROP_USE_STYLE_SET,
  PROP_WEIGHT,
  PROP_WEIGHT_SET,
  PROP_NAME,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

static gboolean
set_rgba (GdkRGBA       *rgba,
          const GdkRGBA *val)
{
  if (val == NULL)
    return FALSE;
  *rgba = *val;
  return TRUE;
}

SchemesStyle *
schemes_style_new (const char *name)
{
  return g_object_new (SCHEMES_TYPE_STYLE,
                       "name", name,
                       NULL);
}

const char *
schemes_style_get_name (SchemesStyle *self)
{
  g_return_val_if_fail (SCHEMES_IS_STYLE (self), NULL);

  return self->name;
}

static void
on_notify_cb (SchemesStyle *self,
              GParamSpec   *pspec)
{
  if (g_str_has_suffix (pspec->name, "-set"))
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_IS_EMPTY]);
}

static void
schemes_style_finalize (GObject *object)
{
  SchemesStyle *self = (SchemesStyle *)object;

  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->language, g_free);
  g_clear_pointer (&self->use_style, g_free);

  G_OBJECT_CLASS (schemes_style_parent_class)->finalize (object);
}

static void
schemes_style_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  SchemesStyle *self = SCHEMES_STYLE (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;

    case PROP_IS_EMPTY:
      g_value_set_boolean (value, schemes_style_is_empty (self));
      break;

    case PROP_BACKGROUND:
      if (self->background_set)
        g_value_set_boxed (value, &self->background);
      break;

    case PROP_BACKGROUND_SET:
      g_value_set_boolean (value, self->background_set);
      break;

    case PROP_BOLD:
      g_value_set_boolean (value, self->bold);
      break;

    case PROP_BOLD_SET:
      g_value_set_boolean (value, self->bold_set);
      break;

    case PROP_FOREGROUND:
      if (self->foreground_set)
        g_value_set_boxed (value, &self->foreground);
      break;

    case PROP_FOREGROUND_SET:
      g_value_set_boolean (value, self->foreground_set);
      break;

    case PROP_ITALIC:
      g_value_set_boolean (value, self->italic);
      break;

    case PROP_ITALIC_SET:
      g_value_set_boolean (value, self->italic_set);
      break;

    case PROP_STRIKETHROUGH:
      g_value_set_boolean (value, self->strikethrough);
      break;

    case PROP_STRIKETHROUGH_SET:
      g_value_set_boolean (value, self->strikethrough_set);
      break;

    case PROP_LINE_BACKGROUND:
      if (self->line_background_set)
        g_value_set_boxed (value, &self->line_background);
      break;

    case PROP_LINE_BACKGROUND_SET:
      g_value_set_boolean (value, self->line_background_set);
      break;

    case PROP_SCALE:
      g_value_set_double (value, self->scale);
      break;

    case PROP_SCALE_SET:
      g_value_set_boolean (value, self->scale_set);
      break;

    case PROP_UNDERLINE_COLOR:
      g_value_set_boxed (value, &self->underline_color);
      break;

    case PROP_UNDERLINE:
      g_value_set_enum (value, self->underline);
      break;

    case PROP_UNDERLINE_SET:
      g_value_set_boolean (value, self->underline_set);
      break;

    case PROP_UNDERLINE_COLOR_SET:
      g_value_set_boolean (value, self->underline_color_set);
      break;

    case PROP_WEIGHT:
      g_value_set_enum (value, self->weight);
      break;

    case PROP_WEIGHT_SET:
      g_value_set_boolean (value, self->weight_set);
      break;

    case PROP_USE_STYLE:
      g_value_set_string (value, self->use_style);
      break;

    case PROP_USE_STYLE_SET:
      g_value_set_boolean (value, self->use_style_set);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
schemes_style_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  SchemesStyle *self = SCHEMES_STYLE (object);

  switch (prop_id)
    {
    case PROP_NAME:
      self->name = g_value_dup_string (value);
      if (strchr (self->name, ':'))
        self->language = g_strndup (self->name, strchr (self->name, ':') - self->name);
      break;

    case PROP_BACKGROUND:
      self->background_set = set_rgba (&self->background, g_value_get_boxed (value));
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_BACKGROUND_SET]);
      break;

    case PROP_FOREGROUND:
      self->foreground_set = set_rgba (&self->foreground, g_value_get_boxed (value));
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_FOREGROUND_SET]);
      break;

    case PROP_UNDERLINE_COLOR:
      self->underline_color_set = set_rgba (&self->underline_color, g_value_get_boxed (value));
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_UNDERLINE_COLOR_SET]);
      break;

    case PROP_BOLD:
      self->bold = g_value_get_boolean (value);
      self->bold_set = TRUE;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_BOLD_SET]);
      break;

    case PROP_ITALIC:
      self->italic = g_value_get_boolean (value);
      self->italic_set = TRUE;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ITALIC_SET]);
      break;

    case PROP_LINE_BACKGROUND:
      self->line_background_set = set_rgba (&self->line_background, g_value_get_boxed (value));
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_LINE_BACKGROUND_SET]);
      break;

    case PROP_SCALE:
      self->scale = g_value_get_double (value);
      self->scale_set = TRUE;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SCALE_SET]);
      break;

    case PROP_STRIKETHROUGH:
      self->strikethrough = g_value_get_boolean (value);
      self->strikethrough_set = TRUE;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_STRIKETHROUGH_SET]);
      break;

    case PROP_UNDERLINE:
      self->underline = g_value_get_enum (value);
      self->underline_set = TRUE;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_UNDERLINE_SET]);
      break;

    case PROP_WEIGHT:
      if (g_value_get_enum (value) != self->weight)
        {
          self->weight = g_value_get_enum (value);
          self->weight_set = TRUE;
          g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_WEIGHT_SET]);
        }
      break;

    case PROP_BACKGROUND_SET:
      self->background_set = g_value_get_boolean (value);
      break;

    case PROP_BOLD_SET:
      self->bold_set = g_value_get_boolean (value);
      break;

    case PROP_FOREGROUND_SET:
      self->foreground_set = g_value_get_boolean (value);
      break;

    case PROP_UNDERLINE_SET:
      self->underline_set = g_value_get_boolean (value);
      break;

    case PROP_UNDERLINE_COLOR_SET:
      self->underline_color_set = g_value_get_boolean (value);
      break;

    case PROP_ITALIC_SET:
      self->italic_set = g_value_get_boolean (value);
      break;

    case PROP_STRIKETHROUGH_SET:
      self->strikethrough_set = g_value_get_boolean (value);
      break;

    case PROP_LINE_BACKGROUND_SET:
      self->line_background_set = g_value_get_boolean (value);
      break;

    case PROP_SCALE_SET:
      self->scale_set = g_value_get_boolean (value);
      break;

    case PROP_WEIGHT_SET:
      self->weight_set = g_value_get_boolean (value);
      break;

    case PROP_USE_STYLE:
      g_free (self->use_style);
      self->use_style = g_value_dup_string (value);
      self->use_style_set = TRUE;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_USE_STYLE_SET]);
      break;

    case PROP_USE_STYLE_SET:
      self->use_style_set = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
schemes_style_class_init (SchemesStyleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = schemes_style_finalize;
  object_class->get_property = schemes_style_get_property;
  object_class->set_property = schemes_style_set_property;

  properties [PROP_NAME] =
    g_param_spec_string ("name", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  properties [PROP_IS_EMPTY] =
    g_param_spec_boolean ("is-empty", NULL, NULL,
                          FALSE,
                          (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties [PROP_BOLD] =
    g_param_spec_boolean ("bold", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_BOLD_SET] =
    g_param_spec_boolean ("bold-set", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_FOREGROUND] =
    g_param_spec_boxed ("foreground", NULL, NULL,
                        GDK_TYPE_RGBA,
                        (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_BACKGROUND] =
    g_param_spec_boxed ("background", NULL, NULL,
                        GDK_TYPE_RGBA,
                        (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_BACKGROUND_SET] =
    g_param_spec_boolean ("background-set", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_LINE_BACKGROUND] =
    g_param_spec_boxed ("line-background", NULL, NULL,
                        GDK_TYPE_RGBA,
                        (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_LINE_BACKGROUND_SET] =
    g_param_spec_boolean ("line-background-set", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_FOREGROUND_SET] =
    g_param_spec_boolean ("foreground-set", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_ITALIC] =
    g_param_spec_boolean ("italic", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_ITALIC_SET] =
    g_param_spec_boolean ("italic-set", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_SCALE] =
    g_param_spec_double ("scale", NULL, NULL,
                         -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_SCALE_SET] =
    g_param_spec_boolean ("scale-set", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_STRIKETHROUGH] =
    g_param_spec_boolean ("strikethrough", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_STRIKETHROUGH_SET] =
    g_param_spec_boolean ("strikethrough-set", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_UNDERLINE_COLOR] =
    g_param_spec_boxed ("underline-color", NULL, NULL,
                        GDK_TYPE_RGBA,
                        (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_UNDERLINE_COLOR_SET] =
    g_param_spec_boolean ("underline-color-set", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_UNDERLINE] =
    g_param_spec_enum ("underline", NULL, NULL,
                       PANGO_TYPE_UNDERLINE,
                       PANGO_UNDERLINE_NONE,
                       (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_UNDERLINE_SET] =
    g_param_spec_boolean ("underline-set", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_WEIGHT] =
    g_param_spec_enum ("weight", NULL, NULL,
                       PANGO_TYPE_WEIGHT,
                       PANGO_WEIGHT_NORMAL,
                       (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_WEIGHT_SET] =
    g_param_spec_boolean ("weight-set", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_USE_STYLE] =
    g_param_spec_string ("use-style", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_USE_STYLE_SET] =
    g_param_spec_boolean ("use-style-set", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
schemes_style_init (SchemesStyle *self)
{
  self->scale = 1.0;
  self->weight = PANGO_WEIGHT_NORMAL;

  g_signal_connect (self,
                    "notify",
                    G_CALLBACK (on_notify_cb),
                    NULL);
}

gboolean
schemes_style_is_empty (SchemesStyle *self)
{
  g_return_val_if_fail (SCHEMES_IS_STYLE (self), FALSE);

  /* TODO */

  return !(self->background_set ||
           self->foreground_set ||
           self->italic_set ||
           self->bold_set ||
           self->scale_set ||
           self->line_background_set ||
           self->use_style_set ||
           self->strikethrough_set ||
           self->underline_set ||
           self->underline_color_set ||
           self->weight_set);
}

static void
write_color_attribute (GString       *string,
                       const char    *key,
                       const GdkRGBA *color,
                       GHashTable    *colors)
{
  g_autofree char *color_str = NULL;
  g_autofree char *hash_color_str = NULL;
  const char *name;

  g_assert (string != NULL);
  g_assert (key != NULL);
  g_assert (color != NULL);
  g_assert (colors != NULL);

  color_str = gdk_rgba_to_string (color);

  if ((name = g_hash_table_lookup (colors, color_str)))
    {
      schemes_xml_writer_add_attribute (string, key, name);
      return;
    }

  if (color->alpha >= 1.0)
    hash_color_str = g_strdup_printf ("#%02X%02X%02X",
                                      (int)roundf (color->red*255.0),
                                      (int)roundf (color->green*255.0),
                                      (int)roundf (color->blue*255.0));
  else
    hash_color_str = g_strdup_printf ("#%s", color_str);

  schemes_xml_writer_add_attribute (string, key, hash_color_str);
}

static void
write_weight_attribute (GString      *string,
                        const char   *key,
                        PangoWeight   weight)
{
  g_autofree char *freeme = NULL;
  const char *str = NULL;

  switch (weight)
    {
    case PANGO_WEIGHT_THIN:
      str = "thin";
      break;

    case PANGO_WEIGHT_BOLD:
      str = "bold";
      break;

    case PANGO_WEIGHT_ULTRABOLD:
      str = "ultrabold";
      break;

    case PANGO_WEIGHT_ULTRAHEAVY:
      str = "ultraheavy";
      break;

    case PANGO_WEIGHT_ULTRALIGHT:
      str = "ultralight";
      break;

    case PANGO_WEIGHT_LIGHT:
      str = "light";
      break;

    case PANGO_WEIGHT_HEAVY:
      str = "heavy";
      break;

    case PANGO_WEIGHT_NORMAL:
      str = "normal";
      break;

    case PANGO_WEIGHT_SEMIBOLD:
      str = "semibold";
      break;

    case PANGO_WEIGHT_SEMILIGHT:
      str = "semilight";
      break;

    case PANGO_WEIGHT_BOOK:
      str = "book";
      break;

    case PANGO_WEIGHT_MEDIUM:
      str = "medium";
      break;

    default:
      str = freeme = g_strdup_printf ("%d", weight);
      break;
    }

  schemes_xml_writer_add_attribute (string, key, str);
}

static inline void
write_enum_attribute (GString    *string,
                      GType       type,
                      const char *name,
                      int         value)
{
  GEnumClass *klass = g_type_class_ref (type);
  const GEnumValue *eval = g_enum_get_value (klass, value);

  if (eval != NULL)
    schemes_xml_writer_add_attribute (string, name, eval->value_nick);

  g_type_class_unref (klass);
}

static inline void
write_boolean_attribute (GString    *string,
                         const char *name,
                         gboolean    value)
{
  const char *valstr = value ? "true" : "false";
  schemes_xml_writer_add_attribute (string, name, valstr);
}

static inline void
write_double_attribute (GString    *string,
                        const char *name,
                        double      value)
{
  char str[G_ASCII_DTOSTR_BUF_SIZE];
  g_ascii_dtostr (str, sizeof str, value);
  schemes_xml_writer_add_attribute (string, name, str);
}

void
schemes_style_serialize (SchemesStyle *self,
                         GString      *string,
                         GHashTable   *colors,
                         guint         longest_style_name)
{
  guint name_len;

  g_return_if_fail (SCHEMES_IS_STYLE (self));
  g_return_if_fail (string != NULL);

  if (schemes_style_is_empty (self))
    return;

  schemes_xml_writer_begin_open_element (string, "style");
  schemes_xml_writer_add_attribute (string, "name", self->name);

  /* Align first attribute (which is often all we have) */
  name_len = strlen (self->name);
  if (name_len < longest_style_name)
    {
      guint diff = longest_style_name - name_len;
      for (guint i = 0; i < diff; i++)
        g_string_append_c (string, ' ');
    }

  if (self->background_set)
    write_color_attribute (string, "background", &self->background, colors);

  if (self->foreground_set)
    write_color_attribute (string, "foreground", &self->foreground, colors);

  if (self->line_background_set)
    write_color_attribute (string, "line-background", &self->line_background, colors);

  if (self->bold_set)
    write_boolean_attribute (string, "bold", self->bold);

  if (self->weight_set)
    write_weight_attribute (string, "weight", self->weight);

  if (self->italic_set)
    write_boolean_attribute (string, "italic", self->italic);

  if (self->underline_set)
    write_enum_attribute (string, PANGO_TYPE_UNDERLINE, "underline", self->underline);

  if (self->underline_color_set)
    write_color_attribute (string, "underline-color", &self->underline_color, colors);

  if (self->scale_set)
    write_double_attribute (string, "scale", self->scale);

  if (self->strikethrough_set)
    write_boolean_attribute (string, "strikethrough", self->strikethrough);

  if (self->use_style_set)
    schemes_xml_writer_add_attribute (string, "use-style", self->use_style);

  schemes_xml_writer_end_open_element (string, FALSE);
}

const char *
schemes_style_get_language (SchemesStyle *self)
{
  g_return_val_if_fail (SCHEMES_IS_STYLE (self), NULL);

  return self->language;
}

void
schemes_style_replace_color (SchemesStyle  *self,
                             const GdkRGBA *previous_color,
                             const GdkRGBA *new_color)
{
  g_return_if_fail (SCHEMES_IS_STYLE (self));
  g_return_if_fail (previous_color != NULL);
  g_return_if_fail (new_color != NULL);

  if (gdk_rgba_equal (&self->foreground, previous_color))
    {
      self->foreground = *new_color;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_FOREGROUND]);
    }

  if (gdk_rgba_equal (&self->background, previous_color))
    {
      self->background = *new_color;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_BACKGROUND]);
    }

  if (gdk_rgba_equal (&self->line_background, previous_color))
    {
      self->line_background = *new_color;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_LINE_BACKGROUND]);
    }

  if (gdk_rgba_equal (&self->underline_color, previous_color))
    {
      self->underline_color = *new_color;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_UNDERLINE_COLOR]);
    }
}

const char *
schemes_style_get_use_style (SchemesStyle *self)
{
  g_return_val_if_fail (SCHEMES_IS_STYLE (self), NULL);

  if (self->use_style_set)
    return self->use_style;

  return NULL;
}

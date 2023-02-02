/* schemes-color.c
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

#include "schemes-color.h"

struct _SchemesColor
{
  GObject parent_instance;
  char *name;
  GdkRGBA color;
  guint color_set : 1;
};

G_DEFINE_TYPE (SchemesColor, schemes_color, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_NAME,
  PROP_COLOR,
  N_PROPS
};

enum {
  COLOR_CHANGED,
  N_SIGNALS
};

static GParamSpec *properties [N_PROPS];
static guint signals [N_SIGNALS];

SchemesColor *
schemes_color_new (const char    *name,
                   const GdkRGBA *color)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (color != NULL, NULL);

  return g_object_new (SCHEMES_TYPE_COLOR,
                       "name", name,
                       "color", color,
                       NULL);
}

static void
schemes_color_set_color (SchemesColor  *self,
                         const GdkRGBA *color)
{
  static const GdkRGBA transparent;
  GdkRGBA previous;

  g_assert (SCHEMES_IS_COLOR (self));

  if (color && self->color_set && gdk_rgba_equal (color, &self->color))
    return;

  previous = self->color;

  self->color_set = color != NULL;
  self->color = color ? *color : transparent;

  g_signal_emit (self, signals [COLOR_CHANGED], 0, &previous);
  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_COLOR]);
}

static void
schemes_color_finalize (GObject *object)
{
  SchemesColor *self = (SchemesColor *)object;

  g_clear_pointer (&self->name, g_free);

  G_OBJECT_CLASS (schemes_color_parent_class)->finalize (object);
}

static void
schemes_color_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  SchemesColor *self = SCHEMES_COLOR (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, schemes_color_get_name (self));
      break;

    case PROP_COLOR:
      g_value_set_boxed (value, schemes_color_get_color (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
schemes_color_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  SchemesColor *self = SCHEMES_COLOR (object);

  switch (prop_id)
    {
    case PROP_NAME:
      self->name = g_value_dup_string (value);
      break;

    case PROP_COLOR:
      schemes_color_set_color (self, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
schemes_color_class_init (SchemesColorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = schemes_color_finalize;
  object_class->get_property = schemes_color_get_property;
  object_class->set_property = schemes_color_set_property;

  properties [PROP_NAME] =
    g_param_spec_string ("name",
                         "Name",
                         "Name",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  properties [PROP_COLOR] =
    g_param_spec_boxed ("color",
                        "Color",
                        "The RGBA color as a GdkRGBA",
                        GDK_TYPE_RGBA,
                        (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  signals [COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, GDK_TYPE_RGBA | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static void
schemes_color_init (SchemesColor *self)
{
}

const char *
schemes_color_get_name (SchemesColor *self)
{
  g_return_val_if_fail (SCHEMES_IS_COLOR (self), NULL);

  return self->name;
}

const GdkRGBA *
schemes_color_get_color (SchemesColor *self)
{
  g_return_val_if_fail (SCHEMES_IS_COLOR (self), NULL);

  if (self->color_set)
    return &self->color;

  return NULL;
}

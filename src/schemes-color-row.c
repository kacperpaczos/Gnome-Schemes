/* schemes-color-row.c
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

#include "schemes-color-row.h"

struct _SchemesColorRow
{
  AdwActionRow parent_instance;
  SchemesColor *color;
  GtkLabel *label;
  GtkColorButton *button;
};

G_DEFINE_TYPE (SchemesColorRow, schemes_color_row, ADW_TYPE_ACTION_ROW)

enum {
  PROP_0,
  PROP_COLOR,
  N_PROPS
};

enum {
  REMOVE,
  N_SIGNALS
};

static GParamSpec *properties [N_PROPS];
static guint signals [N_SIGNALS];

GtkWidget *
schemes_color_row_new (SchemesColor *color)
{
  g_return_val_if_fail (SCHEMES_IS_COLOR (color), NULL);

  return g_object_new (SCHEMES_TYPE_COLOR_ROW,
                       "color", color,
                       NULL);
}

static void
schemes_color_row_set_color (SchemesColorRow *self,
                             SchemesColor    *color)
{
  g_assert (SCHEMES_IS_COLOR_ROW (self));
  g_assert (!color || SCHEMES_IS_COLOR (color));

  if (g_set_object (&self->color, color))
    {
      if (color)
        {
          g_object_bind_property (color, "name", self, "title",
                                  G_BINDING_SYNC_CREATE);
          g_object_bind_property (color, "color", self->button, "rgba",
                                  G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
        }
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_COLOR]);
    }
}

static void
on_remove_clicked_cb (SchemesColorRow *self,
                      GtkButton       *button)
{
  g_assert (SCHEMES_IS_COLOR_ROW (self));
  g_assert (GTK_IS_BUTTON (button));

  g_signal_emit (self, signals[REMOVE], 0);
}

static void
schemes_color_row_dispose (GObject *object)
{
  G_OBJECT_CLASS (schemes_color_row_parent_class)->dispose (object);
}

static void
schemes_color_row_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  SchemesColorRow *self = SCHEMES_COLOR_ROW (object);

  switch (prop_id)
    {
    case PROP_COLOR:
      g_value_set_object (value, schemes_color_row_get_color (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
schemes_color_row_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  SchemesColorRow *self = SCHEMES_COLOR_ROW (object);

  switch (prop_id)
    {
    case PROP_COLOR:
      schemes_color_row_set_color (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
schemes_color_row_class_init (SchemesColorRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = schemes_color_row_dispose;
  object_class->get_property = schemes_color_row_get_property;
  object_class->set_property = schemes_color_row_set_property;

  properties [PROP_COLOR] =
    g_param_spec_object ("color",
                        "Color",
                        "A SchemesColor",
                        SCHEMES_TYPE_COLOR,
                        (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  signals [REMOVE] = g_signal_new ("remove",
                                   G_TYPE_FROM_CLASS (klass),
                                   G_SIGNAL_RUN_LAST,
                                   0,
                                   NULL, NULL,
                                   NULL,
                                   G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class, "/ui/schemes-color-row.ui");
  gtk_widget_class_bind_template_child (widget_class, SchemesColorRow, label);
  gtk_widget_class_bind_template_child (widget_class, SchemesColorRow, button);
  gtk_widget_class_bind_template_callback (widget_class, on_remove_clicked_cb);
}

static gboolean
rgba_to_string (GBinding     *binding,
                const GValue *value,
                GValue       *to_value,
                gpointer      user_data)
{
  g_value_take_string (to_value, gdk_rgba_to_string (g_value_get_boxed (value)));
  return TRUE;
}

static void
schemes_color_row_init (SchemesColorRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_object_bind_property_full (self->button, "rgba", self->label, "label", 0,
                               rgba_to_string, NULL, NULL, NULL);
}

SchemesColor *
schemes_color_row_get_color (SchemesColorRow *self)
{
  g_return_val_if_fail (SCHEMES_IS_COLOR_ROW (self), NULL);

  return self->color;
}

/* schemes-style-row.c
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

#include <glib/gi18n.h>

#include "schemes-style-row.h"
#include "schemes-window.h"

struct _SchemesStyleRow
{
  AdwExpanderRow       parent_instance;

  SchemesStyle        *style;
  GtkGrid             *grid;
  GtkImage            *modified;

  SchemesStyleOptions  options;
};

G_DEFINE_FINAL_TYPE (SchemesStyleRow, schemes_style_row, ADW_TYPE_EXPANDER_ROW)

static gboolean
null_to_transparent (GBinding     *binding,
                     const GValue *from_value,
                     GValue       *to_value,
                     gpointer      user_data)
{
  const GdkRGBA transparent = {0};

  if (g_value_get_boxed (from_value) == NULL)
    g_value_set_boxed (to_value, &transparent);
  else
    g_value_set_boxed (to_value, g_value_get_boxed (from_value));

  return TRUE;
}

static void
unset_cb (GtkWidget    *widget,
          SchemesStyle *style)
{
  const char *property = g_object_get_data (G_OBJECT (widget), "PROPERTY");
  const char *property_set = g_object_get_data (G_OBJECT (widget), "PROPERTY_SET");
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (style), property);

  if (g_type_is_a (pspec->value_type, GDK_TYPE_RGBA))
    {
      static const GdkRGBA transparent = {0};
      g_object_set (style, property, &transparent, NULL);
    }
  else if (G_IS_PARAM_SPEC_BOOLEAN (pspec))
    g_object_set (style, property, ((GParamSpecBoolean *)pspec)->default_value, NULL);
  else if (G_IS_PARAM_SPEC_ENUM (pspec))
    g_object_set (style, property, ((GParamSpecEnum *)pspec)->default_value, NULL);
  else if (G_IS_PARAM_SPEC_FLAGS (pspec))
    g_object_set (style, property, ((GParamSpecFlags *)pspec)->default_value, NULL);
  else if (G_IS_PARAM_SPEC_STRING (pspec))
    g_object_set (style, property, ((GParamSpecString *)pspec)->default_value, NULL);

  g_object_set (style, property_set, FALSE, NULL);
}

static void
connect_unset (GtkWidget    *button,
               SchemesStyle *style,
               const char   *property,
               const char   *property_set)
{
  g_object_set_data (G_OBJECT (button), "PROPERTY", (char *)g_intern_string (property));
  g_object_set_data (G_OBJECT (button), "PROPERTY_SET", (char *)g_intern_string (property_set));
  g_signal_connect_object (button,
                           "clicked",
                           G_CALLBACK (unset_cb),
                           style,
                           0);
}

static void
on_color_clicked_cb (GtkButton       *button,
                     SchemesStyleRow *self)
{
  g_autoptr(GArray) color_ar = NULL;
  GtkColorChooser *chooser;
  SchemesScheme *scheme;
  GListModel *colors;
  GtkWidget *window;
  guint n_colors;

  g_assert (GTK_IS_BUTTON (button));
  g_assert (SCHEMES_IS_STYLE_ROW (self));

  chooser = GTK_COLOR_CHOOSER (gtk_widget_get_parent (GTK_WIDGET (button)));
  color_ar = g_array_new (FALSE, FALSE, sizeof (GdkRGBA));

  window = gtk_widget_get_ancestor (GTK_WIDGET (self), SCHEMES_TYPE_WINDOW);
  scheme = schemes_window_get_scheme (SCHEMES_WINDOW (window));
  colors = schemes_scheme_get_colors (scheme);
  n_colors = g_list_model_get_n_items (colors);

  for (guint i = 0; i < n_colors; i++)
    {
      g_autoptr(SchemesColor) color = g_list_model_get_item (colors, i);
      const GdkRGBA *rgba = schemes_color_get_color (color);

      g_array_append_vals (color_ar, rgba, 1);
    }

  if (color_ar->len > 0)
    {
      gtk_color_chooser_add_palette (chooser, GTK_ORIENTATION_HORIZONTAL, 10, 0, NULL);
      gtk_color_chooser_add_palette (chooser, GTK_ORIENTATION_HORIZONTAL, 10,
                                     n_colors, (GdkRGBA *)(gpointer)color_ar->data);
    }
}

static GtkWidget *
create_unset_button (const char *title)
{
  g_autofree char *tooltip = NULL;
  GtkWidget *button;

  button = gtk_button_new ();
  gtk_button_set_icon_name (GTK_BUTTON (button), "list-remove-symbolic");
  gtk_widget_add_css_class (button, "circular");
  gtk_widget_add_css_class (button, "flat");

  tooltip = g_strdup_printf (_("Unset %s"), title);
  gtk_widget_set_tooltip_text (button, tooltip);

  return button;
}

static void
add_color (SchemesStyleRow *self,
           guint            row,
           const char      *title,
           const char      *property,
           const char      *property_set)
{
  GtkWidget *color;
  GtkWidget *label;
  GtkWidget *unset;

  g_assert (SCHEMES_IS_STYLE_ROW (self));
  g_assert (title != NULL);
  g_assert (property != NULL);
  g_assert (property_set != NULL);

  color = gtk_color_button_new ();
  label = gtk_label_new (title);
  unset = create_unset_button (title);

  g_signal_connect_object (gtk_widget_get_first_child (color),
                           "clicked",
                           G_CALLBACK (on_color_clicked_cb),
                           self,
                           0);

  gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (color), TRUE);
  gtk_widget_set_valign (color, GTK_ALIGN_CENTER);
  gtk_widget_set_halign (color, GTK_ALIGN_END);
  gtk_widget_set_hexpand (label, TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);

  gtk_grid_attach (self->grid, label, 0, row, 1, 1);
  gtk_grid_attach (self->grid, color, 1, row, 1, 1);
  gtk_grid_attach (self->grid, unset, 2, row, 1, 1);

  connect_unset (unset, self->style, property, property_set);

  g_object_bind_property_full (self->style, property,
                               color, "rgba",
                               G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL,
                               null_to_transparent, NULL, NULL, NULL);
  g_object_bind_property (self->style, property_set,
                          unset, "sensitive",
                          G_BINDING_SYNC_CREATE);
}

static void
add_toggle (SchemesStyleRow *self,
            guint            row,
            const char      *title,
            const char      *property,
            const char      *property_set)
{
  GtkWidget *control;
  GtkWidget *label;
  GtkWidget *unset;

  g_assert (SCHEMES_IS_STYLE_ROW (self));
  g_assert (title != NULL);
  g_assert (property != NULL);
  g_assert (property_set != NULL);

  control = gtk_switch_new ();
  label = gtk_label_new (title);
  unset = create_unset_button (title);

  gtk_widget_set_valign (control, GTK_ALIGN_CENTER);
  gtk_widget_set_halign (control, GTK_ALIGN_END);
  gtk_widget_set_hexpand (label, TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);

  gtk_grid_attach (self->grid, label, 0, row, 1, 1);
  gtk_grid_attach (self->grid, control, 1, row, 1, 1);
  gtk_grid_attach (self->grid, unset, 2, row, 1, 1);

  connect_unset (unset, self->style, property, property_set);

  g_object_bind_property (self->style, property,
                          control, "active",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  g_object_bind_property (self->style, property_set,
                          unset, "sensitive",
                          G_BINDING_SYNC_CREATE);
}

static void
add_scale (SchemesStyleRow *self,
           guint            row,
           const char      *title,
           const char      *property,
           const char      *property_set)
{
  GtkWidget *control;
  GtkWidget *label;
  GtkWidget *unset;

  g_assert (SCHEMES_IS_STYLE_ROW (self));
  g_assert (title != NULL);
  g_assert (property != NULL);
  g_assert (property_set != NULL);

  control = gtk_spin_button_new_with_range (0.1, 32.0, 0.1);
  label = gtk_label_new (title);
  unset = create_unset_button (title);

  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (control), 2);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (control), 1.0);
  gtk_widget_set_valign (control, GTK_ALIGN_CENTER);
  gtk_widget_set_halign (control, GTK_ALIGN_END);
  gtk_widget_set_hexpand (label, TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);

  gtk_grid_attach (self->grid, label, 0, row, 1, 1);
  gtk_grid_attach (self->grid, control, 1, row, 1, 1);
  gtk_grid_attach (self->grid, unset, 2, row, 1, 1);

  connect_unset (unset, self->style, property, property_set);

  g_object_bind_property (self->style, property,
                          control, "value",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  g_object_bind_property (self->style, property_set,
                          unset, "sensitive",
                          G_BINDING_SYNC_CREATE);
}

static gboolean
string_to_enum (GBinding     *binding,
                const GValue *from_value,
                GValue       *to_value,
                gpointer      user_data)
{
  GType type = GPOINTER_TO_SIZE (user_data);
  g_autoptr(GEnumClass) klass = g_type_class_ref (type);
  GtkStringObject *so = g_value_get_object (from_value);
  const char *str = gtk_string_object_get_string (so);
  int val = 0;

  for (guint i = 0; i < klass->n_values; i++)
    {
      if (g_strcmp0 (str, klass->values[i].value_nick) == 0)
        {
          val = klass->values[i].value;
          break;
        }
    }

  g_value_set_enum (to_value, val);

  return TRUE;
}

static void
add_enum (SchemesStyleRow *self,
          guint            row,
          const char      *title,
          GType            type,
          const char      *property,
          const char      *property_set)
{
  g_autoptr(GPtrArray) strings = NULL;
  g_autoptr(GEnumClass) klass = NULL;
  GtkWidget *control;
  GtkWidget *label;
  GtkWidget *unset;
  guint value = 0;
  int pos = -1;

  g_assert (SCHEMES_IS_STYLE_ROW (self));
  g_assert (title != NULL);
  g_assert (property != NULL);
  g_assert (property_set != NULL);

  g_object_get (self->style, property, &value, NULL);

  strings = g_ptr_array_new ();
  klass = g_type_class_ref (type);
  for (guint i = 0; i < klass->n_values; i++)
    {
      if (klass->values[i].value == value)
        pos = i;
      g_ptr_array_add (strings, (gpointer)klass->values[i].value_nick);
    }
  g_ptr_array_add (strings, NULL);

  control = gtk_drop_down_new_from_strings ((const char * const *)strings->pdata);
  label = gtk_label_new (title);
  unset = create_unset_button (title);

  gtk_widget_set_valign (control, GTK_ALIGN_CENTER);
  gtk_widget_set_halign (control, GTK_ALIGN_END);
  gtk_widget_set_hexpand (label, TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);

  gtk_grid_attach (self->grid, label, 0, row, 1, 1);
  gtk_grid_attach (self->grid, control, 1, row, 1, 1);
  gtk_grid_attach (self->grid, unset, 2, row, 1, 1);

  connect_unset (unset, self->style, property, property_set);

  g_object_bind_property_full (control, "selected-item",
                               self->style, property,
                               0,
                               string_to_enum, NULL,
                               GSIZE_TO_POINTER (type),
                               NULL);

  if (pos > -1)
    gtk_drop_down_set_selected (GTK_DROP_DOWN (control), pos);

  g_object_bind_property (self->style, property_set,
                          unset, "sensitive",
                          G_BINDING_SYNC_CREATE);
}

static void
add_controls (SchemesStyleRow     *self,
              SchemesStyleOptions  options)
{
  guint row = 0;

  if (options & SCHEMES_STYLE_OPTIONS_HAS_BACKGROUND)
    add_color (self, row++, _("Background"), "background", "background-set");

  if (options & SCHEMES_STYLE_OPTIONS_HAS_FOREGROUND)
    add_color (self, row++, _("Foreground"), "foreground", "foreground-set");

  if (options & SCHEMES_STYLE_OPTIONS_HAS_UNDERLINE_COLOR)
    add_color (self, row++, _("Underline Color"), "underline-color", "underline-color-set");

  if (options & SCHEMES_STYLE_OPTIONS_HAS_LINE_BACKGROUND)
    add_color (self, row++, _("Paragraph Background"), "line-background", "line-background-set");

  if (options & SCHEMES_STYLE_OPTIONS_HAS_BOLD)
    add_toggle (self, row++, _("Bold"), "bold", "bold-set");

  if (options & SCHEMES_STYLE_OPTIONS_HAS_WEIGHT)
    add_enum (self, row++, _("Font Weight"), PANGO_TYPE_WEIGHT, "weight", "weight-set");

  if (options & SCHEMES_STYLE_OPTIONS_HAS_ITALIC)
    add_toggle (self, row++, _("Italic"), "italic", "italic-set");

  if (options & SCHEMES_STYLE_OPTIONS_HAS_UNDERLINE)
    add_enum (self, row++, _("Underline"), PANGO_TYPE_UNDERLINE, "underline", "underline-set");

  if (options & SCHEMES_STYLE_OPTIONS_HAS_STRIKETHROUGH)
    add_toggle (self, row++, _("Strikethrough"), "strikethrough", "strikethrough-set");

  if (options & SCHEMES_STYLE_OPTIONS_HAS_SCALE)
    add_scale (self, row++, _("Font Scale"), "scale", "scale-set");

  /* TODO: underline, weight */
}

static gboolean
empty_to_label (GBinding     *binding,
                const GValue *value,
                GValue       *to_value,
                gpointer      user_data)
{
  if (g_value_get_boolean (value))
    g_value_set_string (to_value, NULL);
  else
    g_value_set_string (to_value, "style-scheme-modified-symbolic");
  return TRUE;
}

GtkWidget *
schemes_style_row_new (const char          *title,
                       const char          *subtitle,
                       SchemesStyleOptions  options,
                       SchemesStyle        *style)
{
  SchemesStyleRow *self;

  self = g_object_new (SCHEMES_TYPE_STYLE_ROW,
                       "title", title,
                       "subtitle", subtitle,
                       NULL);
  self->options = options;
  g_set_object (&self->style, style);
  g_object_bind_property_full (G_OBJECT (style), "is-empty",
                               self->modified, "icon-name",
                               G_BINDING_SYNC_CREATE,
                               empty_to_label, NULL, NULL, NULL);

  add_controls (self, options);

  return GTK_WIDGET (self);
}

static void
schemes_style_row_dispose (GObject *object)
{
  SchemesStyleRow *self = (SchemesStyleRow *)object;

  g_clear_object (&self->style);

  G_OBJECT_CLASS (schemes_style_row_parent_class)->dispose (object);
}

static void
schemes_style_row_class_init (SchemesStyleRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = schemes_style_row_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/ui/schemes-style-row.ui");
  gtk_widget_class_bind_template_child (widget_class, SchemesStyleRow, grid);
  gtk_widget_class_bind_template_child (widget_class, SchemesStyleRow, modified);
}

static void
schemes_style_row_init (SchemesStyleRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

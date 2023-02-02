/* schemes-style.h
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

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SCHEMES_TYPE_STYLE (schemes_style_get_type())

G_DECLARE_FINAL_TYPE (SchemesStyle, schemes_style, SCHEMES, STYLE, GObject)

SchemesStyle *schemes_style_new           (const char    *name);
const char   *schemes_style_get_name      (SchemesStyle  *self);
const char   *schemes_style_get_language  (SchemesStyle  *self);
gboolean      schemes_style_is_empty      (SchemesStyle  *self);
void          schemes_style_serialize     (SchemesStyle  *self,
                                           GString       *string,
                                           GHashTable    *colors,
                                           guint          longest_style_name);
const char   *schemes_style_get_use_style (SchemesStyle  *self);
void          schemes_style_replace_color (SchemesStyle  *self,
                                           const GdkRGBA *previous_color,
                                           const GdkRGBA *new_color);

G_END_DECLS

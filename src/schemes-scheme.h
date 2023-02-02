/* schemes-scheme.h
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

#pragma once

#include <gtksourceview/gtksource.h>

#include "schemes-color.h"
#include "schemes-style.h"

G_BEGIN_DECLS

#define SCHEMES_TYPE_SCHEME (schemes_scheme_get_type())

G_DECLARE_FINAL_TYPE (SchemesScheme, schemes_scheme, SCHEMES, SCHEME, GObject)

SchemesScheme        *schemes_scheme_new             (void);
GFile                *schemes_scheme_get_file        (SchemesScheme  *self);
void                  schemes_scheme_set_file        (SchemesScheme  *self,
                                                      GFile          *file);
const char           *schemes_scheme_get_alternate   (SchemesScheme  *self);
void                  schemes_scheme_set_alternate   (SchemesScheme  *self,
                                                      const char     *alternate);
const char           *schemes_scheme_get_author      (SchemesScheme  *self);
void                  schemes_scheme_set_author      (SchemesScheme  *self,
                                                      const char     *author);
const char           *schemes_scheme_get_id          (SchemesScheme  *self);
void                  schemes_scheme_set_id          (SchemesScheme  *self,
                                                      const char     *id);
const char           *schemes_scheme_get_name        (SchemesScheme  *self);
void                  schemes_scheme_set_name        (SchemesScheme  *self,
                                                      const char     *name);
const char           *schemes_scheme_get_description (SchemesScheme  *self);
void                  schemes_scheme_set_description (SchemesScheme  *self,
                                                      const char     *description);
gboolean              schemes_scheme_get_dark        (SchemesScheme  *self);
void                  schemes_scheme_set_dark        (SchemesScheme  *self,
                                                      gboolean        dark);
gboolean              schemes_scheme_get_named_color (SchemesScheme  *self,
                                                      const char     *name,
                                                      GdkRGBA        *rgba);
GListModel           *schemes_scheme_get_colors      (SchemesScheme  *self);
void                  schemes_scheme_add_color       (SchemesScheme  *self,
                                                      SchemesColor   *color);
void                  schemes_scheme_remove_color    (SchemesScheme  *self,
                                                      SchemesColor   *color);
gboolean              schemes_scheme_import_palette  (SchemesScheme  *self,
                                                      const char     *data,
                                                      gssize          len,
                                                      GError        **error);
SchemesStyle         *schemes_scheme_get_style       (SchemesScheme  *self,
                                                      const char     *name);
char                 *schemes_scheme_to_string       (SchemesScheme  *self);
GtkSourceStyleScheme *schemes_scheme_preview         (SchemesScheme  *self);
gboolean              schemes_scheme_is_pristine     (SchemesScheme  *self);
gboolean              schemes_scheme_load_from_file  (SchemesScheme  *self,
                                                      GFile          *file,
                                                      GError        **error);

G_END_DECLS

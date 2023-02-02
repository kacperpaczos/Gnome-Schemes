/* schemes-color-row.h
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

#include <adwaita.h>

#include "schemes-color.h"

G_BEGIN_DECLS

#define SCHEMES_TYPE_COLOR_ROW (schemes_color_row_get_type())

G_DECLARE_FINAL_TYPE (SchemesColorRow, schemes_color_row, SCHEMES, COLOR_ROW, AdwActionRow)

GtkWidget    *schemes_color_row_new       (SchemesColor    *color);
SchemesColor *schemes_color_row_get_color (SchemesColorRow *self);

G_END_DECLS

/* schemes-window.h
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

#pragma once

#include <adwaita.h>

#include "schemes-scheme.h"

G_BEGIN_DECLS

#define SCHEMES_TYPE_WINDOW (schemes_window_get_type())

G_DECLARE_FINAL_TYPE (SchemesWindow, schemes_window, SCHEMES, WINDOW, AdwApplicationWindow)

SchemesScheme *schemes_window_get_scheme (SchemesWindow *self);
void           schemes_window_set_scheme (SchemesWindow *self,
                                          SchemesScheme *scheme);

G_END_DECLS

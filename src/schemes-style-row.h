/* schemes-style-row.h
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

#include <adwaita.h>

#include "schemes-style.h"

G_BEGIN_DECLS

#define SCHEMES_TYPE_STYLE_ROW (schemes_style_row_get_type())

typedef enum
{
  SCHEMES_STYLE_OPTIONS_HAS_BACKGROUND        = 1 << 0,
  SCHEMES_STYLE_OPTIONS_HAS_FOREGROUND        = 1 << 1,
  SCHEMES_STYLE_OPTIONS_HAS_UNDERLINE         = 1 << 2,
  SCHEMES_STYLE_OPTIONS_HAS_UNDERLINE_COLOR   = 1 << 3,
  SCHEMES_STYLE_OPTIONS_HAS_BOLD              = 1 << 4,
  SCHEMES_STYLE_OPTIONS_HAS_SCALE             = 1 << 5,
  SCHEMES_STYLE_OPTIONS_HAS_ITALIC            = 1 << 6,
  SCHEMES_STYLE_OPTIONS_HAS_LINE_BACKGROUND   = 1 << 7,
  SCHEMES_STYLE_OPTIONS_HAS_STRIKETHROUGH     = 1 << 8,
  SCHEMES_STYLE_OPTIONS_HAS_WEIGHT            = 1 << 9,
  SCHEMES_STYLE_OPTIONS_HAS_ALL               = (SCHEMES_STYLE_OPTIONS_HAS_BACKGROUND |
                                                 SCHEMES_STYLE_OPTIONS_HAS_FOREGROUND |
                                                 SCHEMES_STYLE_OPTIONS_HAS_UNDERLINE |
                                                 SCHEMES_STYLE_OPTIONS_HAS_UNDERLINE_COLOR |
                                                 SCHEMES_STYLE_OPTIONS_HAS_BOLD |
                                                 SCHEMES_STYLE_OPTIONS_HAS_SCALE |
                                                 SCHEMES_STYLE_OPTIONS_HAS_ITALIC |
                                                 SCHEMES_STYLE_OPTIONS_HAS_LINE_BACKGROUND |
                                                 SCHEMES_STYLE_OPTIONS_HAS_STRIKETHROUGH |
                                                 SCHEMES_STYLE_OPTIONS_HAS_WEIGHT),
} SchemesStyleOptions;

G_DECLARE_FINAL_TYPE (SchemesStyleRow, schemes_style_row, SCHEMES, STYLE_ROW, AdwExpanderRow)

GtkWidget *schemes_style_row_new (const char          *title,
                                  const char          *subtitle,
                                  SchemesStyleOptions  flags,
                                  SchemesStyle        *style);

G_END_DECLS

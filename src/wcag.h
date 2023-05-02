/* wcag.h
 *
 * Copyright 2023 Kacper Paczos
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

#include <stdio.h>

G_BEGIN_DECLS

// Shift all characters one position to the left if the first character is '#'.
void removeHash(char hex_color);

// Calculates the contrast between two colors.
double calculate_wcag_contrast(const char *background_color, const char *foreground_color);

// Check if the contrast for normal text meets the requirements of WCAG 2.0 Level AA
bool isContrastValidWCAG2_0_AA(double contrast);

// Check if the contrast for large text meets the requirements of WCAG 2.0 Level AA
bool isLargeTextContrastValidWCAG2_0_AA(double contrast);

// Check if the contrast for normal text meets the requirements of WCAG 2.0 Level AAA
bool isContrastValidWCAG2_0_AAA(double contrast);

// Check if the contrast for large text meets the requirements of WCAG 2.0 Level AAA
bool isLargeTextContrastValidWCAG2_0_AAA(double contrast);

G_END_DECLS

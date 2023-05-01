/* wcag.c
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

#include "config.h"

#include "schemes-application.h"

#include <stdio.h>

double calculate_wcag_contrast(const char* color1, const char* color2) {
    // Convert colors from hex format to RGB values
    unsigned int r1, g1, b1;
    sscanf(color1, "%2x%2x%2x", &r1, &g1, &b1);

    unsigned int r2, g2, b2;
    sscanf(color2, "%2x%2x%2x", &r2, &g2, &b2);

    // Calculate color brightness in sRGB scale
    double luminance1 = (0.2126 * r1 + 0.7152 * g1 + 0.0722 * b1) / 255.0;
    double luminance2 = (0.2126 * r2 + 0.7152 * g2 + 0.0722 * b2) / 255.0;

    // Calculate color contrast
    double contrast = (luminance1 + 0.05) / (luminance2 + 0.05);

    // Return the calculated contrast
    return contrast;
}

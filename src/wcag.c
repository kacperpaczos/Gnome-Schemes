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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Shift all characters one position to the left if the first character is '#'.
void removeHash(char hex_color[])
{
    if (hex_color[0] == '#')
    {
        memmove(hex_color, hex_color + 1, strlen(hex_color));
    }
}

// Calculates the contrast between two colors.
double calculate_wcag_contrast(const char *background_color, const char *foreground_color)
{
    char background[strlen(background_color) + 1];
    char foreground[strlen(foreground_color) + 1];

    // strcpy kopiuje również znak '\0'
    strcpy(background, background_color);
    strcpy(foreground, foreground_color);

    removeHash(background);
    removeHash(foreground);

    // Convert colors from hex format to RGB values
    unsigned int r1, g1, b1;
    sscanf(background, "%2x%2x%2x", &r1, &g1, &b1);

    unsigned int r2, g2, b2;
    sscanf(foreground, "%2x%2x%2x", &r2, &g2, &b2);

    // Calculate color brightness in sRGB scale
    double luminance1 = (0.2126 * r1 + 0.7152 * g1 + 0.0722 * b1) / 255.0;
    double luminance2 = (0.2126 * r2 + 0.7152 * g2 + 0.0722 * b2) / 255.0;

    // Calculate color contrast
    double contrast = (luminance1 + 0.05) / (luminance2 + 0.05);

    // Return the calculated contrast
    return contrast;
}

/*
WCAG 2.0 level AA requires a contrast ratio of at least 4.5:1 for normal text and 3:1 for large text.
WCAG Level AAA requires a contrast ratio of at least 7:1 for normal text and 4.5:1 for large text.

Large text is defined as 14 point (typically 18.66px) and bold or larger, or 18 point (typically 24px) or larger.
https://webaim.org/resources/contrastchecker/
*/

// Check if the contrast for normal text meets the requirements of WCAG 2.0 Level AA
bool isContrastValidWCAG2_0_AA(double contrast)
{
    if (contrast >= 4.5)
    {
        return true; // Contrast is sufficient
    }
    else
    {
        return false; // Contrast does not meet the requirements
    }
}

// Check if the contrast for large text meets the requirements of WCAG 2.0 Level AA
bool isLargeTextContrastValidWCAG2_0_AA(double contrast)
{
    if (contrast >= 3.0)
    {
        return true; // Contrast is sufficient
    }
    else
    {
        return false; // Contrast does not meet the requirements
    }
}

// Check if the contrast for normal text meets the requirements of WCAG 2.0 Level AAA
bool isContrastValidWCAG2_0_AAA(double contrast)
{
    if (contrast >= 7.0)
    {
        return true; // Contrast is sufficient
    }
    else
    {
        return false; // Contrast does not meet the requirements
    }
}

// Check if the contrast for large text meets the requirements of WCAG 2.0 Level AAA
bool isLargeTextContrastValidWCAG2_0_AAA(double contrast)
{
    if (contrast >= 4.5)
    {
        return true; // Contrast is sufficient
    }
    else
    {
        return false; // Contrast does not meet the requirements
    }
}


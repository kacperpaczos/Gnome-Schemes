/* schemes-xml.h
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

#include <glib.h>

G_BEGIN_DECLS

static inline void
schemes_xml_writer_open_element (GString    *string,
                                 const char *element_name)
{
  g_string_append_printf (string, "<%s>", element_name);
}

static inline void
schemes_xml_writer_begin_open_element (GString    *string,
                                       const char *element_name)
{
  g_string_append_printf (string, "<%s", element_name);
}

static inline void
schemes_xml_writer_add_attribute (GString    *string,
                                  const char *attribute_name,
                                  const char *attribute_value)
{
  if (attribute_value != NULL)
    {
      g_autofree char *escaped = g_markup_escape_text (attribute_value, -1);

      g_string_append_c (string, ' ');
      g_string_append (string, attribute_name);
      g_string_append (string, "=\"");
      g_string_append (string, escaped);
      g_string_append_c (string, '"');
    }
}

static inline void
schemes_xml_writer_add_attributes (GString            *string,
                                   const char * const *attribute_names,
                                   const char * const *attribute_values)
{
  for (guint i = 0; attribute_names[i]; i++)
    schemes_xml_writer_add_attribute(string, attribute_names[i], attribute_values[i]);
}

static inline void
schemes_xml_writer_end_open_element (GString  *string,
                                     gboolean  has_children)
{
  if (has_children)
    g_string_append_c (string, '>');
  else
    g_string_append (string, "/>");
}

static inline void
schemes_xml_writer_close_element (GString    *string,
                                  const char *element_name)
{
  g_string_append_printf (string, "</%s>", element_name);
}

static inline void
schemes_xml_writer_add_attributes_va (GString    *string,
                                      const char *first_attribute,
                                      ...)
{
  va_list args;

  va_start (args, first_attribute);
  while (first_attribute)
    {
      const char *value = va_arg (args, const char *);
      if (value)
        schemes_xml_writer_add_attribute (string, first_attribute, value);
      first_attribute = va_arg (args, const char *);
    }
  va_end (args);
}

static inline void
schemes_xml_writer_write_escaped (GString    *string,
                                  const char *str)
{
  char *escaped;

  if (str == NULL || *str == 0)
    return;

  escaped = g_markup_escape_text (str, -1);
  g_string_append (string, escaped);
  g_free (escaped);
}

static inline void
schemes_xml_writer_write_gstring_escaped (GString *string,
                                          GString *to_write)
{
  if (to_write)
    schemes_xml_writer_write_escaped (string, to_write->str);
}

G_END_DECLS

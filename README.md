# Schemes

This application is meant to help people who need to edit GtkSourceView
style-schemes for an application or platform. Additionally, it can help
users modify existing schemes to their preference.

Style schemes created with this application can be used with GtkSourceView
5.3 and newer as it requires support for the `<metadata>` element.

However, if you remove the `<metadata>` element, it is likely to work with
older versions of GtkSourceView as well.

## Building

Use GNOME Builder to checkout the `Schemes` project and click Run.

Otherwise, you need a recent GNOME/libadwaita and chergert/libpanel.

## Installing

The Continuous Integration provides a `.flatpak` which can be installed
using `flatpak --user install --bundle ./path-to-schemes.flatpak`.

## Screenshots

![Editing a style scheme](https://gitlab.gnome.org/chergert/schemes/raw/5ed3a29ff738e0a7c30bce826aaf9b04f5680f40/data/screenshots/schemes-01.png)

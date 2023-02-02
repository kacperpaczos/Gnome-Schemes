#!/usr/bin/env lua

local lgi = require('lgi')
local Gtk = lgi.require('Gtk', '3.0')

local app = Gtk.Application {
  on_activate = function(app)
    local win = app.active_window
    if not win then
      win = Gtk.ApplicationWindow {
        default_width = 600,
        default_height = 300,
        application = app,
        Gtk.Label {
          label = 'Hello, World!',
          visible = true
        }
      }
      win:set_titlebar(Gtk.HeaderBar {
        title = 'Hello, World!',
        show_close_button = true,
        visible = true
      })
    end
    win:present()
  end
}

app:run()
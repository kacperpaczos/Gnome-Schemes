#!/usr/bin/env ruby

require 'gtk3'
require 'fileutils'

path = File.expand_path(File.dirname(__FILE__))

gresource_bin = "#{path}/hello.gresources"
gresource_xml = "#{path}/hello.gresources.xml"

system("glib-compile-resources",
       "--target", gresource_bin,
       "--sourcedir", File.dirname(gresource_xml),
       gresource_xml)

at_exit do
  FileUtils.rm_f([gresource_bin])
end

resource = Gio::Resource.load(gresource_bin)
Gio::Resources.register(resource)

class HelloWindowApp < Gtk::ApplicationWindow
  type_register

  class << self
    def init
      set_template(:resource => "/hello/hello-window.ui")
      bind_template_child("label")
    end
  end

  def initialize(app)
    super(:application => app)
  end
end

class HelloApp < Gtk::Application
  def initialize
    super("org.gtk.hello", :flags_none)

    signal_connect("activate") do |application|
      window = HelloWindowApp.new(application)
      window.present
    end
  end
end

app = HelloApp.new
app.run([$0]+ARGV)

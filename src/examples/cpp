#include "hello-window.hpp"


HelloWindow::HelloWindow()
  : Glib::ObjectBase("HelloWindow")
  , Gtk::Window()
  , headerbar(nullptr)
  , label(nullptr)
{
  builder = Gtk::Builder::create_from_resource("/hello/hello-window.ui");
  builder->get_widget("headerbar", headerbar);
  builder->get_widget("label", label);
  add(*label);
  label->show();
  set_titlebar(*headerbar);
  headerbar->show();
}

#!/usr/bin/env perl

# TODO: Use GtkApplication once it's usable.

use Glib;
use Gtk3 '-init';

my $win = Gtk3::Window->new('toplevel');
my $headerbar = Gtk3::HeaderBar->new();
my $label = Gtk3::Label->new();

$headerbar->set_show_close_button(true);
$headerbar->set_title('Hello, World!');

$label->set_label('Hello, World!');

$win->set_titlebar($headerbar);
$win->add($label);
$win->set_default_size(600, 300);
$win->show_all;
$win->signal_connect(delete_event => sub { Gtk3->main_quit; });
$win->present;

Gtk3::main;

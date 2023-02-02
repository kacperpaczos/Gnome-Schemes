package main

import (
	"github.com/gotk3/gotk3/glib"
	"github.com/gotk3/gotk3/gtk"
)

func main() {
	gtk.Init(nil)

	app, _ := gtk.ApplicationNew("com.example.Hello", glib.APPLICATION_FLAGS_NONE)

	win, _ := gtk.WindowNew(gtk.WINDOW_TOPLEVEL)
	win.SetDefaultSize(600, 300)

	hbBuilder, _ := gtk.BuilderNew()
	hbBuilder.AddFromFile("hello-window.ui")

	hbObject, _ := hbBuilder.GetObject("headerbar")
	headerBar := hbObject.(*gtk.HeaderBar)

	labelObject, _ := hbBuilder.GetObject("label")
	label := labelObject.(*gtk.Label)

	win.SetTitlebar(headerBar)
	win.Add(label)
	win.ShowAll()

	app.Connect("activate", func() { app.AddWindow(win) })
	win.Connect("destroy", func() { gtk.MainQuit() })

	gtk.Main()
}

using Gtk;
using System;

namespace Hello
{
	public class Window : Gtk.Window
	{
		public Window (string title) : base (title)
		{
		}

		static void Main (string[] args)
		{
			Application.Init ();

			var win = new Hello.Window ("Hello, World!");
			win.SetDefaultSize (600, 300);
			win.Present ();

			var label = new Gtk.Label ();
			label.Markup = "<span size='larger' weight='bold'>Hello, World!</span>";
			label.Show ();
			win.Add (label);

			win.DeleteEvent += (_, ev) => {
				Application.Quit ();
			};

			Application.Run ();
		}
	}
}

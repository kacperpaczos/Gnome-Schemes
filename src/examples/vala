using Gtk;

namespace Hello {
	[GtkTemplate (ui = "/hello/hello-window.ui")]
	public class Window : Gtk.Window {
		[GtkChild]
		Label label;
	
		[GtkChild]
		HeaderBar headerbar;
		
		public Window (Gtk.Application app) {
			Object(application: app);
		}
	}
}

int main (string[] args) {
	var app = new Gtk.Application ("com.example.hello", ApplicationFlags.FLAGS_NONE);
	app.activate.connect (() => {
		if (app.active_window == null) {
			new Hello.Window (app).show_all();
		}
		app.active_window.present ();
	});
	int ret = app.run (args);

	return ret;
}

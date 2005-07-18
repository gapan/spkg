#!/usr/bin/env python
from sys import argv, exc_info, path
path.append('../build/lib.linux-i686-2.4')
import pygtk
pygtk.require('2.0')
import gtk
from spkg import *

class gspkg_mainwin:

    def delete_event(self, widget, event, data=None):
        gtk.main_quit()
        return False

    def __init__(self):
        # Create a new window
        self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        self.window.set_title("GNOME SPKG GUI 0.0")

        self.window.set_size_request(640, 480)
        self.window.connect("delete_event", self.delete_event)

        self.liststore = gtk.ListStore(str, str)
        self.treeview = gtk.TreeView(self.liststore)
        self.col_1 = gtk.TreeViewColumn('Package Name')
        self.col_2 = gtk.TreeViewColumn('Description')
        self.treeview.append_column(self.col_1)
        self.treeview.append_column(self.col_2)
        self.cell_1 = gtk.CellRendererText()
        self.cell_2 = gtk.CellRendererText()
        self.cell_1.set_property('cell-background', 'yellow')
        self.col_1.pack_start(self.cell_1, False)
        self.col_2.pack_start(self.cell_2, True)
        self.col_1.set_attributes(self.cell_1, text=0)
        self.col_2.set_attributes(self.cell_2, text=1)
        self.treeview.set_search_column(0)
        self.col_1.set_sort_column_id(0)

        db_open()
        for p in db_get_packages():
                self.liststore.append([p.name, p.version])
        db_close()

        self.window.add(self.treeview)
        self.window.show_all()

def main():
    gtk.main()

if __name__ == "__main__":
    mw = gspkg_mainwin()
    main()

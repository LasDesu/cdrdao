/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998  Andreas Mueller <mueller@daneb.ping.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <fstream.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <strstream.h>

#include <glade/glade.h>

#include "xcdrdao.h"
#include "MDIWindow.h"

void
MDIWindow::nothing_cb()
{
  cout << "nothing here" << endl;
}

void
MDIWindow::install_menus_and_toolbar()
{

  using namespace Gnome::Menu_Helpers;
  
  // File->New menu
  //
  Gnome::UIInfoTree *newMenuTree = new Gnome::UIInfoTree();
  newMenuTree->push_item(Gnome::UIItem(N_("Audio CD"), N_("New Audio CD"),
  				GNOME_STOCK_MENU_NEW,
  				slot(this, &MDIWindow::nothing_cb)));
  newMenuTree->push_item(Gnome::UIItem(N_("Data CD"), N_("New Data CD"),
  				GNOME_STOCK_MENU_NEW,
  				slot(this, &MDIWindow::nothing_cb)));
  newMenuTree->push_item(Gnome::UIItem(N_("Mixed CD"), N_("New Mixed CD"),
  				GNOME_STOCK_MENU_NEW,
  				slot(this, &MDIWindow::nothing_cb)));

  StockMenuItems::NewSubtree *newMenu = new 
  				StockMenuItems::NewSubtree(*newMenuTree);
//  Gnome::UIItem *newMenu = new Gnome::UISubtree(N_("New"),
//  				*newMenuTree, GNOME_STOCK_MENU_NEW);

  // File menu
  //
  Gnome::UIInfoTree *fileMenuTree = new Gnome::UIInfoTree();
  
/*  fileMenuTree->push_item(StockMenuItems::New(N_("New"),
  				("Create a new compilation"),
				slot(this, &MDIWindow::nothing_cb)));
*/
  fileMenuTree->push_item(*newMenu);

  fileMenuTree->push_item(StockMenuItems::Open
  				(slot(this, &MDIWindow::nothing_cb)));

  fileMenuTree->push_item(StockMenuItems::Save
  				(slot(this, &MDIWindow::nothing_cb)));

  fileMenuTree->push_item(StockMenuItems::SaveAs
  				(slot(this, &MDIWindow::nothing_cb)));

  fileMenuTree->push_item(Gnome::UISeparator());

  fileMenuTree->push_item(StockMenuItems::PrintSetup
  				(slot(this, &MDIWindow::nothing_cb)));

  fileMenuTree->push_item(Gnome::UIItem(N_("Print Cover..."), N_("Print Cover"),
  				GNOME_STOCK_MENU_PRINT,
  				slot(this, &MDIWindow::nothing_cb)));

  fileMenuTree->push_item(Gnome::UISeparator());

//This Close refers to close the current (selected) child
  fileMenuTree->push_item(StockMenuItems::Close
  				(slot(this, &MDIWindow::nothing_cb)));

  fileMenuTree->push_item(StockMenuItems::Exit
  				(slot(this, &MDIWindow::app_close)));

  StockMenus::File *fileMenu = new StockMenus::File(*fileMenuTree);

// The Edit Menu should be a per child menu, so every child knows how
// to do the copy, cut and paste operations. And also more operations,
// like in the Audio editing (Insert file, Insert Silence, ...)
// It is here for fast copy and paste ;)
  // Edit menu
  //
  Gnome::UIInfoTree *editMenuTree = new Gnome::UIInfoTree();
  StockMenus::Edit *editMenu = new StockMenus::Edit(*editMenuTree);

  // Actions menu
  //
  Gnome::UIInfoTree *actionsMenuTree = new Gnome::UIInfoTree();
  actionsMenuTree->push_item(Gnome::UIItem(N_("Duplicate CD"), N_(""),
  				GNOME_STOCK_MENU_BLANK,
  				slot(this, &MDIWindow::nothing_cb)));
  actionsMenuTree->push_item(Gnome::UIItem(N_("Record"), N_(""),
  				GNOME_STOCK_MENU_BLANK,
  				slot(this, &MDIWindow::nothing_cb)));
  actionsMenuTree->push_item(Gnome::UIItem(N_("Fixate CD"), N_(""),
  				GNOME_STOCK_MENU_BLANK,
  				slot(this, &MDIWindow::nothing_cb)));
  actionsMenuTree->push_item(Gnome::UIItem(N_("Blank CD-RW"), N_(""),
  				GNOME_STOCK_MENU_BLANK,
  				slot(this, &MDIWindow::nothing_cb)));
  actionsMenuTree->push_item(Gnome::UIItem(N_("Get Info"), N_(""),
  				GNOME_STOCK_MENU_BLANK,
  				slot(this, &MDIWindow::nothing_cb)));

  Gnome::UIItem *actionsMenu = new Gnome::UISubtree(N_("_Actions"),
  				*actionsMenuTree);

  // Settings menu
  //
  Gnome::UIInfoTree *settingsMenuTree = new Gnome::UIInfoTree();
  settingsMenuTree->push_item(Gnome::UIItem(N_("Configure Devices..."), N_(""),
  				GNOME_STOCK_MENU_PROP,
  				slot(this, &MDIWindow::nothing_cb)));
  settingsMenuTree->push_item(StockMenuItems::Preferences
  				(slot(this, &MDIWindow::nothing_cb)));

  StockMenus::Settings *settingsMenu = new StockMenus::Settings(*settingsMenuTree);


  // Help menu
  //
  Gnome::UIInfoTree *helpMenuTree = new Gnome::UIInfoTree();

  helpMenuTree->push_item(Gnome::UIHelp("Quick Start"));

  helpMenuTree->push_item(StockMenuItems::About
  				(slot(this, &MDIWindow::nothing_cb)));

  StockMenus::Help *helpMenu = new StockMenus::Help(*helpMenuTree);

  Gnome::UIInfoTree *menus = new Gnome::UIInfoTree();

  menus->push_item(*fileMenu);
  menus->push_item(*editMenu);
  menus->push_item(*actionsMenu);
  menus->push_item(*settingsMenu);
  menus->push_item(*helpMenu);

//FIXME: MDI STUFF  set_menubar_template(*menus);
  create_menus(*menus);
  
  // Toolbar
  //
  Gnome::UIInfoTree *toolbarTree = new Gnome::UIInfoTree();

  toolbarTree->push_item(Gnome::UIItem(N_("New"),
                                       N_("Create a new compilation"),
                                       GNOME_STOCK_PIXMAP_NEW,
                                       slot(this, &MDIWindow::nothing_cb)));

  toolbarTree->push_item(Gnome::UISeparator());

  toolbarTree->push_item(Gnome::UIItem(N_("Quit"),
                                       N_("Quit application"),
                                       GNOME_STOCK_PIXMAP_QUIT,
                                       slot(this, &MDIWindow::app_close)));

//FIXME: MDI STUFF  set_toolbar_template(*toolbarTree);
  create_toolbar(*toolbarTree);

//  install_menu_hints(*menus);
}

MDIWindow::MDIWindow()
//FIXME: MDI STUFF  : Gnome::MDI("StillNoName", "StillNoTitle")
  : Gnome::App("StillNoName", "StillNoTitle")
{
//  set_policy(false, true, false);
//  set_default_size(600, 400);
//  set_wmclass("StillNoClass", "StillNoClass");
  
  install_menus_and_toolbar();

//   delete_event.connect(slot(this, &MDIWindow::delete_event_cb));

}

void MDIWindow::app_close()
{
/*  if (tocEdit_->tocDirty()) {
    Ask2Box msg(this, "Quit", 0, 2, "Current work not saved.", "",
		"Really Quit?", NULL);
    if (msg.run() != 1)
      return;
  }
*/

//  destroy();
  
//  hide();
//  MDIWindow::remove_all(0);
//  Gnome::Main::quit();
  Gtk::Main::quit();

}

gint 
MDIWindow::delete_event_impl(GdkEventAny* e)
{
  app_close();

  /* Prevent the window's destruction, since we destroyed it 
   * ourselves with app_close()
   */
  return true;
}

/*
void MDIWindow::update(unsigned long level)
{
  if (level & (UPD_TOC_DIRTY | UPD_TOC_DATA)) {
    string s(tocEdit_->filename());

    if (tocEdit_->tocDirty())
      s += "(*)";
    
    set_title(s);

    cursorPos_->set_text("");
  }

  if (level & UPD_TRACK_MARK_SEL) {
    int trackNr, indexNr;

    if (tocEdit_->trackSelection(&trackNr) && 
	tocEdit_->indexSelection(&indexNr)) {
      sampleDisplay_->setSelectedTrackMarker(trackNr, indexNr);
    }
    else {
      sampleDisplay_->setSelectedTrackMarker(0, 0);
    }
  }

  if (level & UPD_SAMPLES) {
    sampleDisplay_->updateToc();
  }
  else if (level & (UPD_TRACK_DATA | UPD_TRACK_MARK_SEL)) {
    sampleDisplay_->updateTrackMarks();
  }

  if (level & UPD_SAMPLE_MARKER) {
    unsigned long marker;

    if (tocEdit_->sampleMarker(&marker)) {
      markerPos_->set_text(string(sample2string(marker)));
      sampleDisplay_->setMarker(marker);
    }
    else {
      markerPos_->set_text(string(""));
      sampleDisplay_->clearMarker();
    }
  }

  if (level & UPD_SAMPLE_SEL) {
    unsigned long start, end;

    if (tocEdit_->sampleSelection(&start, &end)) {
      selectionStartPos_->set_text(string(sample2string(start)));
      selectionEndPos_->set_text(string(sample2string(end)));
      sampleDisplay_->setRegion(start, end);
    }
    else {
      selectionStartPos_->set_text(string(""));
      selectionEndPos_->set_text(string(""));
      sampleDisplay_->setRegion(1, 0);
    }
  }
}
*/

void
nada_cb(GtkWidget *widget, gpointer data)
{
  g_print("%s", "asdfsfdasadf");
  cout << "nothing here" << endl;
}

/*
GtkWidget *
example_creator(GnomeMDIChild *child, gpointer data)
{
  GladeXML *xml;
  GtkWidget *new_view;       
//  GtkWidget *new_view = gtk_vbox_new(TRUE, TRUE);

  xml = glade_xml_new ("./glade/record.glade", "hbox1");
  new_view = glade_xml_get_widget (xml, "hbox1");
  glade_xml_signal_autoconnect(xml);

Gtk::Widget *view2 = Gtk::wrap(new_view);

        return new_view;
}


void
MDIWindow::example_child()
{
Gnome::MDIGenericChild *example = new Gnome::MDIGenericChild("example");
example->set_view_creator(example_creator, NULL);

MDIWindow::add_child(*example);
MDIWindow::add_view(*example);
}
*/
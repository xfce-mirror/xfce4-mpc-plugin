/* xfce4-mpc-plugin.c
 * 
 * Copyright (c) 2006 Landry Breuil (landry at fr.homeunix.org / gaston at gcu.info)
 * This code is licenced under a BSD-style licence. 
 * (OpenBSD variant modeled after the ISC licence)
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
 
#include <stdio.h>
#include <string.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#define DEFAULT_MPD_HOST "localhost"
#define DEFAULT_MPD_PORT 6600
#define DIALOG_ENTRY_WIDTH 15

#include "xfce4-mpc-plugin.h"

#ifndef HAVE_LIBMPD
MpdObj* mpd_new(char* host, int port, char* pass)
{
}
void mpd_free(MpdObj* mo){}
void mpd_connect(MpdObj* mo){}
int mpd_status_get_volume(MpdObj* mo){}
void mpd_status_set_volume(MpdObj* mo, int newvol){}
int mpd_status_update(MpdObj* mo){}
int mpd_player_get_state(MpdObj* mo){}
int mpd_player_prev(MpdObj* mo){}
int mpd_player_next(MpdObj* mo){}
int mpd_player_stop(MpdObj* mo){}
int mpd_player_play(MpdObj* mo){}
mpd_Song* mpd_playlist_get_current_song(MpdObj* mo){}
int mpd_check_error(MpdObj* mo){}
void mpd_set_hostname(MpdObj* mo, char* host){}
void mpd_set_password(MpdObj* mo, char* pass){}
void mpd_send_password(MpdObj* mo){}
void mpd_set_port(MpdObj* mo,int port){}
#endif

static void
mpc_free (XfcePanelPlugin * plugin, t_mpc * mpc)
{
#if DEBUG
   puts("mpc_free()");
#endif
   g_object_unref (mpc->tips);
   mpd_free(mpc->mo);
   g_free (mpc);
}

static void 
mpc_set_orientation (XfcePanelPlugin * plugin, GtkOrientation orientation, t_mpc * mpc)
{
#if DEBUG
   puts("mpc_set_orientation()");
#endif
   xfce_hvbox_set_orientation(mpc->box,orientation);
}

static gboolean mpc_set_size (XfcePanelPlugin * plugin, int size, t_mpc * mpc)
{
#if DEBUG
   printf("mpc_set_size(),size=%d\n",size);
#endif
   
   if (size > 26)
   {
      gtk_container_set_border_width (GTK_CONTAINER (mpc->frame), 2);
      if (xfce_panel_plugin_get_orientation (plugin) == GTK_ORIENTATION_HORIZONTAL)
	 gtk_widget_set_size_request (GTK_WIDGET (mpc->frame), -1, size-2 );
      else
         gtk_widget_set_size_request (GTK_WIDGET (mpc->frame), size-4 , -1);
   }
   else
      gtk_container_set_border_width (GTK_CONTAINER (mpc->frame), 0);
   return TRUE;
}

static void mpc_read_config (XfcePanelPlugin * plugin, t_mpc * mpc)
{
   char *file;
   XfceRc *rc;

#if DEBUG
   puts("mpc_read_config()");
#endif

   if (!(file = xfce_panel_plugin_lookup_rc_file (plugin)))
      return;

   rc = xfce_rc_simple_open (file, TRUE);
   g_free (file);

   if (!rc)
      return;
    
   xfce_rc_set_group (rc, "Settings");

   mpc->mpd_host = (gchar*) xfce_rc_read_entry (rc, "mpd_host",  DEFAULT_MPD_HOST);
   mpc->mpd_port = xfce_rc_read_int_entry (rc, "mpd_port", DEFAULT_MPD_PORT);
   mpc->mpd_password = (gchar*) xfce_rc_read_entry (rc, "mpd_password", NULL);
   mpc->show_frame = xfce_rc_read_bool_entry (rc, "show_frame", TRUE);
/* mpc->stay_connected = xfce_rc_read_bool_entry (rc, "stay_connected", TRUE); */
   
   xfce_rc_close (rc);
}

	
static void mpc_write_config (XfcePanelPlugin * plugin, t_mpc * mpc)
{
   XfceRc *rc;
   char *file;

#if DEBUG
   puts("mpc_write_config()");
#endif

   if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
      return;

   rc = xfce_rc_simple_open (file, FALSE);
   g_free (file);

   if (!rc)
      return;

   /* if necessary, get rid of old settings */
   if (xfce_rc_has_group (rc, "Settings"))
      xfce_rc_delete_group (rc, "Settings", TRUE);
   
   xfce_rc_set_group (rc, "Settings");

   xfce_rc_write_entry (rc, "mpd_host", mpc->mpd_host);
   xfce_rc_write_int_entry (rc, "mpd_port", mpc->mpd_port);
   if (mpc->mpd_password) 
      xfce_rc_write_entry (rc, "mpd_password", mpc->mpd_password);
   xfce_rc_write_bool_entry (rc, "show_frame", mpc->show_frame);
/* xfce_rc_write_bool_entry (rc, "stay_connected", mpc->stay_connected); */

   xfce_rc_close (rc);
}

static void
mpc_dialog_apply_options (t_mpc_dialog *dialog)
{
#if DEBUG
   puts("mpc_dialog_apply_options()");
#endif
   t_mpc *mpc = dialog->mpc;
   const gchar * temp = gtk_entry_get_text(GTK_ENTRY(dialog->textbox_password));
   mpc->mpd_host = g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog->textbox_host)));
   mpc->mpd_port = atoi(gtk_entry_get_text(GTK_ENTRY(dialog->textbox_port)));
   mpc->mpd_password = (strlen(temp)) ? g_strdup(temp) : NULL;
#if DEBUG
   printf("values: host=%s, port=%d, passwd=%s\n", mpc->mpd_host, mpc->mpd_port, mpc->mpd_password);
#endif
   mpd_set_hostname(mpc->mo,mpc->mpd_host);
   mpd_set_port(mpc->mo,mpc->mpd_port);
   mpd_connect(mpc->mo);
   if (mpc->mpd_password)
   {
      mpd_set_password(mpc->mo,mpc->mpd_password);
      mpd_send_password(mpc->mo);
   }
}

static void
mpc_dialog_response (GtkWidget * dlg, int response, t_mpc_dialog * dialog)
{
   t_mpc *mpc = dialog->mpc;
#if DEBUG
   puts("mpc_dialog_response()");
#endif

   mpc_dialog_apply_options (dialog);
   g_free (dialog);
    
   gtk_widget_destroy (dlg);
   xfce_panel_plugin_unblock_menu (mpc->plugin);
   mpc_write_config (mpc->plugin, mpc);
}

static void
mpc_dialog_show_frame_toggled (GtkWidget *w, t_mpc_dialog *dialog)
{

   t_mpc* mpc = dialog->mpc; 
#if DEBUG
   puts("mpc_dialog_show_frame_toggled()");
#endif

   mpc->show_frame = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->checkbox_frame));
   gtk_frame_set_shadow_type (GTK_FRAME (mpc->frame), (mpc->show_frame) ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
}
/*static void
mpc_dialog_stay_connected_toggled (GtkWidget *w, t_mpc_dialog *dialog)
{
   t_mpc* mpc = dialog->mpc; 
   mpc->stay_connected = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->checkbox_connected));
}*/

static void
mpc_create_options (XfcePanelPlugin * plugin, t_mpc * mpc)
{
   GtkWidget *dlg, *header, *vbox, *table;
   GdkPixbuf *pb;
   gchar str_port[20];
   t_mpc_dialog *dialog;

   dialog = g_new0 (t_mpc_dialog, 1);

   dialog->mpc = mpc;

   xfce_panel_plugin_block_menu (plugin);

   dlg = xfce_titled_dialog_new_with_buttons (_("Mpd Client Plugin"),
                                              NULL,
                                              GTK_DIALOG_NO_SEPARATOR,
                                              GTK_STOCK_CLOSE,
                                              GTK_RESPONSE_OK,
                                              NULL);
   xfce_titled_dialog_set_subtitle (XFCE_TITLED_DIALOG (dlg), _("Properties"));

   gtk_window_set_position   (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
   gtk_window_set_icon_name  (GTK_WINDOW (dlg), "xfce-multimedia");

   g_signal_connect (dlg, "response", G_CALLBACK (mpc_dialog_response), dialog);

   vbox = gtk_vbox_new (FALSE, 8);
   gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
   gtk_widget_show (vbox);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox, TRUE, TRUE, 0);
    
   table = gtk_table_new(3,2,FALSE);
   gtk_table_attach_defaults(GTK_TABLE(table),gtk_label_new(_("Host : ")),0,1,0,1);
   gtk_table_attach_defaults(GTK_TABLE(table),gtk_label_new(_("Port : ")),0,1,1,2);
   gtk_table_attach_defaults(GTK_TABLE(table),gtk_label_new(_("Password : ")),0,1,2,3);

   dialog->textbox_host = gtk_entry_new();
   gtk_entry_set_width_chars(GTK_ENTRY(dialog->textbox_host),DIALOG_ENTRY_WIDTH);
   gtk_entry_set_text(GTK_ENTRY(dialog->textbox_host),mpc->mpd_host);
   gtk_table_attach_defaults(GTK_TABLE(table),dialog->textbox_host,1,2,0,1);
    
   dialog->textbox_port = gtk_entry_new();
   gtk_entry_set_width_chars(GTK_ENTRY(dialog->textbox_port),DIALOG_ENTRY_WIDTH);
   g_snprintf(str_port,sizeof(str_port),"%d",mpc->mpd_port);
   gtk_entry_set_text(GTK_ENTRY(dialog->textbox_port),str_port);
   gtk_table_attach_defaults(GTK_TABLE(table),dialog->textbox_port,1,2,1,2);
   
   dialog->textbox_password = gtk_entry_new();
   gtk_entry_set_visibility(GTK_ENTRY(dialog->textbox_password),FALSE);
   gtk_entry_set_width_chars(GTK_ENTRY(dialog->textbox_password),DIALOG_ENTRY_WIDTH);
   if (mpc->mpd_password) gtk_entry_set_text(GTK_ENTRY(dialog->textbox_password),mpc->mpd_password);
   gtk_table_attach_defaults(GTK_TABLE(table),dialog->textbox_password,1,2,2,3);
    
   gtk_widget_show_all (table);
   gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

   dialog->checkbox_frame = gtk_check_button_new_with_mnemonic (_("Show _frame"));
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->checkbox_frame),mpc->show_frame);
   gtk_widget_show (dialog->checkbox_frame);
   gtk_box_pack_start (GTK_BOX (vbox), dialog->checkbox_frame, FALSE, FALSE, 0);

   g_signal_connect (dialog->checkbox_frame, "toggled", G_CALLBACK (mpc_dialog_show_frame_toggled), dialog);
    
/*   dialog->checkbox_connected = gtk_check_button_new_with_mnemonic (_("Stay _connected"));
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->checkbox_connected),mpc->stay_connected);
   gtk_widget_show (dialog->checkbox_connected);
   gtk_box_pack_start (GTK_BOX (vbox), dialog->checkbox_connected, FALSE, FALSE, 0);

   g_signal_connect (dialog->checkbox_connected, "toggled", G_CALLBACK (mpc_dialog_stay_connected_toggled), dialog);*/

    /* show the dialog */
   gtk_widget_show (dlg);
}

static void 
enter_cb(GtkWidget *widget, GdkEventCrossing* event, gpointer data) 
{
   mpd_Song *song;
   char str[512];
   MpdObj *mo = ((t_mpc*) data)->mo;
#if DEBUG
   puts("enter_cb()");
#endif

   if (mpd_status_update(mo) != MPD_OK)
   {
#if DEBUG
      puts("reconnecting..");
#endif
      mpd_connect(mo);
      if (((t_mpc*) data)->mpd_password)
	 mpd_send_password(mo);
      if (mpd_check_error(mo) || mpd_status_update(mo) != MPD_OK)
      {
	 gtk_tooltips_set_tip (((t_mpc*)data)->tips, widget, _(".... not connected ?"), NULL);
	 return;
      }
   }
   switch (mpd_player_get_state(mo))
   {
      case MPD_PLAYER_PLAY:  
	 sprintf(str, "Mpd Playing");
	 break;
      case MPD_PLAYER_PAUSE:   
	 sprintf(str, "Mpd Paused");
	 break;
      case MPD_PLAYER_STOP:
	 sprintf(str, "Mpd Stopped");
	 break;
      default:
	 sprintf(str, "Mpd state ?");
	 break;
   }
   song = mpd_playlist_get_current_song(mo);
   if (song) 
      sprintf(str,"%s - [%s - %s] -/- (#%s) %s", str, song->artist, song->album, song->track, song->title);
   gtk_tooltips_set_tip (((t_mpc*)data)->tips, widget, str, NULL);
}

static void 
prev(GtkWidget *widget, GdkEventButton* event, gpointer data) 
{
   MpdObj *mo = ((t_mpc*) data)->mo;
   if (event->button != 1) return;
   if (mpd_player_prev(mo) != MPD_OK)
   {
#if DEBUG
      puts("reconnecting..");
#endif
      mpd_connect(mo);
      if (((t_mpc*) data)->mpd_password)
	 mpd_send_password(mo);
      if (!mpd_check_error(mo))
      {
#if DEBUG
	 puts("calling mpd_player_prev() after reconnect");
#endif
	 mpd_player_prev(mo);
      }
   }  
#if DEBUG
   else puts("mpd_player_prev() ok");
#endif
}

static void 
play(GtkWidget *widget, GdkEventButton* event, gpointer data)
{
   MpdObj *mo = ((t_mpc*) data)->mo;
   if (event->button != 1) return;
   if (mpd_player_play(mo) != MPD_OK)
   {
#if DEBUG
      puts("reconnecting..");
#endif
      mpd_connect(mo);
      if (((t_mpc*) data)->mpd_password)
	 mpd_send_password(mo);
      if (!mpd_check_error(mo))
      {
#if DEBUG
	 puts("calling mpd_player_play() after reconnect");
#endif
	 mpd_player_play(mo);
      }
   }  
#if DEBUG
   else puts("mpd_player_play() ok");
#endif
}

static void 
stop(GtkWidget *widget, GdkEventButton* event, gpointer data) 
{
   MpdObj *mo = ((t_mpc*) data)->mo;
   if (event->button != 1) return;
   if (mpd_player_stop(mo) != MPD_OK)
   {
#if DEBUG
      puts("reconnecting..");
#endif
      mpd_connect(mo);
      if (((t_mpc*) data)->mpd_password)
	 mpd_send_password(mo);
      if (!mpd_check_error(mo))
      {
#if DEBUG
	 puts("calling mpd_player_stop() after reconnect");
#endif
	 mpd_player_stop(mo);
      }
   }  
#if DEBUG
   else puts("mpd_player_stop() ok");
#endif
}

static void 
next(GtkWidget *widget, GdkEventButton* event, gpointer data) 
{
   MpdObj *mo = ((t_mpc*) data)->mo;
   if (event->button != 1) return;
   if (mpd_player_next(mo) != MPD_OK)
   {
#if DEBUG
      puts("reconnecting..");
#endif
      mpd_connect(mo);
      if (((t_mpc*) data)->mpd_password)
	 mpd_send_password(mo);
      if (!mpd_check_error(mo))
      {
#if DEBUG
	 puts("calling mpd_player_next() after reconnect");
#endif
	 mpd_player_next(mo);
      }
   }  
#if DEBUG
   else puts("mpd_player_next() ok");
#endif
}

static void 
scroll_cb(GtkWidget *widget, GdkEventScroll* event, gpointer data)
{
   MpdObj *mo = ((t_mpc*) data)->mo;
   int curvol=-1;
#if DEBUG
   puts("scroll_cb()");
#endif

   if (mpd_status_update(mo) != MPD_OK)
   {
#if DEBUG
      puts("reconnecting..");
#endif
      mpd_connect(mo);
      if (((t_mpc*) data)->mpd_password)
	 mpd_send_password(mo);
      if (mpd_check_error(mo) || mpd_status_update(mo) != MPD_OK)
      {
	 gtk_tooltips_set_tip (((t_mpc*)data)->tips, widget, _(".... not connected ?"), NULL);
	 return;
      }
   }

   curvol = mpd_status_get_volume(mo);
#if DEBUG
   printf("current volume=%d\n",curvol);
#endif
   
   if (event->type != GDK_SCROLL)
      return;
   else if (event->direction == GDK_SCROLL_DOWN) 
      mpd_status_set_volume(mo,curvol-5);
   else if (event->direction == GDK_SCROLL_UP) 
      mpd_status_set_volume(mo,curvol+5);
}

static void 
new_button_with_img(XfcePanelPlugin * plugin, GtkWidget *parent, GtkWidget *button, gchar *icon, gpointer cb, gpointer data) 
{
   GtkWidget *image;

   button = (GtkWidget*) xfce_create_panel_button();
   image = gtk_image_new_from_stock(icon,GTK_ICON_SIZE_BUTTON);
   gtk_button_set_image(GTK_BUTTON(button),image);
   xfce_panel_plugin_add_action_widget (plugin, button);
   gtk_widget_show (GTK_WIDGET(button));
   g_signal_connect (G_OBJECT(button), "button_press_event", G_CALLBACK(cb), data);
   gtk_box_pack_start (GTK_BOX(parent), button, TRUE,TRUE,0);
}

static t_mpc* 
mpc_create (XfcePanelPlugin * plugin)
{
   t_mpc *mpc;
#if DEBUG
   puts("mpc_create()");
#endif

   mpc = g_new0 (t_mpc, 1);
   
   mpc->plugin = plugin;

   mpc->tips = gtk_tooltips_new ();
   g_object_ref (mpc->tips);
   gtk_object_sink (GTK_OBJECT (mpc->tips));
    
   mpc->frame = gtk_frame_new (NULL);
   gtk_frame_set_shadow_type (GTK_FRAME (mpc->frame), GTK_SHADOW_IN);
   gtk_widget_show (mpc->frame);

   mpc->ebox = gtk_event_box_new();
   g_signal_connect (G_OBJECT(mpc->ebox), "enter_notify_event", G_CALLBACK(enter_cb), mpc);
   g_signal_connect (G_OBJECT(mpc->ebox), "scroll_event", G_CALLBACK(scroll_cb), mpc);
   gtk_widget_show (mpc->ebox);
   
   mpc->box = (GtkWidget*) xfce_hvbox_new(xfce_panel_plugin_get_orientation(plugin), FALSE, 0);
   
   gtk_container_add (GTK_CONTAINER(mpc->ebox), mpc->box);
   gtk_container_add (GTK_CONTAINER(mpc->frame), mpc->ebox);
   
   new_button_with_img(plugin, mpc->box, mpc->prev, GTK_STOCK_MEDIA_PREVIOUS, G_CALLBACK(prev), mpc);
   new_button_with_img(plugin, mpc->box, mpc->stop, GTK_STOCK_MEDIA_STOP, G_CALLBACK(stop), mpc);
   new_button_with_img(plugin, mpc->box, mpc->play, GTK_STOCK_MEDIA_PLAY, G_CALLBACK(play), mpc);
   new_button_with_img(plugin, mpc->box, mpc->next, GTK_STOCK_MEDIA_NEXT, G_CALLBACK(next), mpc);

   gtk_widget_show_all (mpc->box);
    
   return mpc;
}

static void
mpc_construct (XfcePanelPlugin * plugin)
{
   t_mpc *mpc;
#if DEBUG
   puts("mpc_construct()");
#endif

   xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    /* create widgets */
   mpc = mpc_create (plugin);
   mpc->mpd_host = DEFAULT_MPD_HOST;
   mpc->mpd_port = DEFAULT_MPD_PORT;
   mpc->mpd_password = NULL;
   mpc->show_frame = TRUE;
   /* mpc->stay_connected = TRUE; */

   mpc_read_config (plugin, mpc);
   
   /* create a connection*/
#if DEBUG
   debug_set_level(10);
#endif
   mpc->mo = mpd_new(mpc->mpd_host,mpc->mpd_port,mpc->mpd_password);
   mpd_connect(mpc->mo);
   if (mpc->mpd_password)
      mpd_send_password(mpc->mo);
   
   gtk_container_add (GTK_CONTAINER (plugin), mpc->frame);

    /* connect signal handlers */
   g_signal_connect (plugin, "free-data", G_CALLBACK (mpc_free), mpc);
   g_signal_connect (plugin, "save", G_CALLBACK (mpc_write_config), mpc);
   g_signal_connect (plugin, "size-changed", G_CALLBACK (mpc_set_size), mpc);
   g_signal_connect (plugin, "orientation-changed", G_CALLBACK (mpc_set_orientation), mpc);
   /* the configure and about menu items are hidden by default */
   xfce_panel_plugin_menu_show_configure (plugin);
   g_signal_connect (plugin, "configure-plugin", G_CALLBACK (mpc_create_options), mpc);
}


/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (mpc_construct);


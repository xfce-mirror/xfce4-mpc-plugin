/* xfce4-mpc-plugin.c
 *
 * Copyright (c) 2006-2007 Landry Breuil (landry at fr.homeunix.org / gaston at gcu.info)
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

/* double inclusion ?*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfcegui4/libxfcegui4.h>
#include <string.h>
#include <glib/gprintf.h>

#define DEFAULT_MPD_HOST "localhost"
#define DEFAULT_MPD_PORT 6600
#define DIALOG_ENTRY_WIDTH 15
#define STRLENGTH 32

#include "xfce4-mpc-plugin.h"

static void
mpc_free (XfcePanelPlugin * plugin, t_mpc * mpc)
{
   DBG ("!");

   g_object_unref (mpc->tips);
   mpd_free(mpc->mo);
   g_free (mpc);
}

static void
mpc_set_orientation (XfcePanelPlugin * plugin, GtkOrientation orientation, t_mpc * mpc)
{
   DBG ("!");

   xfce_hvbox_set_orientation(XFCE_HVBOX(mpc->box), orientation);
}

static gboolean
mpc_set_size (XfcePanelPlugin * plugin, int size, t_mpc * mpc)
{
   DBG ("size=%d",size);

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

static void
mpc_read_config (XfcePanelPlugin * plugin, t_mpc * mpc)
{
   char *file;
   XfceRc *rc;
   GtkWidget *label;
   char str[30];

   DBG ("!");

   if (!(file = xfce_panel_plugin_lookup_rc_file (plugin)))
      return;

   rc = xfce_rc_simple_open (file, TRUE);
   g_free (file);

   if (!rc)
      return;

   xfce_rc_set_group (rc, "Settings");

   if (mpc->mpd_host != NULL)
      g_free (mpc->mpd_host);
   if (mpc->mpd_password != NULL)
      g_free (mpc->mpd_password);

   mpc->mpd_host = g_strdup(xfce_rc_read_entry (rc, "mpd_host",  DEFAULT_MPD_HOST));
   mpc->mpd_port = xfce_rc_read_int_entry (rc, "mpd_port", DEFAULT_MPD_PORT);
   mpc->mpd_password = g_strdup(xfce_rc_read_entry (rc, "mpd_password", ""));
   mpc->show_frame = xfce_rc_read_bool_entry (rc, "show_frame", TRUE);
   mpc->client_appl = g_strdup(xfce_rc_read_entry (rc, "client_appl",  ""));
   label = gtk_bin_get_child(GTK_BIN(mpc->appl));
   g_sprintf(str, "%s %s", _("Launch"), mpc->client_appl);
   gtk_label_set_text(GTK_LABEL(label),str);
   DBG ("Settings : %s@%s:%d\nframe:%d\nappl:%s", mpc->mpd_password, mpc->mpd_host, mpc->mpd_port, mpc->show_frame, mpc->client_appl);
   xfce_rc_close (rc);
}


static void mpc_write_config (XfcePanelPlugin * plugin, t_mpc * mpc)
{
   XfceRc *rc;
   char *file;

   DBG ("!");

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
   xfce_rc_write_entry (rc, "mpd_password", mpc->mpd_password);
   xfce_rc_write_bool_entry (rc, "show_frame", mpc->show_frame);
   xfce_rc_write_entry (rc, "client_appl", mpc->client_appl);

   xfce_rc_close (rc);
}

static gboolean
mpc_plugin_reconnect (t_mpc *mpc)
{
   DBG ("!");

   mpd_connect (mpc->mo);
   if (strlen(mpc->mpd_password))
      mpd_send_password (mpc->mo);

   return !mpd_check_error (mpc->mo);
}

static void
mpc_dialog_apply_options (t_mpc_dialog *dialog)
{
   DBG ("!");
   GtkWidget *label;
   char str[30];

   t_mpc *mpc = dialog->mpc;
   mpc->mpd_host = g_strndup(gtk_entry_get_text(GTK_ENTRY(dialog->textbox_host)),STRLENGTH);
   mpc->mpd_port = atoi(gtk_entry_get_text(GTK_ENTRY(dialog->textbox_port)));
   mpc->mpd_password = g_strndup(gtk_entry_get_text(GTK_ENTRY(dialog->textbox_password)),STRLENGTH);
   mpc->client_appl = g_strndup(gtk_entry_get_text(GTK_ENTRY(dialog->textbox_client_appl)),STRLENGTH);
   label = gtk_bin_get_child(GTK_BIN(mpc->appl));
   g_sprintf(str, "%s %s", _("Launch"), mpc->client_appl);
   gtk_label_set_text(GTK_LABEL(label),str);

   DBG ("Apply: host=%s, port=%d, passwd=%s, appl=%s", mpc->mpd_host, mpc->mpd_port, mpc->mpd_password, mpc->client_appl);

   mpd_disconnect(mpc->mo);
   mpd_set_hostname(mpc->mo,mpc->mpd_host);
   mpd_set_port(mpc->mo,mpc->mpd_port);
   mpd_set_password(mpc->mo,mpc->mpd_password);
   mpd_connect(mpc->mo);
   if (strlen(mpc->mpd_password))
      mpd_send_password (mpc->mo);
}

static void
mpc_dialog_response (GtkWidget * dlg, int response, t_mpc_dialog * dialog)
{
   t_mpc *mpc = dialog->mpc;

   DBG ("!");

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

   DBG ("!");

   mpc->show_frame = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->checkbox_frame));
   gtk_frame_set_shadow_type (GTK_FRAME (mpc->frame), (mpc->show_frame) ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
}

static void
mpc_create_options (XfcePanelPlugin * plugin, t_mpc* mpc)
{
   GtkWidget *dlg, *vbox, *table;
   gchar str_port[20];
   t_mpc_dialog *dialog;

   DBG("!");

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

   table = gtk_table_new(4,2,FALSE);
   gtk_table_attach_defaults(GTK_TABLE(table),gtk_label_new(_("Host : ")),0,1,0,1);
   gtk_table_attach_defaults(GTK_TABLE(table),gtk_label_new(_("Port : ")),0,1,1,2);
   gtk_table_attach_defaults(GTK_TABLE(table),gtk_label_new(_("Password : ")),0,1,2,3);
   gtk_table_attach_defaults(GTK_TABLE(table),gtk_label_new(_("Client : ")),0,1,3,4);

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
   gtk_entry_set_text(GTK_ENTRY(dialog->textbox_password),mpc->mpd_password);
   gtk_table_attach_defaults(GTK_TABLE(table),dialog->textbox_password,1,2,2,3);

   dialog->textbox_client_appl = gtk_entry_new();
   gtk_entry_set_width_chars(GTK_ENTRY(dialog->textbox_client_appl),DIALOG_ENTRY_WIDTH);
   gtk_entry_set_text(GTK_ENTRY(dialog->textbox_client_appl),mpc->client_appl);
   gtk_table_attach_defaults(GTK_TABLE(table),dialog->textbox_client_appl,1,2,3,4);

   gtk_widget_show_all (table);
   gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

   dialog->checkbox_frame = gtk_check_button_new_with_mnemonic (_("Show _frame"));
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->checkbox_frame),mpc->show_frame);
   gtk_widget_show (dialog->checkbox_frame);
   gtk_box_pack_start (GTK_BOX (vbox), dialog->checkbox_frame, FALSE, FALSE, 0);

   g_signal_connect (dialog->checkbox_frame, "toggled", G_CALLBACK (mpc_dialog_show_frame_toggled), dialog);

   /* show the dialog */
   gtk_widget_show (dlg);
}

static void
mpc_launch_client(GtkWidget *widget, t_mpc* mpc)
{
   DBG("Going to xfce_exec(\"%s\")", mpc->client_appl);
   xfce_exec(mpc->client_appl, FALSE, TRUE, NULL);
}

static void
mpc_random_toggled(GtkWidget *widget, t_mpc* mpc)
{
   DBG("!");
   mpd_player_set_random(mpc->mo, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)));
}

static void
mpc_repeat_toggled(GtkWidget *widget, t_mpc* mpc)
{
   DBG("!");
   mpd_player_set_repeat(mpc->mo, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)));
}

static void
enter_cb(GtkWidget *widget, GdkEventCrossing* event, t_mpc* mpc)
{
   mpd_Song *song;
   gchar str[512];

   DBG("!");

   if (mpd_status_update(mpc->mo) != MPD_OK)
   {
      if (!mpc_plugin_reconnect (mpc) || mpd_status_update (mpc->mo) != MPD_OK)
      {
         gtk_tooltips_set_tip (mpc->tips, widget, _(".... not connected ?"), NULL);
         return;
      }
   }

   g_sprintf(str, "Volume : %d%%", mpd_status_get_volume(mpc->mo));

   switch (mpd_player_get_state(mpc->mo))
   {
      case MPD_PLAYER_PLAY:
         g_sprintf(str, "%s - Mpd Playing",str);
         break;
      case MPD_PLAYER_PAUSE:
         g_sprintf(str, "%s - Mpd Paused",str);
         break;
      case MPD_PLAYER_STOP:
         g_sprintf(str, "%s - Mpd Stopped",str);
         break;
      default:
         g_sprintf(str, "%s - Mpd state ?",str);
         break;
   }
   song = mpd_playlist_get_current_song(mpc->mo);
   if (song)
      g_sprintf(str,"%s - [%s - %s] -/- (#%s) %s", str, song->artist, song->album, song->track, song->title);
   else
      g_sprintf(str,"%s - Failed to get song info ?", str);

   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mpc->random), mpd_player_get_random(mpc->mo));
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mpc->repeat), mpd_player_get_repeat(mpc->mo));

   gtk_tooltips_set_tip (mpc->tips, widget, str, NULL);
}

static void
playlist_title_dblclicked (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, t_mpc* mpc)
{
   GtkTreeModel *model;
   GtkTreeIter iter;
   gint id = 0, pos = 0;

   model = gtk_tree_view_get_model(treeview);
   if (gtk_tree_model_get_iter(model, &iter, path))
   {
      gtk_tree_model_get(model, &iter, 2, &pos, 3, &id, -1);
      DBG("Dbl-clicked id %d (pos=%d)",id, pos);
      mpd_player_play_id(mpc->mo, id);
   }
   gtk_widget_destroy(mpc->playlist);
}

static void
show_playlist (t_mpc* mpc)
{
   DBG("!");
   GtkWidget *scrolledwin,*treeview;
   GtkListStore *liststore;
   GtkTreeIter iter;
   GtkTreePath *path_to_cur;
   GtkCellRenderer *renderer;
   gchar str[512];
   int current;
   MpdData *mpd_data;

   mpc->playlist = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_default_size(GTK_WINDOW(mpc->playlist), 300, 530);
   gtk_window_set_icon_name(GTK_WINDOW(mpc->playlist),"xfce-multimedia");
   gtk_window_set_title(GTK_WINDOW(mpc->playlist),_("Mpd playlist"));
   gtk_window_set_keep_above(GTK_WINDOW(mpc->playlist),TRUE); /* UGLY !!! */
   scrolledwin = gtk_scrolled_window_new(NULL, NULL);
   gtk_container_add(GTK_CONTAINER(mpc->playlist),GTK_WIDGET(scrolledwin));

   treeview = gtk_tree_view_new ();
   gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
   gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);
   g_signal_connect(treeview, "row-activated", G_CALLBACK(playlist_title_dblclicked), mpc);
   gtk_container_add(GTK_CONTAINER(scrolledwin),treeview);

   liststore = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
   gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL(liststore));

   renderer = gtk_cell_renderer_pixbuf_new ();
   gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "Icon", renderer, "stock-id", 0, NULL);
   renderer = gtk_cell_renderer_text_new ();
   gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "Title", renderer, "text", 1, NULL);

   mpc_plugin_reconnect(mpc);

   current = mpd_player_get_current_song_pos (mpc->mo);
   DBG ("Current song pos in the list: %d", current);
   mpd_data = mpd_playlist_get_changes (mpc->mo, -1);
   DBG ("Got playlist, creating window");
   for (;(mpd_data = mpd_data_get_next (mpd_data));)
   {
      g_sprintf(str,"%s - %s", mpd_data->song->artist, mpd_data->song->title);

      gtk_list_store_append (liststore, &iter);
      if (current == mpd_data->song->pos)
      {
         gtk_list_store_set (liststore, &iter, 0, "gtk-media-play", 1, str, 2, mpd_data->song->pos, 3, mpd_data->song->id, -1);
         path_to_cur = gtk_tree_model_get_path(GTK_TREE_MODEL(liststore), &iter);
         gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(treeview), path_to_cur, NULL, TRUE, 0.5, 0);
         gtk_tree_view_set_cursor(GTK_TREE_VIEW(treeview), path_to_cur, NULL, FALSE);
         gtk_tree_path_free(path_to_cur);
      }
      else
         gtk_list_store_set (liststore, &iter, 0, "gtk-cdrom", 1, str, 2, mpd_data->song->pos, 3, mpd_data->song->id, -1);
   }
   gtk_widget_show_all(mpc->playlist);
}

static void
toggle(GtkWidget *widget, GdkEventButton* event, t_mpc* mpc)
{
   if (1 == event->button)
   {
      if (mpd_status_update (mpc->mo) != MPD_OK)
         if (!mpc_plugin_reconnect (mpc))
            return;

      switch (mpd_player_get_state(mpc->mo))
      {
         case MPD_PLAYER_PAUSE:
         case MPD_PLAYER_PLAY:
            mpd_player_pause(mpc->mo); /* toggles play/pause */
            break;
         case MPD_PLAYER_STOP:
         default:
            mpd_player_play(mpc->mo); /* if stopped, mpd_player_pause() doesn't restart playing */
            break;
      }
   }
   else
      show_playlist(mpc);
}

static void
prev(GtkWidget *widget, GdkEventButton* event, t_mpc* mpc)
{
   if (1 == event->button)
   {
      if (mpd_player_prev(mpc->mo) != MPD_OK)
      {
         if (mpc_plugin_reconnect(mpc))
         {
            DBG("calling mpd_player_prev() after reconnect");
            mpd_player_prev(mpc->mo);
         }
      }
      else
         DBG("mpd_player_prev() ok");
   }
   else
      show_playlist(mpc);
}

static void
stop(GtkWidget *widget, GdkEventButton* event, t_mpc* mpc)
{
   if (1 == event->button)
   {
      if (mpd_player_stop(mpc->mo) != MPD_OK)
      {
         if (mpc_plugin_reconnect(mpc))
         {
            DBG("calling mpd_player_stop() after reconnect");
            mpd_player_stop(mpc->mo);
         }
      }
      else
         DBG("mpd_player_stop() ok");
   }
   else
      show_playlist(mpc);
}

static void
next(GtkWidget *widget, GdkEventButton* event, t_mpc* mpc)
{
   if (1 == event->button)
   {
      if (mpd_player_next(mpc->mo) != MPD_OK)
      {
         if (mpc_plugin_reconnect(mpc))
         {
            DBG("calling mpd_player_next() after reconnect");
            mpd_player_next(mpc->mo);
         }
      }
      else
         DBG("mpd_player_next() ok");
   }
   else
      show_playlist(mpc);
}

static void
scroll_cb(GtkWidget *widget, GdkEventScroll* event, t_mpc* mpc)
{
   int curvol=-1;

   if (event->type != GDK_SCROLL)
      return;
   else if (mpd_status_update (mpc->mo) != MPD_OK)
   {
      if (!mpc_plugin_reconnect (mpc) || mpd_status_update (mpc->mo) != MPD_OK)
      {
         gtk_tooltips_set_tip (mpc->tips, widget, _(".... not connected ?"), NULL);
         return;
      }
   }

   curvol = mpd_status_get_volume(mpc->mo);
   DBG("current volume=%d", curvol);
   curvol = ((event->direction == GDK_SCROLL_DOWN) ? curvol-5 : curvol+5);
   DBG("setting new volume=%d", curvol);
   mpd_status_set_volume(mpc->mo,curvol);
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

   DBG("!");

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
   new_button_with_img(plugin, mpc->box, mpc->toggle, GTK_STOCK_MEDIA_PAUSE, G_CALLBACK(toggle), mpc);
   new_button_with_img(plugin, mpc->box, mpc->next, GTK_STOCK_MEDIA_NEXT, G_CALLBACK(next), mpc);

   mpc->random = gtk_check_menu_item_new_with_label (_("Random"));
   g_signal_connect (G_OBJECT(mpc->random), "toggled", G_CALLBACK (mpc_random_toggled), mpc);
   mpc->repeat = gtk_check_menu_item_new_with_label (_("Repeat"));
   g_signal_connect (G_OBJECT(mpc->repeat), "toggled", G_CALLBACK (mpc_repeat_toggled), mpc);
   mpc->appl = gtk_menu_item_new_with_label (_("Launch"));
   g_signal_connect (G_OBJECT(mpc->appl), "activate", G_CALLBACK (mpc_launch_client), mpc);
   xfce_panel_plugin_menu_insert_item(plugin,GTK_MENU_ITEM(mpc->random));
   xfce_panel_plugin_menu_insert_item(plugin,GTK_MENU_ITEM(mpc->repeat));
   xfce_panel_plugin_menu_insert_item(plugin,GTK_MENU_ITEM(mpc->appl));
   gtk_widget_show (mpc->repeat);
   gtk_widget_show (mpc->random);
   gtk_widget_show (mpc->appl);
   gtk_widget_show_all (mpc->box);

   return mpc;
}

static void
mpc_construct (XfcePanelPlugin * plugin)
{
   t_mpc *mpc;

   DBG("!");
#if DEBUG
#ifdef HAVE_LIBMPD
   debug_set_level(10);
#endif
#endif

   xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

   /* create widgets */
   mpc = mpc_create (plugin);
   mpc->mpd_host = g_strdup(DEFAULT_MPD_HOST);
   mpc->mpd_port = DEFAULT_MPD_PORT;
   mpc->mpd_password = g_strdup("");
   mpc->client_appl = g_strdup("");
   mpc->show_frame = TRUE;
   mpc->playlist = NULL;

   mpc_read_config (plugin, mpc);

   /* create a connection*/
   mpc->mo = mpd_new(mpc->mpd_host,mpc->mpd_port,mpc->mpd_password);

   gtk_container_add (GTK_CONTAINER (plugin), mpc->frame);
   gtk_frame_set_shadow_type (GTK_FRAME (mpc->frame), ((mpc->show_frame) ? GTK_SHADOW_IN : GTK_SHADOW_NONE));

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


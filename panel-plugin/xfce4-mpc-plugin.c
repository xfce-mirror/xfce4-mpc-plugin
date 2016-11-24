/* xfce4-mpc-plugin.c
 *
 * Copyright (c) 2006-2016 Landry Breuil <landry at xfce.org>
 * This code is licensed under a BSD-style license.
 * (OpenBSD variant modeled after the ISC license)
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

#include <libxfce4ui/libxfce4ui.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gprintf.h>

#include "xfce4-mpc-plugin.h"

#define DEFAULT_MPD_HOST "localhost"
#define DEFAULT_MPD_PORT 6600
#define DIALOG_ENTRY_WIDTH 20

static void resize_button(GtkWidget *, gint);

static void
mpc_free (XfcePanelPlugin * plugin, t_mpc * mpc)
{
   DBG ("!");

   mpd_free(mpc->mo);
   g_free (mpc);
}

static void
mpc_set_mode (XfcePanelPlugin * plugin, XfcePanelPluginMode mode, t_mpc * mpc)
{
   GtkOrientation orientation;
   DBG ("!");

   orientation =
      (mode != XFCE_PANEL_PLUGIN_MODE_VERTICAL) ?
      GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;

   gtk_orientable_set_orientation(GTK_ORIENTABLE(mpc->box), orientation);
   xfce_panel_plugin_set_small (plugin, (mode != XFCE_PANEL_PLUGIN_MODE_DESKBAR));
}

static gboolean
mpc_set_size (XfcePanelPlugin * plugin, int size, t_mpc * mpc)
{
   int border_width = (size > 26 && mpc->show_frame ? 1 : 0);
    size /= xfce_panel_plugin_get_nrows (plugin);

   DBG ("size=%d",size);
   gtk_container_set_border_width (GTK_CONTAINER (mpc->frame), border_width);
   size -= 2 * border_width;
   resize_button (GTK_WIDGET (mpc->next), size);
   resize_button (GTK_WIDGET (mpc->prev), size);
   resize_button (GTK_WIDGET (mpc->stop), size);
   resize_button (GTK_WIDGET (mpc->toggle), size);
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
   if (mpc->tooltip_format != NULL)
      g_free (mpc->tooltip_format);
   if (mpc->playlist_format != NULL)
      g_free (mpc->playlist_format);
   if (mpc->client_appl != NULL)
      g_free (mpc->client_appl);

   mpc->mpd_host = g_strdup(xfce_rc_read_entry (rc, "mpd_host",  DEFAULT_MPD_HOST));
   mpc->mpd_port = xfce_rc_read_int_entry (rc, "mpd_port", DEFAULT_MPD_PORT);
   mpc->mpd_password = g_strdup(xfce_rc_read_entry (rc, "mpd_password", ""));
   mpc->show_frame = xfce_rc_read_bool_entry (rc, "show_frame", TRUE);
   mpc->client_appl = g_strdup(xfce_rc_read_entry (rc, "client_appl", "SETME"));
   mpc->tooltip_format = g_strdup(xfce_rc_read_entry (rc, "tooltip_format", "Volume : %vol%% - Mpd %status%%newline%%%artist% - %album% -/- (#%track%) %title%"));
   mpc->playlist_format = g_strdup(xfce_rc_read_entry (rc, "playlist_format", "%artist% - %album% -/- (#%track%) %title%"));

   label = gtk_bin_get_child(GTK_BIN(mpc->appl));
   g_snprintf(str, sizeof(str), "%s %s", _("Launch"), mpc->client_appl);
   gtk_label_set_text(GTK_LABEL(label),str);

   DBG ("Settings read : %s@%s:%d\nframe:%d\nappl:%s\ntooltip:%s\nplaylist:%s", mpc->mpd_password, mpc->mpd_host, mpc->mpd_port, mpc->show_frame, mpc->client_appl, mpc->tooltip_format, mpc->playlist_format);
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
   xfce_rc_write_entry (rc, "tooltip_format", mpc->tooltip_format);
   xfce_rc_write_entry (rc, "playlist_format", mpc->playlist_format);

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
   GtkWidget *label;
   char str[128];

   t_mpc *mpc = dialog->mpc;
   DBG ("!");
   mpc->mpd_host = g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog->textbox_host)));
   mpc->mpd_port = atoi(gtk_entry_get_text(GTK_ENTRY(dialog->textbox_port)));
   mpc->mpd_password = g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog->textbox_password)));
   mpc->client_appl = g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog->textbox_client_appl)));
   mpc->tooltip_format = g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog->textbox_tooltip_format)));
   mpc->playlist_format = g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog->textbox_playlist_format)));

   if (0 == strlen(mpc->client_appl))
      mpc->client_appl = g_strdup("SETME");
   if (0 == strlen(mpc->tooltip_format))
      mpc->tooltip_format = g_strdup("Volume : %vol%% - Mpd %status%%newline%%artist% - %album% -/- (#%track%) %title%");
   if (0 == strlen(mpc->playlist_format))
      mpc->playlist_format = g_strdup("%artist% - %album% -/- (#%track%) %title%");

   label = gtk_bin_get_child(GTK_BIN(mpc->appl));
   g_snprintf(str, sizeof(str), "%s %s", _("Launch"), mpc->client_appl);
   gtk_label_set_text(GTK_LABEL(label),str);

   DBG ("Apply: host=%s, port=%d, passwd=%s, appl=%s\ntooltip=%s\nplaylist=%s", mpc->mpd_host, mpc->mpd_port, mpc->mpd_password, mpc->client_appl, mpc->tooltip_format, mpc->playlist_format);

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
mpc_dialog_show_frame_toggled (GtkWidget *w, gboolean state, t_mpc_dialog *dialog)
{
   int size;
   t_mpc* mpc = dialog->mpc;

   DBG ("!");
   size = xfce_panel_plugin_get_size(mpc->plugin);
   mpc->show_frame = state;
   gtk_frame_set_shadow_type (GTK_FRAME (mpc->frame), (mpc->show_frame) ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
   gtk_switch_set_state(GTK_SWITCH(w), state);
   mpc_set_size(mpc->plugin, size, mpc);
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
                                              GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(plugin))),
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              "gtk-close",
                                              GTK_RESPONSE_OK,
                                              NULL);
   xfce_titled_dialog_set_subtitle (XFCE_TITLED_DIALOG (dlg), _("Properties"));

   gtk_window_set_position   (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
   gtk_window_set_icon_name  (GTK_WINDOW (dlg), "applications-multimedia");

   g_signal_connect (dlg, "response", G_CALLBACK (mpc_dialog_response), dialog);

   vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
   gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
   gtk_widget_show (vbox);
   gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (dlg))), vbox, TRUE, TRUE, 0);

   table = gtk_grid_new();
   gtk_grid_set_column_spacing (GTK_GRID(table), 2);
   gtk_grid_set_row_spacing (GTK_GRID(table), 2);
   gtk_grid_attach(GTK_GRID(table),gtk_label_new(_("Host : ")),0,0,1,1);
   gtk_grid_attach(GTK_GRID(table),gtk_label_new(_("Port : ")),0,1,1,1);
   gtk_grid_attach(GTK_GRID(table),gtk_label_new(_("Password : ")),0,2,1,1);
   gtk_grid_attach(GTK_GRID(table),gtk_label_new(_("MPD Client : ")),0,3,1,1);
   gtk_grid_attach(GTK_GRID(table),gtk_label_new(_("Tooltip Format : ")),0,4,1,1);
   gtk_grid_attach(GTK_GRID(table),gtk_label_new(_("Playlist Format : ")),0,5,1,1);
   gtk_grid_attach(GTK_GRID(table),gtk_label_new_with_mnemonic(_("Show _frame")),0,6,1,1);

   dialog->textbox_host = gtk_entry_new();
   gtk_entry_set_width_chars(GTK_ENTRY(dialog->textbox_host),DIALOG_ENTRY_WIDTH);
   gtk_entry_set_text(GTK_ENTRY(dialog->textbox_host),mpc->mpd_host);
   gtk_grid_attach(GTK_GRID(table),dialog->textbox_host,1,0,1,1);

   dialog->textbox_port = gtk_entry_new();
   gtk_entry_set_width_chars(GTK_ENTRY(dialog->textbox_port),DIALOG_ENTRY_WIDTH);
   g_snprintf(str_port,sizeof(str_port),"%d",mpc->mpd_port);
   gtk_entry_set_text(GTK_ENTRY(dialog->textbox_port),str_port);
   gtk_grid_attach(GTK_GRID(table),dialog->textbox_port,1,1,1,1);

   dialog->textbox_password = gtk_entry_new();
   gtk_entry_set_visibility(GTK_ENTRY(dialog->textbox_password),FALSE);
   gtk_entry_set_width_chars(GTK_ENTRY(dialog->textbox_password),DIALOG_ENTRY_WIDTH);
   gtk_entry_set_text(GTK_ENTRY(dialog->textbox_password),mpc->mpd_password);
   gtk_grid_attach(GTK_GRID(table),dialog->textbox_password,1,2,1,1);

   dialog->textbox_client_appl = gtk_entry_new();
   gtk_entry_set_width_chars(GTK_ENTRY(dialog->textbox_client_appl),DIALOG_ENTRY_WIDTH);
   gtk_entry_set_text(GTK_ENTRY(dialog->textbox_client_appl),mpc->client_appl);
   gtk_grid_attach(GTK_GRID(table),dialog->textbox_client_appl,1,3,1,1);

   dialog->textbox_tooltip_format = gtk_entry_new();
   gtk_entry_set_width_chars(GTK_ENTRY(dialog->textbox_tooltip_format),DIALOG_ENTRY_WIDTH);
   gtk_entry_set_text(GTK_ENTRY(dialog->textbox_tooltip_format),mpc->tooltip_format);
   gtk_grid_attach(GTK_GRID(table),dialog->textbox_tooltip_format,1,4,1,1);

   dialog->textbox_playlist_format = gtk_entry_new();
   gtk_entry_set_width_chars(GTK_ENTRY(dialog->textbox_playlist_format),DIALOG_ENTRY_WIDTH);
   gtk_entry_set_text(GTK_ENTRY(dialog->textbox_playlist_format),mpc->playlist_format);
   gtk_grid_attach(GTK_GRID(table),dialog->textbox_playlist_format,1,5,1,1);

   dialog->checkbox_frame = gtk_switch_new();
   gtk_switch_set_active(GTK_SWITCH(dialog->checkbox_frame),mpc->show_frame);

   g_signal_connect (dialog->checkbox_frame, "state-set", G_CALLBACK (mpc_dialog_show_frame_toggled), dialog);
   gtk_grid_attach(GTK_GRID(table),dialog->checkbox_frame,1,6,1,1);

   gtk_widget_set_tooltip_text (dialog->textbox_host, _("Hostname or IP address"));
   gtk_widget_set_tooltip_text (dialog->textbox_client_appl, _("Graphical MPD Client to launch in plugin context menu"));
   gtk_widget_set_tooltip_text (dialog->textbox_playlist_format, _("Variables : %artist%, %album%, %file%, %track% and %title%"));
   gtk_widget_set_tooltip_text (dialog->textbox_tooltip_format, _("Variables : %vol%, %status%, %newline%, %artist%, %album%, %file%, %track% and %title%"));

   gtk_widget_show_all (table);
   gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

   /* show the dialog */
   gtk_widget_show (dlg);
}

static void
mpc_launch_client(GtkWidget *widget, t_mpc* mpc)
{
   DBG("Going to xfce_exec(\"%s\")", mpc->client_appl);
   xfce_spawn_command_line_on_screen(gdk_screen_get_default(), mpc->client_appl, FALSE, TRUE, NULL);
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
mpc_output_toggled(GtkWidget *widget, t_mpc* mpc)
{
   int i;
   DBG("!");
   /* lookup menuitem */
   for (i = 0; i < mpc->nb_outputs && mpc->mpd_outputs[i]->menuitem != widget; i++);
   if (i != mpc->nb_outputs) /* oops case ? */
      /* set corresponding mpd output status */
      mpd_server_set_output_device(mpc->mo, mpc->mpd_outputs[i]->id, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)));
}

/* get output list from mpd, add a checkbox to rightclick menu for each */
static void
mpc_update_outputs(t_mpc* mpc)
{
   GtkWidget *chkitem;
   int i,j=0, old_nb_outputs = mpc->nb_outputs;
   MpdData * data = mpd_server_get_output_devices(mpc->mo);
   DBG("!");
   do {
      DBG("got output %d with name %s, enabled=%d",data->output_dev->id,data->output_dev->name,data->output_dev->enabled);
      /* check if this output doesn't already exist */
      for (i = 0; i < mpc->nb_outputs && mpc->mpd_outputs[i]->id != data->output_dev->id ; i++);

      if (i == mpc->nb_outputs) {
         DBG("output not found, adding a new checkitem at pos %d",i);
         chkitem = gtk_check_menu_item_new_with_label (data->output_dev->name);
         g_signal_connect (G_OBJECT(chkitem), "toggled", G_CALLBACK (mpc_output_toggled), mpc);
         xfce_panel_plugin_menu_insert_item(mpc->plugin,GTK_MENU_ITEM(chkitem));
         gtk_widget_show (chkitem);
         mpc->mpd_outputs[i] = g_new(t_mpd_output,1);
         mpc->mpd_outputs[i]->id = data->output_dev->id;
         mpc->mpd_outputs[i]->menuitem = chkitem;
         mpc->nb_outputs++;
      }
      mpc->mpd_outputs[i]->enabled = data->output_dev->enabled;
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mpc->mpd_outputs[i]->menuitem), mpc->mpd_outputs[i]->enabled);
      j++;
   } while (NULL != (data = mpd_data_get_next (data)));
   /* something changed, better prune the list and recreate it */
   if (j != mpc->nb_outputs || (old_nb_outputs && j != old_nb_outputs)) {
      DBG("didnt found same amount of outputs (was %d got %d), resetting output list", (old_nb_outputs ? old_nb_outputs : mpc->nb_outputs), j);
      for (i = 0; i < mpc->nb_outputs ; i++) {
         gtk_widget_destroy(mpc->mpd_outputs[i]->menuitem);
         g_free(mpc->mpd_outputs[i]);
      }
      mpc->nb_outputs = 0;
      mpc_update_outputs(mpc);
   }
}

static void
str_replace(GString *str, gchar* pattern, gchar* replacement)
{
   gchar *nstr;
   GRegex *regex;
   if (!replacement)
      return;
   regex = g_regex_new(pattern, 0, 0, NULL);
   nstr = g_regex_replace_literal( regex, str->str, -1, 0, replacement, 0, NULL);
   g_regex_unref (regex);
   g_string_assign(str, nstr);
   g_free(nstr);
}

static void
format_song_display(mpd_Song* song, GString *str, t_mpc* mpc)
{
   if (0 == str->len)
      g_string_assign(str, mpc->playlist_format);

   /* replace %artist% by song->artist, etc */
   str_replace(str, "%artist%", song->artist);
   str_replace(str, "%album%", song->album);
   str_replace(str, "%title%", song->title);
   str_replace(str, "%track%", song->track);
   str_replace(str, "%file%", song->file);
}

static gboolean
enter_cb(GtkWidget *widget, GdkEventCrossing* event, t_mpc* mpc)
{
   mpd_Song *song;
   GString *str = NULL;
   gchar vol[20];
   DBG("!");

   if (mpd_status_update(mpc->mo) != MPD_OK)
   {
      if (!mpc_plugin_reconnect (mpc) || mpd_status_update (mpc->mo) != MPD_OK)
      {
         gtk_widget_set_tooltip_text (mpc->box, _(".... not connected ?"));
         return FALSE;
      }
   }
   str = g_string_new(mpc->tooltip_format);

   /* replace %vol% by mpd_status_get_volume(mpc->mo) */
   g_sprintf(vol, "%d", mpd_status_get_volume(mpc->mo));
   str_replace(str, "%vol%", vol);
   /* newlines */
   str_replace(str, "%newline%", "\n");
   /* replace %status% by mpd status string */
   switch (mpd_player_get_state(mpc->mo))
   {
      case MPD_PLAYER_PLAY:
         str_replace(str, "%status%", "Playing");
         break;
      case MPD_PLAYER_PAUSE:
         str_replace(str, "%status%", "Paused");
         break;
      case MPD_PLAYER_STOP:
         str_replace(str, "%status%", "Stopped");
         break;
      default:
         str_replace(str, "%status%", "state ?");
         break;
   }
   song = mpd_playlist_get_current_song(mpc->mo);
   if (song && song->id != -1)
      format_song_display(song, str, mpc);
   else
      g_string_assign(str, "Failed to get song info ?");

   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mpc->random), mpd_player_get_random(mpc->mo));
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mpc->repeat), mpd_player_get_repeat(mpc->mo));
   mpc_update_outputs(mpc);

   gtk_widget_set_tooltip_text (mpc->box, str->str);
   g_string_free(str, TRUE);
   return FALSE;
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
   GtkWidget *scrolledwin, *treeview;
   GtkListStore *liststore;
   GtkTreeIter iter;
   GtkTreePath *path_to_cur;
   GtkCellRenderer *renderer;
   GString *str;
   int current;
   MpdData *mpd_data;
   DBG("!");

   str = g_string_new('\0');
   if (mpc->playlist)
   {
      gtk_window_present(GTK_WINDOW(mpc->playlist));
      return;
   }
   /* create playlist window only if playlist is not empty */
   if (0 != mpd_playlist_get_playlist_length(mpc->mo))
   {
      DBG ("Creating playlist window");
      mpc->playlist = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_window_set_default_size(GTK_WINDOW(mpc->playlist), 400, 600);
      gtk_window_set_icon_name(GTK_WINDOW(mpc->playlist),"applications-multimedia");
      gtk_window_set_title(GTK_WINDOW(mpc->playlist),_("Mpd playlist"));
      g_signal_connect(mpc->playlist, "destroy", G_CALLBACK(gtk_widget_destroyed), &mpc->playlist);
      scrolledwin = gtk_scrolled_window_new(NULL, NULL);
      gtk_container_add(GTK_CONTAINER(mpc->playlist),GTK_WIDGET(scrolledwin));

      treeview = gtk_tree_view_new ();
      gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
      g_signal_connect(treeview, "row-activated", G_CALLBACK(playlist_title_dblclicked), mpc);
      gtk_container_add(GTK_CONTAINER(scrolledwin),treeview);

      liststore = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
      gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL(liststore));

      renderer = gtk_cell_renderer_pixbuf_new ();
      gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "Icon", renderer, "stock-id", 0, NULL);
      renderer = gtk_cell_renderer_text_new ();
      gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "Title", renderer, "text", 1, NULL);

      if (!mpc_plugin_reconnect(mpc))
      {
         gtk_widget_destroy(mpc->playlist);
         return;
      }

      current = mpd_player_get_current_song_pos (mpc->mo);
      DBG ("Current song pos in the list: %d", current);
      mpd_data = mpd_playlist_get_changes (mpc->mo, -1);
      DBG ("Got playlist, filling treeview");
      do
      {
         g_string_erase(str, 0, -1);
         format_song_display(mpd_data->song, str, mpc);

         gtk_list_store_append (liststore, &iter);
         if (current == mpd_data->song->pos)
         {
            gtk_list_store_set (liststore, &iter, 0, "gtk-media-play", 1, str->str, 2, mpd_data->song->pos, 3, mpd_data->song->id, -1);
            path_to_cur = gtk_tree_model_get_path(GTK_TREE_MODEL(liststore), &iter);
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(treeview), path_to_cur, NULL, TRUE, 0.5, 0);
            gtk_tree_view_set_cursor(GTK_TREE_VIEW(treeview), path_to_cur, NULL, FALSE);
            gtk_tree_path_free(path_to_cur);
         }
         else
            gtk_list_store_set (liststore, &iter, 0, "gtk-cdrom", 1, str->str, 2, mpd_data->song->pos, 3, mpd_data->song->id, -1);
      } while (NULL != (mpd_data = mpd_data_get_next (mpd_data)));
      gtk_widget_show_all(mpc->playlist);
   }
   g_string_free(str, TRUE);
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
         gtk_widget_set_tooltip_text(widget, _(".... not connected ?"));
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
resize_button(GtkWidget *btn, gint size)
{
   GtkIconTheme *icon_theme;
   GdkPixbuf *pixbuf, *scaled;
   GtkWidget *image = g_object_get_data(G_OBJECT(btn), "image");
   gchar *icon = g_object_get_data(G_OBJECT(image), "icon-name");
   icon_theme = gtk_icon_theme_get_default();
   DBG("Resizing button, loading icon %s and rescaling it to size %d", icon, size / 2 - 2);
   pixbuf = gtk_icon_theme_load_icon (icon_theme, icon, size / 2 - 2, 0, NULL);
   gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
   g_object_unref (G_OBJECT (pixbuf));
   gtk_widget_set_size_request (btn, size, size);
}

static GtkWidget*
new_button_with_cbk(XfcePanelPlugin * plugin, GtkWidget *parent, gchar* icon, gpointer cb, gpointer data)
{
   GtkWidget *button = xfce_panel_create_button();
   GtkWidget *image = gtk_image_new();
   g_object_set_data(G_OBJECT(image), "icon-name", icon);
   g_object_set_data(G_OBJECT(button), "image", image);
   gtk_container_add(GTK_CONTAINER(button), image);
   xfce_panel_plugin_add_action_widget (plugin, button);
   g_signal_connect (G_OBJECT(button), "button_press_event", G_CALLBACK(cb), data);
   g_signal_connect (G_OBJECT(button), "enter_notify_event", G_CALLBACK(enter_cb), data);
   g_signal_connect (G_OBJECT(button), "scroll_event", G_CALLBACK(scroll_cb), data);
   gtk_widget_add_events(GTK_WIDGET(button), GDK_SCROLL_MASK);
   gtk_box_pack_start (GTK_BOX(parent), button, TRUE, TRUE, 0);
   return button;
}

static void
add_separator_and_label_with_markup(XfcePanelPlugin* plugin, gchar* lbl)
{
   GtkWidget *separator, *menuitem, *label;
   separator = gtk_separator_menu_item_new();
   menuitem = gtk_menu_item_new_with_label(lbl);
   gtk_widget_set_sensitive(menuitem, FALSE);
   label = gtk_bin_get_child(GTK_BIN(menuitem));
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   xfce_panel_plugin_menu_insert_item(plugin,GTK_MENU_ITEM(separator));
   xfce_panel_plugin_menu_insert_item(plugin,GTK_MENU_ITEM(menuitem));
   gtk_widget_show (separator);
   gtk_widget_show (menuitem);
}

static t_mpc*
mpc_create (XfcePanelPlugin * plugin)
{
   t_mpc *mpc;

   DBG("!");

   mpc = g_new0 (t_mpc, 1);

   mpc->plugin = plugin;

   mpc->frame = gtk_frame_new (NULL);
   gtk_frame_set_shadow_type (GTK_FRAME (mpc->frame), GTK_SHADOW_IN);
   gtk_widget_show (mpc->frame);

   mpc->box = gtk_box_new(xfce_panel_plugin_get_orientation(plugin), 0);

   gtk_container_add (GTK_CONTAINER(mpc->frame), mpc->box);

   mpc->prev = new_button_with_cbk(plugin, mpc->box, "media-skip-backward", G_CALLBACK(prev), mpc);
   mpc->stop = new_button_with_cbk(plugin, mpc->box, "media-playback-stop", G_CALLBACK(stop), mpc);
   mpc->toggle = new_button_with_cbk(plugin, mpc->box, "media-playback-pause", G_CALLBACK(toggle), mpc);
   mpc->next = new_button_with_cbk(plugin, mpc->box, "media-skip-forward", G_CALLBACK(next), mpc);

   mpc->random = gtk_check_menu_item_new_with_label (_("Random"));
   g_signal_connect (G_OBJECT(mpc->random), "toggled", G_CALLBACK (mpc_random_toggled), mpc);
   mpc->repeat = gtk_check_menu_item_new_with_label (_("Repeat"));
   g_signal_connect (G_OBJECT(mpc->repeat), "toggled", G_CALLBACK (mpc_repeat_toggled), mpc);
   mpc->appl = gtk_menu_item_new_with_label (_("Launch"));
   g_signal_connect (G_OBJECT(mpc->appl), "activate", G_CALLBACK (mpc_launch_client), mpc);

   add_separator_and_label_with_markup(plugin, _("<b><i>Commands</i></b>"));
   xfce_panel_plugin_menu_insert_item(plugin,GTK_MENU_ITEM(mpc->random));
   xfce_panel_plugin_menu_insert_item(plugin,GTK_MENU_ITEM(mpc->repeat));
   xfce_panel_plugin_menu_insert_item(plugin,GTK_MENU_ITEM(mpc->appl));
   add_separator_and_label_with_markup(plugin, _("<b><i>Outputs</i></b>"));
   gtk_widget_show (mpc->repeat);
   gtk_widget_show (mpc->random);
   gtk_widget_show (mpc->appl);
   gtk_widget_show_all (mpc->box);

   return mpc;
}

static void
mpc_show_about(XfcePanelPlugin *plugin, t_mpc* mpc)
{
   GdkPixbuf *icon;
   const gchar *auth[] = { "Landry Breuil <landry at xfce.org>", NULL };
   icon = xfce_panel_pixbuf_from_source("applications-multimedia", NULL, 32);
   gtk_show_about_dialog(NULL,
      "logo", icon,
      "license", xfce_get_license_text (XFCE_LICENSE_TEXT_BSD),
      "version", PACKAGE_VERSION,
      "program-name", PACKAGE_NAME,
      "comments", _("A simple panel-plugin client for Music Player Daemon"),
      "website", "http://goodies.xfce.org/projects/panel-plugins/xfce4-mpc-plugin",
      "copyright", _("Copyright (c) 2006-2016 Landry Breuil\n"),
      "authors", auth, NULL);

   if(icon)
      g_object_unref(G_OBJECT(icon));
}

static void
mpc_construct (XfcePanelPlugin * plugin)
{
   t_mpc *mpc;

   DBG("!");
#if DEBUG
#if HAVE_LIBMPD
   debug_set_level(10);
#endif
#endif

   xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

   /* create widgets */
   mpc = mpc_create (plugin);
   /* default values when no configuration is found */
   mpc->mpd_host = g_strdup(DEFAULT_MPD_HOST);
   mpc->mpd_port = DEFAULT_MPD_PORT;
   mpc->mpd_password = g_strdup("");
   mpc->client_appl = g_strdup("SETME");
   mpc->tooltip_format = g_strdup("Volume : %vol%% - Mpd %status%%newline%%artist% - %album% -/- (#%track%) %title%");
   mpc->playlist_format = g_strdup("%artist% - %album% -/- (#%track%) %title%");
   mpc->show_frame = TRUE;
   mpc->playlist = NULL;
   mpc->mpd_outputs = g_new(t_mpd_output*,1);
   mpc->nb_outputs = 0;

   mpc_read_config (plugin, mpc);

   /* create a connection*/
   mpc->mo = mpd_new(mpc->mpd_host,mpc->mpd_port,mpc->mpd_password);

   gtk_container_add (GTK_CONTAINER (plugin), mpc->frame);
   gtk_frame_set_shadow_type (GTK_FRAME (mpc->frame), ((mpc->show_frame) ? GTK_SHADOW_IN : GTK_SHADOW_NONE));

    /* connect signal handlers */
   g_signal_connect (plugin, "free-data", G_CALLBACK (mpc_free), mpc);
   g_signal_connect (plugin, "save", G_CALLBACK (mpc_write_config), mpc);
   g_signal_connect (plugin, "size-changed", G_CALLBACK (mpc_set_size), mpc);
   g_signal_connect (plugin, "mode-changed", G_CALLBACK (mpc_set_mode), mpc);
   xfce_panel_plugin_set_small (plugin, TRUE);
   /* the configure and about menu items are hidden by default */
   xfce_panel_plugin_menu_show_configure (plugin);

   g_signal_connect (plugin, "configure-plugin", G_CALLBACK (mpc_create_options), mpc);
   xfce_panel_plugin_menu_show_about(plugin);
   g_signal_connect (plugin, "about", G_CALLBACK (mpc_show_about), mpc);
}


/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER (mpc_construct);


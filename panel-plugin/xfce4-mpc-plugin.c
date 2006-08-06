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

/* double inclusion ?*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfcegui4/libxfcegui4.h>

#define DEFAULT_MPD_HOST "localhost"
#define DEFAULT_MPD_PORT 6600
#define DIALOG_ENTRY_WIDTH 15
#define STRLENGTH 32

#include "xfce4-mpc-plugin.h"

#ifndef HAVE_LIBMPD

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

MpdObj* mpd_new(char* host, int port, char* pass)
{
   MpdObj* mo = g_new0(MpdObj,1);

   DBG("host=%s, port=%d, pass=%s", host, port, pass);
   
   mo->host = g_strndup(host,STRLENGTH);
   mo->port = port;
   mo->pass = g_strndup(pass,STRLENGTH);
   mo->socket = 0;
   mo->status = 0;
   mo->curvol = 0;
   mo->error = 0;
   mo->buffer[0] = '\0';
   mo->buflen = 0;

   return mo;
}
void mpd_free(MpdObj* mo)
{
   DBG("!");
   
   if (mo->socket)
      close(mo->socket);
   g_free(mo->host);
   g_free(mo->pass);
   g_free(mo);
}
void mpd_connect(MpdObj* mo)
{
   struct hostent* remote_he;
   struct sockaddr* remote_sa;
   struct sockaddr_in remote_si;
   int flags,err,nbread;
   char* temp;
   struct timeval tv;
   fd_set fds;

   mo->buffer[0] = '\0';
   mo->buflen = 0;

   /* ??? */
   if (mo->socket) close(mo->socket);

   DBG("!");
  
   if (!(remote_he = (struct hostent*) gethostbyname(mo->host)))
   {
      mo->error = MPD_ERROR_UNKHOST;
      DBG("ERROR @gethostbyname(), err=%s",strerror(errno));
      return;
   }
   memset(&remote_si, 0, sizeof(struct sockaddr_in));
   remote_si.sin_family = AF_INET;
   remote_si.sin_port = htons(mo->port);
   memcpy((char *)&remote_si.sin_addr.s_addr,( char *)remote_he->h_addr, remote_he->h_length);

   remote_sa = (struct sockaddr *)&remote_si;

   if ((mo->socket = socket(AF_INET,SOCK_STREAM,0)) < 0)
   {
      mo->error = MPD_ERROR_NOSOCK;
      DBG("ERROR @socket(), err=%s",strerror(errno));
      return;
   }
   
   flags = fcntl(mo->socket, F_GETFL, 0);
   fcntl(mo->socket, F_SETFL, flags | O_NONBLOCK);
   if (connect(mo->socket,remote_sa, sizeof(struct sockaddr_in)) < 0 && errno != EINPROGRESS)
   {
      mo->error = MPD_ERROR_CONNPORT;
      DBG("ERROR @connect(), err=%s",strerror(errno));
      return;
   }

   while(!(temp = strstr(mo->buffer,"\n"))) 
   {
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      FD_ZERO(&fds);
      FD_SET(mo->socket,&fds);
      if((err = select(mo->socket+1,&fds,NULL,NULL,&tv)) == 1) 
      {
	 if ((nbread = recv(mo->socket,&(mo->buffer[mo->buflen]), MAXBUFLEN-mo->buflen,0)) <= 0)
	 {
	    mo->error = MPD_ERROR_NORESPONSE;
	    DBG("ERROR @recv(), err=%s",strerror(errno));
	    return;
	 }
	 mo->buflen+=nbread;
	 mo->buffer[mo->buflen] = '\0';
      }
      else if(err < 0)
      {
	 if (errno == EINTR)
	    continue;
	 mo->error = MPD_ERROR_CONNPORT;
	 DBG("ERROR @select(), err=%s",strerror(errno));
	 return;
      }
      else
      {
	 mo->error = MPD_ERROR_NORESPONSE;
	 DBG("ERROR @select(), timeoute'ed -> err=%s",strerror(errno));
	 return;
      }
   }


   if (strncmp(mo->buffer,MPD_WELCOME_MESSAGE, strlen(MPD_WELCOME_MESSAGE)))
   {
      mo->error = MPD_ERROR_NOTMPD;
      DBG("ERROR @strncmp() -> answer didn't come from mpd");
      return;
   }

   DBG("Received welcome message :\"%s\"",mo->buffer);
   
   *temp = '\0';
   strcpy(mo->buffer, temp+1);
   mo->buflen = strlen(mo->buffer);
}

void mpd_disconnect(MpdObj* mo)
{
   DBG("!");
   if (mo->socket) 
      close(mo->socket);
}

int mpd_status_get_volume(MpdObj* mo)
{
   DBG("! return %d",mo->curvol);
   return mo->curvol;
}

void mpd_status_set_volume(MpdObj* mo, int newvol)
{
   DBG("!");
   /* write setvol 'newvol' to socket */
}

int mpd_status_update(MpdObj* mo)
{
   DBG("!");
   /* write 'status' to socket */
   /* return MPD_ERROR_* if error */
   return MPD_OK;
}

mpd_Song* mpd_playlist_get_current_song(MpdObj* mo)
{
   mpd_Song* ms = g_new0(mpd_Song,1);
   
   DBG("!");
   
   /* write 'currentsong' to socket */
   /* return NULL if error */
   return ms;
}

int mpd_player_get_state(MpdObj* mo)
{
   DBG("! return %d",mo->status);
   return mo->status;
}

int mpd_player_prev(MpdObj* mo){return MPD_OK;}
int mpd_player_next(MpdObj* mo){return MPD_OK;}
int mpd_player_stop(MpdObj* mo){return MPD_OK;}
int mpd_player_pause(MpdObj* mo){return MPD_OK;}

int mpd_check_error(MpdObj* mo)
{
   DBG("! return %d",mo->error);
   return mo->error;
}

void mpd_send_password(MpdObj* mo){}
void mpd_set_hostname(MpdObj* mo, char* host)
{
   DBG("! new hostname=%s",host);
   g_free(mo->host);
   mo->host = g_strndup(host,STRLENGTH);
}

void mpd_set_password(MpdObj* mo, char* pass)
{
   DBG("! new password=%s",pass);
   g_free(mo->pass);
   mo->pass = g_strndup(pass,STRLENGTH);
}

void mpd_set_port(MpdObj* mo,int port)
{
   DBG("! new port=%d",port);
   mo->port = port;
}
#endif /* !HAVE_LIBMPD */ 

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
   DBG ("Settings : %s@%s:%d\nframe:%d", mpc->mpd_password, mpc->mpd_host, mpc->mpd_port, mpc->show_frame);
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

   t_mpc *mpc = dialog->mpc;
   mpc->mpd_host = g_strndup(gtk_entry_get_text(GTK_ENTRY(dialog->textbox_host)),STRLENGTH);
   mpc->mpd_port = atoi(gtk_entry_get_text(GTK_ENTRY(dialog->textbox_port)));
   mpc->mpd_password = g_strndup(gtk_entry_get_text(GTK_ENTRY(dialog->textbox_password)),STRLENGTH);
   
   DBG ("Apply: host=%s, port=%d, passwd=%s", mpc->mpd_host, mpc->mpd_port, mpc->mpd_password);

   mpd_disconnect(mpc->mo);
   mpd_set_hostname(mpc->mo,mpc->mpd_host);
   mpd_set_port(mpc->mo,mpc->mpd_port);
   mpd_set_password(mpc->mo,mpc->mpd_password);
   mpd_connect(mpc->mo);
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
   gtk_entry_set_text(GTK_ENTRY(dialog->textbox_password),mpc->mpd_password);
   gtk_table_attach_defaults(GTK_TABLE(table),dialog->textbox_password,1,2,2,3);
    
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

   switch (mpd_player_get_state(mpc->mo))
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
   song = mpd_playlist_get_current_song(mpc->mo);
   if (song) 
      g_sprintf(str,"%s - [%s - %s] -/- (#%s) %s", str, song->artist, song->album, song->track, song->title);
   else
      g_sprintf(str,"%s - Failed to get song info ?", str);
   gtk_tooltips_set_tip (mpc->tips, widget, str, NULL);
}

static void 
toggle(GtkWidget *widget, GdkEventButton* event, t_mpc* mpc)
{
   DBG("!");

   if (event->button != 1) 
      return;
   
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

static void 
prev(GtkWidget *widget, GdkEventButton* event, t_mpc* mpc) 
{
   if (event->button != 1) 
      return;
   
   if (mpd_player_prev(mpc->mo) != MPD_OK)
   {
      if (mpc_plugin_reconnect (mpc))
      {
         DBG("calling mpd_player_prev() after reconnect");
	 mpd_player_prev(mpc->mo);
      }
   }
   else
      DBG("mpd_player_prev() ok");
}

static void 
stop(GtkWidget *widget, GdkEventButton* event, t_mpc* mpc) 
{
   if (event->button != 1) 
      return;
   
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

static void 
next(GtkWidget *widget, GdkEventButton* event, t_mpc* mpc) 
{
   if (event->button != 1) 
      return;
   
   if (mpd_player_next(mpc->mo) != MPD_OK)
   {
      if (mpc_plugin_reconnect (mpc))
      {
         DBG("calling mpd_player_next() after reconnect");
	 mpd_player_next(mpc->mo);
      }
   }
   else
      DBG("mpd_player_next() ok");
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

   gtk_widget_show_all (mpc->box);
    
   return mpc;
}

static void
mpc_construct (XfcePanelPlugin * plugin)
{
   t_mpc *mpc;

   DBG("!");
#if DEBUG
/*   setvbuf(stdout, NULL, _IONBF, 0); */
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
   mpc->show_frame = TRUE;
   /* mpc->stay_connected = TRUE; */

   mpc_read_config (plugin, mpc);
   
   /* create a connection*/
   mpc->mo = mpd_new(mpc->mpd_host,mpc->mpd_port,mpc->mpd_password);
/* no need to connect now, as we show configure dialog first then reconnect
   mpd_connect(mpc->mo);
   if (mpc->mpd_password)
      mpd_send_password(mpc->mo);
*/   
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


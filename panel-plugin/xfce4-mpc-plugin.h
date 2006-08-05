/* xfce4-mpc-plugin.h
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
 
#include <gtk/gtk.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-hvbox.h>
#include <libxfce4panel/xfce-panel-convenience.h>

#ifdef HAVE_LIBMPD
#include <libmpd/libmpd.h>
#if DEBUG
#include <libmpd/debug_printf.h>
#endif
#else

#define MPD_PLAYER_STOP 1
#define MPD_PLAYER_PLAY 2
#define MPD_PLAYER_PAUSE 3
#define MPD_OK 0
#define MPD_WELCOME_MESSAGE "OK MPD "
#define MAXBUFLEN 1000

#define MPD_ERROR_NOSOCK  9
#define MPD_ERROR_TIMEOUT  10 /* timeout trying to talk to mpd */
#define MPD_ERROR_SYSTEM   11 /* system error */
#define MPD_ERROR_UNKHOST  12 /* unknown host */
#define MPD_ERROR_CONNPORT 13 /* problems connecting to port on host */
#define MPD_ERROR_NOTMPD   14 /* mpd not running on port at host */
#define MPD_ERROR_NORESPONSE  15 /* no response on attempting to connect */
typedef struct {
   gchar* host;
   int port;
   gchar* pass;
   int socket;
   int status;
   int curvol;
   int error;
   char buffer[MAXBUFLEN+1];
   int buflen;
} MpdObj;

typedef struct {
   char* artist;
   char* album;
   char* track;
   char* title;
} mpd_Song;

MpdObj* mpd_new(char*,int,char*);
void mpd_free(MpdObj*);
void mpd_connect(MpdObj*);
void mpd_disconnect(MpdObj*);
int mpd_status_get_volume(MpdObj*);
void mpd_status_set_volume(MpdObj*,int);
int mpd_status_update(MpdObj*);
int mpd_player_get_state(MpdObj*);
int mpd_player_prev(MpdObj*);
int mpd_player_next(MpdObj*);
int mpd_player_stop(MpdObj*);
int mpd_player_pause(MpdObj*);
mpd_Song* mpd_playlist_get_current_song(MpdObj*);
int mpd_check_error(MpdObj*);
void mpd_set_hostname(MpdObj*,char*);
void mpd_set_password(MpdObj*,char*);
void mpd_send_password(MpdObj*);
void mpd_set_port(MpdObj*,int);

#endif /* !HAVE_LIBMPD */

typedef struct {
   XfcePanelPlugin *plugin;
   GtkTooltips *tips;
   GtkWidget *frame,*ebox,*box,*prev,*stop,*toggle,*next;
   /* mpd handle */
   MpdObj *mo;
   gchar* mpd_host;
   gint mpd_port;
   gchar * mpd_password;
   gboolean show_frame;
} t_mpc;

typedef struct {
   t_mpc *mpc;
   GtkWidget *textbox_host;
   GtkWidget *textbox_port;
   GtkWidget *textbox_password;
   GtkWidget *checkbox_frame;
} t_mpc_dialog;


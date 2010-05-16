/* simple-libmpd.h
 *
 * Copyright (c) 2006-2010 Landry Breuil (landry at fr.homeunix.org / gaston at gcu.info)
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

/* for gchar and g_new conveniences */
#include <gtk/gtk.h>

#define MPD_PLAYER_STOP 1
#define MPD_PLAYER_PLAY 2
#define MPD_PLAYER_PAUSE 3
#define MPD_OK 0
#define MPD_FAILED 1
#define MPD_NOTOK 2
#define MPD_WELCOME_MESSAGE "OK MPD "
#define MAXBUFLEN 1000

#define MPD_ERROR_NOSOCK      9
#define MPD_ERROR_TIMEOUT     10 /* timeout trying to talk to mpd */
#define MPD_ERROR_SYSTEM      11 /* system error */
#define MPD_ERROR_UNKHOST     12 /* unknown host */
#define MPD_ERROR_CONNPORT    13 /* problems connecting to port on host */
#define MPD_ERROR_NOTMPD      14 /* mpd not running on port at host */
#define MPD_ERROR_NORESPONSE  15 /* no response on attempting to connect */
#define MPD_ERROR_SENDING     16 /* error sending command */
#define MPD_ERROR_CONNCLOSED  17 /* connection closed by mpd */


typedef struct {
   char* file;
   char* artist;
   char* album;
   char* track;
   char* title;
   int pos;
   int id;
} mpd_Song;

typedef struct {
   int id;
   char* name;
   int enabled;
} mpd_Output;

typedef struct {
   gchar* host;
   int port;
   gchar* pass;
   int socket;
   int status;
   int curvol;
   int song;
   int songid;
   int repeat;
   int random;
   int playlistlength;
   mpd_Song* cursong;
   int error;
   char buffer[MAXBUFLEN*2];
   int buflen;
} MpdObj;

typedef enum {
   MPD_DATA_TYPE_SONG,
   MPD_DATA_TYPE_OUTPUT_DEV
} MpdDataType;

/* here, i must cheat, too hard to follow libmpd's behaviour */
typedef struct {
   /* holds type = song or output */
   MpdDataType type;
   /* ptr to current song */
   mpd_Song* song;
   /* vector of all songs in playlist */
   mpd_Song* allsongs;
   /* ptr to current output */
   mpd_Output* output_dev;
   /* vector of all outputs */
   mpd_Output** alloutputs;
   int nb;
   int cur;
} MpdData;

MpdObj* mpd_new(char*, int, char*);
void mpd_free(MpdObj*);
void mpd_connect(MpdObj*);
void mpd_disconnect(MpdObj*);
int mpd_status_get_volume(MpdObj*);
void mpd_status_set_volume(MpdObj*, int);
int mpd_status_update(MpdObj*);
int mpd_player_get_state(MpdObj*);
int mpd_player_prev(MpdObj*);
int mpd_player_next(MpdObj*);
int mpd_player_stop(MpdObj*);
int mpd_player_pause(MpdObj*);
int mpd_player_play(MpdObj*);
int mpd_player_play_id(MpdObj*, int);
int mpd_player_get_current_song_pos(MpdObj*);
MpdData* mpd_playlist_get_changes(MpdObj*, int);
MpdData* mpd_data_get_next(MpdData*);
mpd_Song* mpd_playlist_get_current_song(MpdObj*);
MpdData* mpd_server_get_output_devices(MpdObj*);
int mpd_server_set_output_device (MpdObj*, int, int);
int mpd_playlist_get_playlist_length(MpdObj*);
int mpd_check_error(MpdObj*);
void mpd_set_hostname(MpdObj*, char*);
void mpd_set_password(MpdObj*, char*);
void mpd_send_password(MpdObj*);
void mpd_set_port(MpdObj*, int);
int mpd_player_set_random(MpdObj*,int);
int mpd_player_get_random(MpdObj*);
int mpd_player_set_repeat(MpdObj*,int);
int mpd_player_get_repeat(MpdObj*);

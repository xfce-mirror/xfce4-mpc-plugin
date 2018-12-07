/* simple-libmpd.c
 *
 * Copyright (c) 2006-2012 Landry Breuil <landry at xfce.org>
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

#include "simple-libmpd.h"

/* for DBG(..) macros */
#include <libxfce4util/libxfce4util.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
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

   mo->host = g_strdup(host);
   mo->port = port;
   mo->pass = g_strdup(pass);
   mo->socket = 0;
   mo->status = 0;
   mo->repeat = 0;
   mo->random = 0;
   mo->curvol = 0;
   mo->song = 0;
   mo->songid = 0;
   mo->playlistlength = 0;
   mo->cursong = NULL;
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
   int err,nbread;
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

   if (connect(mo->socket,remote_sa, sizeof(struct sockaddr_in)) < 0)
   {
      mo->error = MPD_ERROR_CONNPORT;
      DBG("ERROR @connect(), err=%s",strerror(errno));
      return;
   }

   tv.tv_sec = 1;
   tv.tv_usec = 0;
   FD_ZERO(&fds);
   FD_SET(mo->socket,&fds);
   if((err = select(mo->socket+1,&fds,NULL,NULL,&tv)) == 1)
   {
      if ((nbread = recv(mo->socket, mo->buffer, MAXBUFLEN, 0)) < 0)
      {
         mo->error = MPD_ERROR_NORESPONSE;
         DBG("ERROR @recv(), err=%s",strerror(errno));
         return;
      }
      if (nbread == 0)
      {
          mo->error = MPD_ERROR_CONNCLOSED;
          DBG("ERROR @recv(), connection closed by server");
          return;
      }
      mo->buflen = nbread;
      mo->buffer[mo->buflen] = '\0';
   }
   else if(err < 0)
   {
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

   if (strncmp(mo->buffer,MPD_WELCOME_MESSAGE, strlen(MPD_WELCOME_MESSAGE)))
   {
      mo->error = MPD_ERROR_NOTMPD;
      DBG("ERROR @strncmp() -> answer didn't come from mpd");
      return;
   }

   DBG("Received %d bytes = welcome message :\"%s\"", mo->buflen, mo->buffer);

   *mo->buffer = '\0';
   mo->buflen = 0;
   mo->error = 0;
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

int mpd_wait_for_answer(MpdObj *mo)
{
   struct timeval tv;
   int err,nbread;
   fd_set fds;
   err = nbread = 0;

   DBG("!");

   tv.tv_sec = 1;
   tv.tv_usec = 0;
   FD_ZERO(&fds);
   FD_SET(mo->socket,&fds);
   if((err = select(mo->socket+1,&fds,NULL,NULL,&tv)) == 1)
   {
      if ((nbread = recv(mo->socket, mo->buffer, MAXBUFLEN, 0)) < 0)
      {
         mo->error = MPD_ERROR_NORESPONSE;
         DBG("ERROR @recv(), err=%s",strerror(errno));
         return -1;
      }
      if (nbread == 0)
      {
         mo->error = MPD_ERROR_CONNCLOSED;
         DBG("ERROR @recv(), connection closed by server");
         return -1;
      }

      mo->buflen = nbread;
      mo->buffer[mo->buflen] = '\0';
      DBG("Read %d bytes, buff=\"%s\"", nbread, mo->buffer);
   }
   else if(err < 0)
   {
      mo->error = MPD_ERROR_CONNPORT;
      DBG("ERROR @select(), err=%s",strerror(errno));
      return -1;
   }
   else
   {
      mo->error = MPD_ERROR_NORESPONSE;
      DBG("ERROR @select(), timeoute'ed -> err=%s",strerror(errno));
      return -1;
   }
   if (!strncmp(mo->buffer,"ACK",3))
      mo->error = MPD_NOTOK;
   else
      mo->error = MPD_OK;
   return nbread;
}

int mpd_send_single_cmd(MpdObj*mo, char* cmd)
{
   int nbwri = 0;

   if (mo->socket)
   {
      DBG("Sending \"%s\"",cmd);
      if ((nbwri = send(mo->socket, cmd, strlen(cmd), 0)) <= 0)
      {
         mo->error = MPD_ERROR_SENDING;
         DBG("ERROR @send(), err=%s",strerror(errno));
      }
      DBG("Sent %d bytes",nbwri);
      /* wait for OK */
      mpd_wait_for_answer(mo);

      if(!mo->error)
      {
         if (strcmp(mo->buffer,"OK\n") != 0)
         {
            mo->error = MPD_FAILED;
            DBG("ERROR : did not received OK");
         }
      }
      *mo->buffer = '\0';
      mo->buflen = 0;
   }
   else
   {
      mo->error = MPD_ERROR_NOSOCK;
      DBG("ERROR : socket == NULL ?");
   }
   return ((!mo->error) ? MPD_OK : MPD_FAILED);
}

void send_complex_cmd(MpdObj* mo, char* cmd, void (*parse_answer_fct)(), void *res)
{
   int nbwri, nbread, tmp_bufsize;
   char *ptr;
   char *tmp_buffer, *tmp;
   /* write 'cmd' to socket */
   if (mo->socket)
   {
      DBG("Sending \"%s\"",cmd);
      if ((nbwri = send(mo->socket, cmd, strlen(cmd), 0)) < 0)
      {
         mo->error = MPD_ERROR_SENDING;
         DBG("ERROR @send(), err=%s",strerror(errno));
         return;
      }
      DBG("Sent %d bytes",nbwri);

      nbread = mpd_wait_for_answer(mo);
      /* special case for long answers with 'playlistinfo' - hack to loop until we have received the final OK\n*/
      while (!mo->error && ( nbread == MAXBUFLEN || 0 != strcmp(mo->buffer + strlen(mo->buffer) - 3,"OK\n")))
      {
         /* save the end of the buffer from last occurence of 'file:', and replace it with 'OK\n' */
         ptr = g_strrstr(mo->buffer, "file:");
         if (!ptr)
         {
            mo->error = MPD_ERROR_CONNCLOSED;
            DBG("ERROR parsing reply");
            *mo->buffer = '\0';
            mo->buflen = 0;
            return;
         }

         tmp_buffer = (char*) calloc (MAXBUFLEN*2, sizeof(char));
         strcpy(tmp_buffer, ptr);
         tmp_bufsize = strlen(tmp_buffer);
         strcpy(ptr, "OK\n");
         DBG("tmp_buffer contains \"%s\"", tmp_buffer);
         DBG("buffer now contains \"%s\"", mo->buffer);
         /* parse buffer */
         (*parse_answer_fct)(mo, res);

         /* re-read stuff */
         nbread = mpd_wait_for_answer(mo);
         /* append the saved end to the beginning of buffer */
         /* ugly memory management. really. */
         tmp = (char*) calloc (MAXBUFLEN*2, sizeof(char));
         strcpy(tmp, mo->buffer);
         strcpy(mo->buffer, tmp_buffer);
         strcpy(mo->buffer + tmp_bufsize, tmp);
         mo->buffer[tmp_bufsize + nbread] = '\0';
         DBG("added tmp_buffer at the beginning of buffer, now contains \"%s\"", mo->buffer);
         free(tmp_buffer);
         free(tmp);
      }
      /* finally parse the end of the answer => 'currentsong' and 'status' parsing should jump directly here*/
      if (!mo->error)
         (*parse_answer_fct)(mo, res);

      *mo->buffer = '\0';
      mo->buflen = 0;
   }
   else
   {
      mo->error = MPD_ERROR_NOSOCK;
      DBG("ERROR : socket == NULL ?");
   }
}

void parse_status_answer(MpdObj *mo, void* unused_param)
{
   gchar **lines, **tokens;
   int i;
   mo->songid = -1;
   lines = g_strsplit(mo->buffer, "\n", 0);
   for (i = 0 ; lines[i] && strncmp(lines[i], "OK", 2) ; i++)
   {
      tokens = g_strsplit(lines[i], ":", 2);
      /* remove leading whitespace */
      tokens[1] = g_strchug(tokens[1]);
      DBG("key=\"%s\",value=\"%s\"", tokens[0], tokens[1]);
      if      (0 == strcmp("volume",tokens[0])) mo->curvol = atoi(tokens[1]);
      else if (0 == strcmp("repeat",tokens[0])) mo->repeat = atoi(tokens[1]);
      else if (0 == strcmp("random",tokens[0])) mo->random = atoi(tokens[1]);
      else if (0 == strcmp("playlistlength",tokens[0])) mo->playlistlength = atoi(tokens[1]);
      else if (0 == strcmp("state", tokens[0]))
      {
         if      (0 == strcmp("play", tokens[1])) mo->status = MPD_PLAYER_PLAY;
         else if (0 == strcmp("pause",tokens[1])) mo->status = MPD_PLAYER_PAUSE;
         else if (0 == strcmp("stop", tokens[1])) mo->status = MPD_PLAYER_STOP;
      }
      else if (0 == strcmp("song",tokens[0])) mo->song = atoi(tokens[1]);
      else if (0 == strcmp("songid",tokens[0])) mo->songid = atoi(tokens[1]);
      g_strfreev(tokens);
   }
   g_strfreev(lines);
}

int mpd_status_update(MpdObj* mo)
{
   mo->status = 0;
   DBG("!");
   send_complex_cmd(mo, "status\n", parse_status_answer, NULL);
   return ((!mo->error) ? MPD_OK : MPD_FAILED);
}


void parse_one_song(MpdObj *mo, void* param)
{
   mpd_Song* ms = (mpd_Song*) param;
   gchar **lines, **tokens;
   int i;
   ms->file = ms->artist = ms->album = ms->title = ms->track = NULL;
   ms->id = ms->pos = -1;

   lines = g_strsplit(mo->buffer, "\n", 0);
   for (i = 0 ; lines[i] && strcmp(lines[i], "OK") ; i++)
   {
      tokens = g_strsplit(lines[i], ":", 2);
      /* remove leading whitespace */
      tokens[1] = g_strchug(tokens[1]);
      DBG("key=\"%s\",value=\"%s\"", tokens[0], tokens[1]);
      if      (!ms->file   && 0 == strcmp("file",  tokens[0])) ms->file  = g_strdup(tokens[1]);
      else if (!ms->artist && 0 == strcmp("Artist",tokens[0])) ms->artist= g_strdup(tokens[1]);
      else if (!ms->album  && 0 == strcmp("Album", tokens[0])) ms->album = g_strdup(tokens[1]);
      else if (!ms->title  && 0 == strcmp("Title", tokens[0])) ms->title = g_strdup(tokens[1]);
      else if (!ms->track  && 0 == strcmp("Track", tokens[0])) ms->track = g_strdup(tokens[1]);
      else if (ms->pos < 0 && 0 == strcmp("Pos",   tokens[0])) ms->pos   = atoi(tokens[1]);
      else if (ms->id < 0  && 0 == strcmp("Id",    tokens[0])) ms->id    = atoi(tokens[1]);
      g_strfreev(tokens);
   }
   if (ms->id < 0)
      mo->error = MPD_FAILED;
   g_strfreev(lines);
}

void parse_playlistinfo_answer(MpdObj *mo, void *param)
{
   MpdData* md = (MpdData*) param;
   mpd_Song* ms;
   gchar **lines, **tokens;
   int i = 0;

   lines = g_strsplit(mo->buffer, "\n", 0);
   while(lines[i] && strcmp(lines[i],"OK"))
   {
      ms = &md->allsongs[md->nb];
      ms->file = ms->artist = ms->album = ms->title = ms->track = NULL;
      ms->id = ms->pos = -1;
      DBG("Going to parse song #%d", md->nb);

      while(lines[i] && strcmp(lines[i],"OK") && ms->id < 0)
      {
         tokens = g_strsplit(lines[i], ":", 2);
         /* remove leading whitespace */
         tokens[1] = g_strchug(tokens[1]);
         DBG("key=\"%s\",value=\"%s\"", tokens[0], tokens[1]);
         if      (!ms->file   && 0 == strcmp("file",  tokens[0])) ms->file  = g_strdup(tokens[1]);
         else if (!ms->artist && 0 == strcmp("Artist",tokens[0])) ms->artist= g_strdup(tokens[1]);
         else if (!ms->album  && 0 == strcmp("Album", tokens[0])) ms->album = g_strdup(tokens[1]);
         else if (!ms->title  && 0 == strcmp("Title", tokens[0])) ms->title = g_strdup(tokens[1]);
         else if (!ms->track  && 0 == strcmp("Track", tokens[0])) ms->track = g_strdup(tokens[1]);
         else if (ms->pos < 0 && 0 == strcmp("Pos",   tokens[0])) ms->pos   = atoi(tokens[1]);
         else if (ms->id < 0  && 0 == strcmp("Id",    tokens[0])) ms->id    = atoi(tokens[1]);
         i++;
         g_strfreev(tokens);
      }
      md->nb++;
   }
   g_strfreev(lines);
   DBG("Got 'OK', md->nb = %d", md->nb);
}

void parse_outputs_answer(MpdObj *mo, void *param)
{
   MpdData* md = (MpdData*) param;
   gchar **lines, **tokens;
   int i = 0;
   lines = g_strsplit(mo->buffer, "\n", 0);
   while(lines[i] && strcmp(lines[i],"OK"))
   {
      md->alloutputs[md->nb] = g_new(mpd_Output, 1);
      md->alloutputs[md->nb]->enabled = -1;
      DBG("Going to parse output #%d", md->nb);
      while(lines[i] && strcmp(lines[i],"OK") && md->alloutputs[md->nb]->enabled < 0)
      {
         tokens = g_strsplit(lines[i], ":", 2);
         /* remove leading whitespace */
         tokens[1] = g_strchug(tokens[1]);
         DBG("key=\"%s\",value=\"%s\"", tokens[0], tokens[1]);
         if      (0 == strcmp("outputid",tokens[0])) md->alloutputs[md->nb]->id = atoi(tokens[1]);
         else if (0 == strcmp("outputname",tokens[0])) md->alloutputs[md->nb]->name = g_strdup(tokens[1]);
         else if (0 == strcmp("outputenabled",tokens[0])) md->alloutputs[md->nb]->enabled = atoi(tokens[1]);
         i++;
         g_strfreev(tokens);
      }
      md->nb++;
   }
   g_strfreev(lines);
}

mpd_Song* mpd_playlist_get_current_song(MpdObj* mo)
{
   DBG("!");
   if (mo->cursong != NULL && mo->cursong->id != mo->songid)
   {
      if (mo->cursong->file)   free(mo->cursong->file);
      if (mo->cursong->artist) free(mo->cursong->artist);
      if (mo->cursong->album)  free(mo->cursong->album);
      if (mo->cursong->title)  free(mo->cursong->title);
      if (mo->cursong->track)  free(mo->cursong->track);
      free(mo->cursong);
      mo->cursong = NULL;
   }
   if (mo->cursong == NULL)
   {
      mo->cursong = g_new0(mpd_Song,1);
      DBG("updating currentsong");
      send_complex_cmd(mo, "currentsong\n", parse_one_song, (void*) mo->cursong);
   }
   return ((!mo->error) ? mo->cursong : NULL);
}

int mpd_playlist_get_playlist_length(MpdObj* mo)
{
   return mo->playlistlength;
}

int mpd_player_get_current_song_pos(MpdObj* mo)
{
   DBG("!");
   return ((mpd_status_update(mo) != MPD_OK) ? -1 : mo->song);
}

MpdData* mpd_playlist_get_changes(MpdObj* mo, int old_playlist_id)
{
   MpdData* md = g_new0(MpdData,1);
   md->cur = md->nb = 0;
   md->type = MPD_DATA_TYPE_SONG;
   md->allsongs = g_new(mpd_Song, mo->playlistlength);
   DBG("!");
   send_complex_cmd(mo, "playlistinfo\n", parse_playlistinfo_answer, (void*) md);
   md->song = &(md->allsongs[0]);
   return ((!mo->error) ? md : NULL);
}

MpdData* mpd_data_get_next(MpdData* md)
{
   md->cur++;
   /* free(md) and return NULL if after last */
   if (md->cur == md->nb)
   {
      for (md->cur--; md->cur; md->cur--)
      {
         switch (md->type) {
            case MPD_DATA_TYPE_OUTPUT_DEV:
            if (md->alloutputs[md->cur]->name) free(md->alloutputs[md->cur]->name);
            break;
            case MPD_DATA_TYPE_SONG:
            if (md->allsongs[md->cur].file) free(md->allsongs[md->cur].file);
            if (md->allsongs[md->cur].artist) free(md->allsongs[md->cur].artist);
            if (md->allsongs[md->cur].album) free(md->allsongs[md->cur].album);
            if (md->allsongs[md->cur].title) free(md->allsongs[md->cur].title);
            if (md->allsongs[md->cur].track) free(md->allsongs[md->cur].track);
            break;
         }
      }
      switch (md->type) {
         case MPD_DATA_TYPE_OUTPUT_DEV:
            g_free(md->alloutputs);
            break;
         case MPD_DATA_TYPE_SONG:
            g_free(md->allsongs);
            break;
      }
      g_free(md);
      DBG("Free()'d md");
      return NULL;
   }
   switch (md->type) {
      case MPD_DATA_TYPE_OUTPUT_DEV:
         md->output_dev = md->alloutputs[md->cur];
         break;
      case MPD_DATA_TYPE_SONG:
         md->song = (&md->allsongs[md->cur]);
         break;
   }
   return md;
}

MpdData* mpd_server_get_output_devices(MpdObj* mo)
{
   MpdData* md = g_new0(MpdData,1);
   DBG("!");
   md->cur = md->nb = 0;
   md->alloutputs = g_new(mpd_Output*,1);
   md->type = MPD_DATA_TYPE_OUTPUT_DEV;
   send_complex_cmd(mo, "outputs\n", parse_outputs_answer, (void*) md);
   md->output_dev = md->alloutputs[0];
   return ((!mo->error) ? md : NULL);
}

int mpd_server_set_output_device (MpdObj* mo, int id, int state)
{
   char outbuf[18];
   /* write (enable|disable)output 'id' to socket */
   DBG("!");
   snprintf(outbuf, sizeof(outbuf), "%soutput %d\n",(state ? "enable" : "disable"), id);
   return mpd_send_single_cmd(mo,outbuf);
}

void mpd_status_set_volume(MpdObj* mo, int newvol)
{
   char outbuf[15];
   /* write setvol 'newvol' to socket */
   DBG("!");
   snprintf(outbuf, sizeof(outbuf), "setvol %d\n",newvol);
   mpd_send_single_cmd(mo,outbuf);
}

int mpd_player_get_state(MpdObj* mo)
{
   DBG("! return %d",mo->status);
   return mo->status;
}

int mpd_player_get_random(MpdObj* mo)
{
   DBG("! return %d",mo->random);
   return mo->random;
}

int mpd_player_set_random(MpdObj* mo, int random)
{
   char outbuf[15];
   DBG("!");
   snprintf(outbuf, sizeof(outbuf), "random %d\n",random);
   return mpd_send_single_cmd(mo,outbuf);

}

int mpd_player_set_repeat(MpdObj* mo, int repeat)
{
   char outbuf[15];
   DBG("!");
   snprintf(outbuf, sizeof(outbuf), "repeat %d\n",repeat);
   return mpd_send_single_cmd(mo,outbuf);
}

int mpd_player_get_repeat(MpdObj* mo)
{
   DBG("! return %d",mo->repeat);
   return mo->repeat;
}

int mpd_player_prev(MpdObj* mo)
{
   DBG("!");
   return mpd_send_single_cmd(mo,"previous\n");
}

int mpd_player_next(MpdObj* mo)
{
   DBG("!");
   return mpd_send_single_cmd(mo,"next\n");
}

int mpd_player_stop(MpdObj* mo)
{
   DBG("!");
   return mpd_send_single_cmd(mo,"stop\n");
}

int mpd_player_pause(MpdObj* mo)
{
   DBG("!");
   if (mo->status != MPD_PLAYER_PLAY)
      return mpd_send_single_cmd(mo,"pause 0\n");
   else
      return mpd_send_single_cmd(mo,"pause 1\n");
}

int mpd_player_play(MpdObj* mo)
{
   DBG("!");
   return mpd_send_single_cmd(mo,"play\n");
}

int mpd_player_play_id(MpdObj* mo, int id)
{
   char outbuf[15];
   DBG("!");
   snprintf(outbuf, sizeof(outbuf), "playid %d\n",id);
   return mpd_send_single_cmd(mo,outbuf);
}

int mpd_check_error(MpdObj* mo)
{
   DBG("! return %d",mo->error);
   return mo->error;
}

void mpd_send_password(MpdObj* mo)
{
   char outbuf[256];
   int wrote;
   DBG("!");
   /* write password 'password' to socket */
   wrote = snprintf(outbuf, sizeof(outbuf), "password %s\n",mo->pass);
   if (wrote > 255) {
	/* the password is too long to fit though there doesn't seem to be a
	 * nice way to report this error :-/ */
	fprintf(stderr, "xfce4-mpc-plugin: password too long!\n");
	mo->error = MPD_ERROR_SYSTEM;
	return;
   }
   mpd_send_single_cmd(mo,outbuf);
}

void mpd_set_hostname(MpdObj* mo, char* host)
{
   DBG("! new hostname=%s",host);
   g_free(mo->host);
   mo->host = g_strdup(host);
}

void mpd_set_password(MpdObj* mo, char* pass)
{
   DBG("! new password=%s",pass);
   g_free(mo->pass);
   mo->pass = g_strdup(pass);
}

void mpd_set_port(MpdObj* mo,int port)
{
   DBG("! new port=%d",port);
   mo->port = port;
}


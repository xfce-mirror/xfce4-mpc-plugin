/* simple-libmpd.c
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

#define STRLENGTH 32

MpdObj* mpd_new(char* host, int port, char* pass)
{
   MpdObj* mo = g_new0(MpdObj,1);

   DBG("host=%s, port=%d, pass=%s", host, port, pass);
   
   mo->host = g_strndup(host,STRLENGTH);
   mo->port = port;
   mo->pass = g_strndup(pass,STRLENGTH);
   mo->socket = 0;
   mo->status = 0;
   mo->repeat = 0;
   mo->random = 0;
   mo->curvol = 0;
   mo->cursong = 0;
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

void mpd_wait_for_answer(MpdObj *mo)
{
   struct timeval tv;
   int err,nbread;
   err = nbread = 0;
   fd_set fds;
   
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
         return;
      }
      if (nbread == 0)
      {   
         mo->error = MPD_ERROR_CONNCLOSED;
         DBG("ERROR @recv(), connection closed by server");
         return;
      }
 
      DBG("Read %d bytes, buff=\"%s\"", nbread, mo->buffer);
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
   if (!strncmp(mo->buffer,"ACK",3))
      mo->error = MPD_NOTOK; 
   else
      mo->error = MPD_OK;
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

void* send_complex_cmd(MpdObj* mo, char* cmd, void* (*parse_answer_fct)())
{
   int nbwri;
   void *res = NULL;
   /* write 'cmd' to socket */
   if (mo->socket) 
   {
      DBG("Sending \"%s\"",cmd);
      if ((nbwri = send(mo->socket, cmd, strlen(cmd), 0)) < 0)
      {
         mo->error = MPD_ERROR_SENDING;
         DBG("ERROR @send(), err=%s",strerror(errno));
         return NULL;
      }
      DBG("Sent %d bytes",nbwri);
      
      mpd_wait_for_answer(mo);
      
      if (!mo->error)
         res = (*parse_answer_fct)(mo);

      *mo->buffer = '\0';
      mo->buflen = 0;
   }
   else
   {
      mo->error = MPD_ERROR_NOSOCK;
      DBG("ERROR : socket == NULL ?");
   }
   /* return NULL if error */
   return res;
}

void * parse_status_answer(MpdObj *mo)
{
   char *eol,*ptr;
   char key[15], value[200];

   while (strcmp(mo->buffer,"OK\n"))
   {
      /*HACK @#!*/
      ptr = strstr(mo->buffer, ":");
      eol = strstr(mo->buffer, "\n");
      strncpy(key, mo->buffer, ptr - mo->buffer);
      key[ptr - mo->buffer]='\0';
      strncpy(value,ptr + 2 , eol - ptr - 2);
      value[eol - ptr - 2]='\0';
 
      DBG("key=\"%s\",value=\"%s\"", key, value);
      if      (0 == strcmp("volume",key)) mo->curvol = atoi(value);
      else if (0 == strcmp("repeat",key)) mo->repeat = atoi(value);
      else if (0 == strcmp("random",key)) mo->random = atoi(value);
      else if (0 == strcmp("song",key)) mo->cursong = atoi(value);
      else if (0 == strcmp("state", key)) 
      {
         if      (0 == strcmp("play", value)) mo->status = MPD_PLAYER_PLAY;
         else if (0 == strcmp("pause",value)) mo->status = MPD_PLAYER_PAUSE;
         else if (0 == strcmp("stop", value)) mo->status = MPD_PLAYER_STOP;
      }
      *eol = '\0';
      strcpy(mo->buffer, eol+1);
      mo->buflen = strlen(mo->buffer);
   }
   return NULL;
}

int mpd_status_update(MpdObj* mo)
{
   mo->status = 0;
   DBG("!");
   send_complex_cmd(mo, "status\n", parse_status_answer);
   return ((!mo->error) ? MPD_OK : MPD_FAILED);
}
   

void* parse_currentsong_answer(MpdObj *mo)
{
   mpd_Song* ms = g_new0(mpd_Song,1);
   char *eol,*ptr;
   char key[15], value[200];

   ms->artist= ms->album = ms->title = ms->track = NULL;
   while (strcmp(mo->buffer,"OK\n"))
   {
      /*HACK @#!*/
      ptr = strstr(mo->buffer, ":");
      eol = strstr(mo->buffer, "\n");
      strncpy(key, mo->buffer, ptr - mo->buffer);
      key[ptr - mo->buffer]='\0';
      strncpy(value,ptr + 2 , eol - ptr - 2);
      value[eol - ptr - 2]='\0';

      DBG("key=\"%s\",value=\"%s\"", key, value);
      if      (!ms->artist && 0 == strcmp("Artist",key)) ms->artist= strdup(value);
      else if (!ms->album  && 0 == strcmp("Album", key)) ms->album = strdup(value);
      else if (!ms->title  && 0 == strcmp("Title", key)) ms->title = strdup(value);
      else if (!ms->track  && 0 == strcmp("Track", key)) ms->track = strdup(value);
      *eol = '\0';
      strcpy(mo->buffer, eol+1);
      mo->buflen = strlen(mo->buffer);
   }
   return (void*) ms;
}

void* parse_playlistinfo_answer(MpdObj *mo)
{
   MpdData* md = (MpdData*) calloc(1, sizeof(MpdData));
   /* mimic parse_currentsong_answer on a large loop */
   /* TODO HERE */
   return (void*) md;
}

mpd_Song* mpd_playlist_get_current_song(MpdObj* mo)
{
   mpd_Song* ms;
   DBG("!");
   ms = (mpd_Song*) send_complex_cmd(mo, "currentsong\n", parse_currentsong_answer);
   /* return NULL if error */
   return ((!mo->error) ? ms : NULL);
}

int mpd_player_get_current_song_pos(MpdObj* mo)
{
   DBG("!");
   return ((mpd_status_update(mo) != MPD_OK) ? -1 : mo->cursong);
}

MpdData* mpd_playlist_get_changes(MpdObj* mo, int old_playlist_id)
{
   MpdData* md;
   DBG("!");
   md = (MpdData*) send_complex_cmd(mo, "playlistinfo\n", parse_playlistinfo_answer);
   /* return NULL if error */
   return ((!mo->error) ? md : NULL);
}

MpdData* mpd_data_get_next(MpdData* md)
{
   md->cur++;
   /* return NULL if after last */
   if (md->cur == md->nb)
   {
      /* free recursively(md) */
      /* TODO HERE */
      return NULL;
   }
   md->song = md->allsongs[md->cur];
   return md;
}

void mpd_status_set_volume(MpdObj* mo, int newvol)
{
   char outbuf[15];
   /* write setvol 'newvol' to socket */
   DBG("!");
   sprintf(outbuf,"setvol %d\n",newvol);
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
   sprintf(outbuf,"random %d\n",random);
   return mpd_send_single_cmd(mo,outbuf);

}

int mpd_player_set_repeat(MpdObj* mo, int repeat)
{
   char outbuf[15];
   DBG("!");
   sprintf(outbuf,"repeat %d\n",repeat);
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
   sprintf(outbuf,"play %d\n",id);
   return mpd_send_single_cmd(mo,outbuf);
}

int mpd_check_error(MpdObj* mo)
{
   DBG("! return %d",mo->error);
   return mo->error;
}

void mpd_send_password(MpdObj* mo)
{
   DBG("!");
   char outbuf[30];
   /* write password 'password' to socket */
   sprintf(outbuf,"password %s\n",mo->pass);
   mpd_send_single_cmd(mo,outbuf);
}

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


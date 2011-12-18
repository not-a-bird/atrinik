/************************************************************************
*            Atrinik, a Multiplayer Online Role Playing Game            *
*                                                                       *
*    Copyright (C) 2009-2011 Alex Tokar and Atrinik Development Team    *
*                                                                       *
* Fork from Daimonin (Massive Multiplayer Online Role Playing Game)     *
* and Crossfire (Multiplayer game for X-windows).                       *
*                                                                       *
* This program is free software; you can redistribute it and/or modify  *
* it under the terms of the GNU General Public License as published by  *
* the Free Software Foundation; either version 2 of the License, or     *
* (at your option) any later version.                                   *
*                                                                       *
* This program is distributed in the hope that it will be useful,       *
* but WITHOUT ANY WARRANTY; without even the implied warranty of        *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
* GNU General Public License for more details.                          *
*                                                                       *
* You should have received a copy of the GNU General Public License     *
* along with this program; if not, write to the Free Software           *
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.             *
*                                                                       *
* The author can be reached at admin@atrinik.org                        *
************************************************************************/

/**
 * @file
 * Socket initialization related code. */

#include <global.h>
#include "zlib.h"

/** All the server/client files. */
_srv_client_files SrvClientFiles[SRV_CLIENT_FILES];

/** Socket information. */
Socket_Info socket_info;
/** Established connections for clients not yet playing. */
socket_struct *init_sockets;

/**
 * Initializes a connection - really, it just sets up the data structure,
 * socket setup is handled elsewhere.
 *
 * Sends server version to the client.
 * @param ns Client's socket.
 * @param from Where the connection is coming from.
 * @todo Remove version sending legacy support for older clients at some
 * point. */
void init_connection(socket_struct *ns, const char *from_ip)
{
	int bufsize = 65535;
	int oldbufsize;
	socklen_t buflen = sizeof(int);
	packet_struct *packet;
	int i;

#ifdef WIN32
	u_long temp = 1;

	if (ioctlsocket(ns->fd, FIONBIO, &temp) == -1)
	{
		LOG(llevDebug, "init_connection(): Error on ioctlsocket.\n");
	}
#else
	if (fcntl(ns->fd, F_SETFL, O_NDELAY | O_NONBLOCK) == -1)
	{
		LOG(llevDebug, "init_connection(): Error on fcntl.\n");
	}
#endif

	if (getsockopt(ns->fd, SOL_SOCKET, SO_SNDBUF, (char *) &oldbufsize, &buflen) == -1)
	{
		oldbufsize = 0;
	}

	if (oldbufsize < bufsize)
	{
		if (setsockopt(ns->fd, SOL_SOCKET, SO_SNDBUF, (char *) &bufsize, sizeof(bufsize)))
		{
			LOG(llevDebug, "init_connection(): setsockopt unable to set output buf size to %d\n", bufsize);
		}
	}

	ns->login_count = 0;
	ns->keepalive = 0;
	ns->addme = 0;
	ns->faceset = 0;
	ns->sound = 0;
	ns->ext_title_flag = 1;
	ns->status = Ns_Add;
	ns->mapx = 17;
	ns->mapy = 17;
	ns->mapx_2 = 8;
	ns->mapy_2 = 8;
	ns->password_fails = 0;
	ns->is_bot = 0;

	for (i = 0; i < SRV_CLIENT_FILES; i++)
	{
		ns->requested_file[i] = 0;
	}

	ns->packet_recv = packet_new(0, 1024 * 3, 0);
	ns->packet_recv_cmd = packet_new(0, 1024 * 64, 0);

	memset(&ns->lastmap, 0, sizeof(struct Map));
	ns->packet_head = NULL;
	ns->packet_tail = NULL;
	pthread_mutex_init(&ns->packet_mutex, NULL);

	ns->host = strdup_local(from_ip);

	packet = packet_new(CLIENT_CMD_VERSION, 4, 4);
	packet_append_uint32(packet, SOCKET_VERSION);
	socket_send_packet(ns, packet);
}

/**
 * This sets up the socket and reads all the image information into
 * memory. */
void init_ericserver(void)
{
	struct sockaddr_in insock;
	struct linger linger_opt;
#ifndef WIN32
	struct protoent *protox;

#	ifdef HAVE_SYSCONF
	socket_info.max_filedescriptor = sysconf(_SC_OPEN_MAX);
#	else
#		ifdef HAVE_GETDTABLESIZE
	socket_info.max_filedescriptor = getdtablesize();
#		else
	"Unable to find usable function to get max filedescriptors";
#		endif
#	endif
#else
	WSADATA w;

	/* Used in select, ignored in winsockets */
	socket_info.max_filedescriptor = 1;
	/* This sets up all socket stuff */
	WSAStartup(0x0101, &w);
#endif

	socket_info.timeout.tv_sec = 0;
	socket_info.timeout.tv_usec = 0;
	socket_info.nconns = 0;

	LOG(llevDebug, "Initialize new client/server data\n");
	socket_info.nconns = 1;
	init_sockets = malloc(sizeof(socket_struct));
	socket_info.allocated_sockets = 1;

#ifndef WIN32
	protox = getprotobyname("tcp");

	if (protox == NULL)
	{
		LOG(llevBug, "init_ericserver: Error getting protox\n");
		return;
	}

	init_sockets[0].fd = socket(PF_INET, SOCK_STREAM, protox->p_proto);

#else
	init_sockets[0].fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif

	if (init_sockets[0].fd == -1)
	{
		LOG(llevError, "Cannot create socket: %s\n", strerror_local(errno));
	}

	insock.sin_family = AF_INET;
	insock.sin_port = htons(settings.csport);
	insock.sin_addr.s_addr = htonl(INADDR_ANY);

	linger_opt.l_onoff = 0;
	linger_opt.l_linger = 0;

	if (setsockopt(init_sockets[0].fd, SOL_SOCKET, SO_LINGER, (char *) &linger_opt, sizeof(struct linger)))
	{
		LOG(llevError, "init_ericserver(): Cannot setsockopt(SO_LINGER): %s\n", strerror_local(errno));
	}

	/* Would be nice to have an autoconf check for this.  It appears that
	 * these functions are both using the same calling syntax, just one
	 * of them needs extra values passed. */
#if !defined(_WEIRD_OS_)
	{
		int tmp = 1;

		if (setsockopt(init_sockets[0].fd, SOL_SOCKET, SO_REUSEADDR, (char *) &tmp, sizeof(tmp)))
		{
			LOG(llevDebug, "Cannot setsockopt(SO_REUSEADDR): %s\n", strerror_local(errno));
		}
	}
#else
	if (setsockopt(init_sockets[0].fd, SOL_SOCKET, SO_REUSEADDR, (char *) NULL, 0))
	{
		LOG(llevDebug, "Cannot setsockopt(SO_REUSEADDR): %s\n", strerror_local(errno));
	}
#endif

	if (bind(init_sockets[0].fd, (struct sockaddr *) &insock, sizeof(insock)) == -1)
	{
#ifndef WIN32
		close(init_sockets[0].fd);
#else
		shutdown(init_sockets[0].fd, SD_BOTH);
		closesocket(init_sockets[0].fd);
#endif
		LOG(llevError, "Cannot bind socket to port %d: %s\n", ntohs(insock.sin_port), strerror_local(errno));
	}

	if (listen(init_sockets[0].fd, 5) == -1)
	{
#ifndef WIN32
		close(init_sockets[0].fd);
#else
		shutdown(init_sockets[0].fd, SD_BOTH);
		closesocket(init_sockets[0].fd);
#endif
		LOG(llevError, "Cannot listen on socket: %s\n", strerror_local(errno));
	}

	init_sockets[0].status = Ns_Wait;
	read_client_images();
	updates_init();
	init_srv_files();
}

/**
 * Frees all the memory that ericserver allocates. */
void free_all_newserver(void)
{
	LOG(llevDebug, "Freeing all new client/server information.\n");

	free_socket_images();
	free(init_sockets);
}

/**
 * Basically, all we need to do here is free all data structures that
 * might be associated with the socket.
 *
 * It is up to the called to update the list.
 * @param ns The socket. */
void free_newsocket(socket_struct *ns)
{
#ifndef WIN32
	if (close(ns->fd))
#else
	shutdown(ns->fd, SD_BOTH);
	if (closesocket(ns->fd))
#endif
	{
#ifdef ESRV_DEBUG
		LOG(llevDebug, "Error closing socket %d\n", ns->fd);
#endif
	}

	if (ns->host)
	{
		free(ns->host);
	}

	packet_free(ns->packet_recv);
	packet_free(ns->packet_recv_cmd);

	socket_buffer_clear(ns);

	memset(ns, 0, sizeof(ns));
}

/**
 * Load server file.
 * @param fname Filename of the server file.
 * @param id ID of the server file.
 * @param cmd The data command. */
static void load_srv_file(char *fname, int id)
{
	FILE *fp;
	char *contents, *compressed;
	size_t fsize, numread;
	struct stat statbuf;

	LOG(llevDebug, "Loading %s...", fname);

	if ((fp = fopen(fname, "rb")) == NULL)
	{
		LOG(llevError, "Can't open file %s\n", fname);
	}

	fstat(fileno(fp), &statbuf);
	fsize = statbuf.st_size;
	/* Allocate a buffer to hold the whole file. */
	contents = malloc(fsize);

	if (!contents)
	{
		LOG(llevError, "load_srv_file(): Out of memory.\n");
	}

	numread = fread(contents, 1, fsize, fp);
	fclose(fp);

	/* Get a crc from the uncompressed file. */
	SrvClientFiles[id].crc = crc32(1L, (const unsigned char FAR *) contents, numread);
	/* Store uncompressed length. */
	SrvClientFiles[id].len_ucomp = numread;

	/* Calculate the upper bound of the compressed size. */
	numread = compressBound(fsize);
	/* Allocate a buffer to hold the compressed file. */
	compressed = malloc(numread);

	if (!compressed)
	{
		LOG(llevError, "load_srv_file(): Out of memory.\n");
	}

	compress2((Bytef *) compressed, (uLong *) &numread, (const unsigned char FAR *) contents, fsize, Z_BEST_COMPRESSION);
	SrvClientFiles[id].file = malloc(numread);

	if (!SrvClientFiles[id].file)
	{
		LOG(llevError, "load_srv_file(): Out of memory.\n");
	}

	memcpy(SrvClientFiles[id].file, compressed, numread);
	SrvClientFiles[id].len = numread;

	/* Free temporary buffers. */
	free(contents);
	free(compressed);

	LOG(llevDebug, " size: %"FMT64U" (%"FMT64U") (crc uncomp.: %lu)\n", (uint64) SrvClientFiles[id].len_ucomp, (uint64) numread, SrvClientFiles[id].crc);
}

/**
 * Get the lib/settings default file and create the data/client_settings
 * file from it. */
static void create_client_settings(void)
{
	char buf[MAX_BUF * 4];
	int i;
	FILE *fset_default, *fset_create;

	LOG(llevDebug, "Creating %s/client_settings...\n", settings.localdir);

	snprintf(buf, sizeof(buf), "%s/client_settings", settings.datadir);

	/* Open default */
	if ((fset_default = fopen(buf, "rb")) == NULL)
	{
		LOG(llevError, "Can not open file %s\n", buf);
	}

	/* Delete our target - we create it new now */
	snprintf(buf, sizeof(buf), "%s/client_settings", settings.localdir);
	unlink(buf);

	/* Open target client_settings */
	if ((fset_create = fopen(buf, "wb")) == NULL)
	{
		fclose(fset_default);
		LOG(llevError, "Can not open file %s\n", buf);
	}

	/* Copy default to target */
	while (fgets(buf, MAX_BUF, fset_default) != NULL)
	{
		fputs(buf, fset_create);
	}

	fclose(fset_default);

	/* Now add the level information */
	snprintf(buf, sizeof(buf), "level %d\n", MAXLEVEL);
	fputs(buf, fset_create);

	for (i = 0; i <= MAXLEVEL; i++)
	{
		snprintf(buf, sizeof(buf), "%"FMT64HEX"\n", new_levels[i]);
		fputs(buf, fset_create);
	}

	fclose(fset_create);
}

/**
 * Get the lib/server_settings default file and create the
 * data/server_settings file from it. */
static void create_server_settings(void)
{
	char buf[MAX_BUF];
	size_t i;
	FILE *fp;

	snprintf(buf, sizeof(buf), "%s/server_settings", settings.localdir);
	LOG(llevDebug, "Creating %s...\n", buf);

	fp = fopen(buf, "wb");

	if (!fp)
	{
		LOG(llevError, "Couldn't create %s.\n", buf);
	}

	/* Copy the default. */
	snprintf(buf, sizeof(buf), "%s/server_settings", settings.datadir);
	copy_file(buf, fp);

	/* Add the level information. */
	snprintf(buf, sizeof(buf), "level %d\n", MAXLEVEL);
	fputs(buf, fp);

	for (i = 0; i <= MAXLEVEL; i++)
	{
		snprintf(buf, sizeof(buf), "%"FMT64HEX"\n", new_levels[i]);
		fputs(buf, fp);
	}

	fclose(fp);
}

/**
 * Initialize animations file for the client. */
static void create_server_animations(void)
{
	char buf[MAX_BUF];
	FILE *fp, *fp2;

	snprintf(buf, sizeof(buf), "%s/anims", settings.localdir);
	LOG(llevDebug, "Creating %s...\n", buf);

	fp = fopen(buf, "wb");

	if (!fp)
	{
		LOG(llevError, "Couldn't create %s.\n", buf);
	}

	snprintf(buf, sizeof(buf), "%s/animations", settings.datadir);
	fp2 = fopen(buf, "rb");

	if (!fp2)
	{
		LOG(llevError, "Couldn't open %s.\n", buf);
	}

	while (fgets(buf, sizeof(buf), fp2))
	{
		/* Copy anything but face names. */
		if (!strncmp(buf, "anim ", 5) || !strcmp(buf, "mina\n") || !strncmp(buf, "facings ", 8))
		{
			fputs(buf, fp);
		}
		/* Transform face names into IDs. */
		else
		{
			char *end = strchr(buf, '\n');

			*end = '\0';
			fprintf(fp, "%d\n", find_face(buf, 0));
		}
	}

	fclose(fp2);
	fclose(fp);
}

/**
 * Load all the server files we can send to client.
 *
 * client_bmaps is generated from the server at startup out of the
 * Atrinik png file. */
void init_srv_files(void)
{
	char buf[MAX_BUF];

	memset(&SrvClientFiles, 0, sizeof(SrvClientFiles));

	snprintf(buf, sizeof(buf), "%s/hfiles", settings.datadir);
	load_srv_file(buf, SRV_CLIENT_HFILES);

	snprintf(buf, sizeof(buf), "%s/hfiles_v2", settings.datadir);
	load_srv_file(buf, SRV_CLIENT_HFILES_V2);

	snprintf(buf, sizeof(buf), "%s/animations", settings.datadir);
	load_srv_file(buf, SRV_CLIENT_ANIMS);

	snprintf(buf, sizeof(buf), "%s/client_bmaps", settings.localdir);
	load_srv_file(buf, SRV_CLIENT_BMAPS);

	snprintf(buf, sizeof(buf), "%s/client_skills", settings.datadir);
	load_srv_file(buf, SRV_CLIENT_SKILLS);

	snprintf(buf, sizeof(buf), "%s/client_spells", settings.datadir);
	load_srv_file(buf, SRV_CLIENT_SPELLS);

	create_client_settings();

	snprintf(buf, sizeof(buf), "%s/client_settings", settings.localdir);
	load_srv_file(buf, SRV_CLIENT_SETTINGS);

	snprintf(buf, sizeof(buf), "%s/%s", settings.localdir, UPDATES_FILE_NAME);
	load_srv_file(buf, SRV_FILE_UPDATES);

	snprintf(buf, sizeof(buf), "%s/%s", settings.localdir, SRV_FILE_SPELLS_FILENAME);
	load_srv_file(buf, SRV_FILE_SPELLS_V2);

	create_server_settings();
	snprintf(buf, sizeof(buf), "%s/server_settings", settings.localdir);
	load_srv_file(buf, SRV_SERVER_SETTINGS);
	new_chars_init();

	create_server_animations();
	snprintf(buf, sizeof(buf), "%s/anims", settings.localdir);
	load_srv_file(buf, SRV_CLIENT_ANIMS_V2);

	snprintf(buf, sizeof(buf), "%s/effects", settings.datadir);
	load_srv_file(buf, SRV_CLIENT_EFFECTS);

	snprintf(buf, sizeof(buf), "%s/%s", settings.localdir, SRV_CLIENT_SKILLS_FILENAME);
	load_srv_file(buf, SRV_CLIENT_SKILLS_V2);
}

/**
 * Free all server files previously initialized by init_srv_files(). */
void free_srv_files(void)
{
	int i;

	LOG(llevDebug, "Freeing server/client files.\n");

	for (i = 0; i < SRV_CLIENT_FILES; i++)
	{
		free(SrvClientFiles[i].file);
	}
}

/**
 * A connecting client has requested a server file.
 *
 * Note that we don't know anything about the player at this point - we
 * got an open socket, an IP, a matching version, and an usable setup
 * string from the client.
 * @param ns The client's socket.
 * @param id ID of the server file. */
void send_srv_file(socket_struct *ns, int id)
{
	packet_struct *packet;

	packet = packet_new(CLIENT_CMD_DATA, 1 + 4 + SrvClientFiles[id].len, 0);
	packet_append_uint8(packet, id);
	packet_append_uint32(packet, SrvClientFiles[id].len_ucomp);
	packet_append_data_len(packet, SrvClientFiles[id].file, SrvClientFiles[id].len);
	socket_send_packet(ns, packet);
}

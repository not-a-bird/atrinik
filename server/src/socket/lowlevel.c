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
 * Low level socket related functions. */

#include <global.h>

/**
 * Receives data from socket.
 * @param ns Socket to read from.
 * @return 1 on success, -1 on failure. */
int socket_recv(socket_struct *ns)
{
	int stat_ret;

#ifdef WIN32
	stat_ret = recv(ns->fd, ns->packet_recv->data + ns->packet_recv->len, ns->packet_recv->size - ns->packet_recv->len, 0);
#else
	do
	{
		stat_ret = read(ns->fd, ns->packet_recv->data + ns->packet_recv->len, ns->packet_recv->size - ns->packet_recv->len);
	}
	while (stat_ret == -1 && errno == EINTR);
#endif

	if (stat_ret == 0)
	{
		return -1;
	}

	if (stat_ret > 0)
	{
		ns->packet_recv->len += stat_ret;
	}
	else if (stat_ret < 0)
	{
#ifdef WIN32
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			if (WSAGetLastError() == WSAECONNRESET)
			{
				LOG(llevDebug, "Connection closed by client.\n");
			}
			else
			{
				LOG(llevDebug, "socket_recv() got error %d, returning %d.\n", WSAGetLastError(), stat_ret);
			}

			return stat_ret;
		}
#else
		if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
		{
			LOG(llevDebug, "socket_recv() got error %d: %s, returning %d.\n", errno, strerror_local(errno), stat_ret);
			return stat_ret;
		}
#endif
	}

	return 1;
}

/**
 * Enable TCP_NODELAY on the specified file descriptor (socket).
 * @param fd Socket's file descriptor. */
void socket_enable_no_delay(int fd)
{
	int tmp = 1;

	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &tmp, sizeof(tmp)))
	{
		LOG(llevDebug, "socket_enable_no_delay(): Cannot enable TCP_NODELAY: %s\n", strerror_local(errno));
	}
}

/**
 * Disable TCP_NODELAY on the specified file descriptor (socket).
 * @param fd Socket's file descriptor. */
void socket_disable_no_delay(int fd)
{
	int tmp = 0;

	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &tmp, sizeof(tmp)))
	{
		LOG(llevDebug, "socket_disable_no_delay(): Cannot disable TCP_NODELAY: %s\n", strerror_local(errno));
	}
}

static void socket_packet_enqueue(socket_struct *ns, packet_struct *packet)
{
	if (!ns->packet_head)
	{
		ns->packet_head = packet;
		packet->prev = NULL;
	}
	else
	{
		ns->packet_tail->next = packet;
		packet->prev = ns->packet_tail;
	}

	ns->packet_tail = packet;
	packet->next = NULL;
}

static void socket_packet_dequeue(socket_struct *ns, packet_struct *packet)
{
	if (!packet->prev)
	{
		ns->packet_head = packet->next;
	}
	else
	{
		packet->prev->next = packet->next;
	}

	if (!packet->next)
	{
		ns->packet_tail = packet->prev;
	}
	else
	{
		packet->next->prev = packet->prev;
	}

	packet_free(packet);
}

/**
 * Dequeue all socket buffers in the queue.
 * @param ns Socket to clear the socket buffers for. */
void socket_buffer_clear(socket_struct *ns)
{
	pthread_mutex_lock(&ns->packet_mutex);

	while (ns->packet_head)
	{
		socket_packet_dequeue(ns, ns->packet_head);
	}

	pthread_mutex_unlock(&ns->packet_mutex);
}

/**
 * Write data to socket.
 * @param ns The socket we are writing to. */
void socket_buffer_write(socket_struct *ns)
{
	int amt, max;

	pthread_mutex_lock(&ns->packet_mutex);

	while (ns->packet_head)
	{
		if (ns->packet_head->ndelay)
		{
			socket_enable_no_delay(ns->fd);
		}

		max = ns->packet_head->len - ns->packet_head->pos;
		amt = send(ns->fd, ns->packet_head->data + ns->packet_head->pos, max, MSG_DONTWAIT);

		if (ns->packet_head->ndelay)
		{
			socket_disable_no_delay(ns->fd);
		}

#ifndef WIN32
		if (!amt)
		{
			amt = max;
		}
		else
#endif
		if (amt < 0)
		{
#ifdef WIN32
			if (WSAGetLastError() != WSAEWOULDBLOCK)
			{
				LOG(llevDebug, "socket_buffer_write(): New socket write failed (%d).\n", WSAGetLastError());
#else
			if (errno != EWOULDBLOCK)
			{
				LOG(llevDebug, "socket_buffer_write(): New socket write failed (%d: %s).\n", errno, strerror_local(errno));
#endif
				ns->status = Ns_Dead;
				break;
			}
			/* EWOULDBLOCK: We can't write because socket is busy. */
			else
			{
				break;
			}
		}

		ns->packet_head->pos += amt;

		if (ns->packet_head->len - ns->packet_head->pos == 0)
		{
			socket_packet_dequeue(ns, ns->packet_head);
		}
	}

	pthread_mutex_unlock(&ns->packet_mutex);
}

void socket_send_packet(socket_struct *ns, packet_struct *packet)
{
	packet_struct *tmp;
	size_t toread;

	if (ns->status == Ns_Dead)
	{
		return;
	}

	packet_compress(packet);

	tmp = packet_new(0, 4, 0);
	tmp->ndelay = packet->ndelay;
	toread = packet->len + 1;

	if (toread > 32 * 1024 - 1)
	{
		tmp->data[0] = ((toread >> 16) & 0xff) | 0x80;
		tmp->data[1] = (toread >> 8) & 0xff;
		tmp->data[2] = (toread) & 0xff;
		tmp->len = 3;
	}
	else
	{
		tmp->data[0] = (toread >> 8) & 0xff;
		tmp->data[1] = (toread) & 0xff;
		tmp->len = 2;
	}

	packet_append_uint8(tmp, packet->type);

	pthread_mutex_lock(&ns->packet_mutex);
	socket_packet_enqueue(ns, tmp);
	socket_packet_enqueue(ns, packet);
	pthread_mutex_unlock(&ns->packet_mutex);
}

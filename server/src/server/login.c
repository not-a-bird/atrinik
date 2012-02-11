/************************************************************************
*            Atrinik, a Multiplayer Online Role Playing Game            *
*                                                                       *
*    Copyright (C) 2009-2012 Alex Tokar and Atrinik Development Team    *
*                                                                       *
* Fork from Crossfire (Multiplayer game for X-windows).                 *
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
 * Player login/save/logout related functions. */

#include <global.h>
#include <loader.h>

/**
 * Checks to see if the passed player name is valid or not. Does checks
 * like min/max name length, whether there is anyone else playing by that
 * name, and whether there are illegal characters in the player name.
 * @param pl Player.
 * @param name Name to check.
 * @return 1 if the name is ok, 0 otherwise. */
int check_name(player *pl, char *name)
{
	size_t name_len;

	if (name[0] == '\0')
	{
		draw_info_send(0, COLOR_RED, &pl->socket, "You must provide a name to log in.");
		return 0;
	}

	name_len = strlen(name);

	if (name_len < PLAYER_NAME_MIN || name_len > PLAYER_NAME_MAX)
	{
		draw_info_send(0, COLOR_RED, &pl->socket, "That name has an invalid length.");
		return 0;
	}

	if (!playername_ok(name))
	{
		draw_info_send(0, COLOR_RED, &pl->socket, "That name contains illegal characters.");
		return 0;
	}

	return 1;
}

/**
 * Saves a player to file.
 * @param op Player to save.
 * @return Non zero if successful. */
int save_player(object *op)
{
	FILE *fp;
	char filename[MAX_BUF], backupfile[MAX_BUF];
	player *pl = CONTR(op);
	int i;

	/* Sanity check - some stuff changes this when player is exiting */
	if (op->type != PLAYER)
	{
		return 0;
	}

	/* Prevent accidental saves if connection is reset after player has
	 * mostly exited. */
	if (pl->state != ST_PLAYING)
	{
		return 0;
	}

	/* Is this a map players can't save on? */
	if (op->map && MAP_PLAYER_NO_SAVE(op->map))
	{
		return 0;
	}

	snprintf(filename, sizeof(filename), "%s/players/%s/%s.pl", settings.datapath, op->name, op->name);
	path_ensure_directories(filename);
	fp = fopen(filename, "w");
	snprintf(backupfile, sizeof(backupfile), "%s.tmp", filename);
	rename(filename, backupfile);

	if (!fp)
	{
		draw_info(COLOR_WHITE, op, "Can't open file for saving.");
		logger_print(LOG(DEBUG), "Can't open file for saving (%s).", filename);
		rename(backupfile, filename);
		return 0;
	}

	fprintf(fp, "password %s\n", pl->password);
	fprintf(fp, "no_shout %d\n", pl->no_shout);
	fprintf(fp, "tcl %d\n", pl->tcl);
	fprintf(fp, "tgm %d\n", pl->tgm);
	fprintf(fp, "tsi %d\n", pl->tsi);
	fprintf(fp, "tli %d\n", pl->tli);
	fprintf(fp, "tls %d\n", pl->tls);
	fprintf(fp, "gen_hp %d\n", pl->gen_hp);
	fprintf(fp, "gen_sp %d\n", pl->gen_sp);
	fprintf(fp, "digestion %d\n", pl->digestion);

	if (op->map)
	{
		fprintf(fp, "map %s\n", op->map->path);
	}
	else
	{
		fprintf(fp, "map %s\n", EMERGENCY_MAPPATH);
	}

	fprintf(fp, "savebed_map %s\n", pl->savebed_map);
	fprintf(fp, "bed_x %d\nbed_y %d\n", pl->bed_x, pl->bed_y);
	fprintf(fp, "Str %d\n", pl->orig_stats.Str);
	fprintf(fp, "Dex %d\n", pl->orig_stats.Dex);
	fprintf(fp, "Con %d\n", pl->orig_stats.Con);
	fprintf(fp, "Int %d\n", pl->orig_stats.Int);
	fprintf(fp, "Pow %d\n", pl->orig_stats.Pow);
	fprintf(fp, "Wis %d\n", pl->orig_stats.Wis);
	fprintf(fp, "Cha %d\n", pl->orig_stats.Cha);

	for (i = 0; i < pl->num_cmd_permissions; i++)
	{
		if (pl->cmd_permissions[i])
		{
			fprintf(fp, "cmd_permission %s\n", pl->cmd_permissions[i]);
		}
	}

	for (i = 0; i < pl->num_faction_ids; i++)
	{
		if (pl->faction_ids[i])
		{
			fprintf(fp, "faction %s %"FMT64"\n", pl->faction_ids[i], pl->faction_reputation[i]);
		}
	}

	for (i = 0; i < pl->num_region_maps; i++)
	{
		if (pl->region_maps[i])
		{
			fprintf(fp, "rmap %s\n", pl->region_maps[i]);
		}
	}

	fprintf(fp, "fame %"FMT64"\n", pl->fame);
	fprintf(fp, "endplst\n");

	SET_FLAG(op, FLAG_NO_FIX_PLAYER);

	save_object(fp, op);

	/* Make sure the write succeeded */
	if (fclose(fp) == EOF)
	{
		draw_info(COLOR_WHITE, op, "Can't save character.");
		CLEAR_FLAG(op, FLAG_NO_FIX_PLAYER);
		rename(backupfile, filename);
		return 0;
	}

	rename(backupfile, filename);
	chmod(filename, SAVE_MODE);
	CLEAR_FLAG(op, FLAG_NO_FIX_PLAYER);
	return 1;
}

/**
 * Reorder the inventory of an object (reverses the order of the inventory
 * objects).
 * Helper function to reorder the reverse loaded player inventory.
 *
 * @note This will recursively reorder the container inventories.
 * @param op The object to reorder. */
static void reorder_inventory(object *op)
{
	object *tmp, *tmp2;

	tmp2 = op->inv->below;
	op->inv->above = NULL;
	op->inv->below = NULL;

	for (; tmp2; )
	{
		tmp = tmp2;
		/* Save the following element */
		tmp2 = tmp->below;
		tmp->above = NULL;
		/* Resort it like in insert_ob_in_ob() */
		tmp->below = op->inv;
		tmp->below->above = tmp;
		op->inv = tmp;

		if (tmp->inv)
		{
			reorder_inventory(tmp);
		}
	}
}

/**
 * Player in login failed to provide a correct password.
 *
 * After several repeated password failures, kill the socket.
 * @param pl Player. */
static void wrong_password(player *pl)
{
	pl->socket.password_fails++;

	logger_print(LOG(SYSTEM), "%s@%s: Failed to provide correct password.", query_name(pl->ob, NULL), pl->socket.host);

	if (pl->socket.password_fails >= MAX_PASSWORD_FAILURES)
	{
		logger_print(LOG(SYSTEM), "%s@%s: Failed to provide a correct password too many times!", query_name(pl->ob, NULL), pl->socket.host);
		draw_info_send(0, COLOR_RED, &pl->socket, "You have failed to provide a correct password too many times.");
		pl->socket.status = Ns_Zombie;
	}
	else
	{
		FREE_AND_COPY_HASH(pl->ob->name, "noname");
		get_name(pl->ob);
	}
}

/**
 * Login a player.
 * @param op Player. */
void check_login(object *op)
{
	FILE *fp;
	void *mybuffer;
	char filename[MAX_BUF], buf[MAX_BUF], bufall[MAX_BUF];
	int value, correct = 0;
	player *pl = CONTR(op), *pltmp;
	time_t elapsed_save_time = 0;
	struct stat	statbuf;

	strcpy(pl->maplevel, first_map_path);

	/* Check if this matches a connected player, and if so disconnect old
	 * and connect new. */
	for (pltmp = first_player; pltmp != NULL; pltmp = pltmp->next)
	{
		if (pltmp != pl && pltmp->ob->name != NULL && !strcmp(pltmp->ob->name, op->name))
		{
			if (check_password(pl->write_buf + 1, pltmp->password))
			{
				pltmp->socket.status = Ns_Dead;
				/* Need to call this, otherwise the player won't get saved correctly. */
				remove_ns_dead_player(pltmp);
				break;
			}
			else
			{
				wrong_password(pl);
				return;
			}
		}
	}

	if (pl->state == ST_PLAYING)
	{
		logger_print(LOG(SYSTEM), ">%s< from IP %s - double login!", op->name, pl->socket.host);
		pl->socket.status = Ns_Zombie;
		return;
	}

	if (checkbanned(op->name, pl->socket.host))
	{
		logger_print(LOG(SYSTEM), "Ban: Banned player tried to login. [%s@%s]", op->name, pl->socket.host);
		draw_info_send(0, COLOR_RED, &pl->socket, "Connection refused due to a ban.");
		pl->socket.status = Ns_Zombie;
		return;
	}

	logger_print(LOG(INFO), "Login %s from IP %s", op->name, pl->socket.host);

	snprintf(filename, sizeof(filename), "%s/players/%s/%s.pl", settings.datapath, op->name, op->name);
	fp = fopen(filename, "rb");

	/* If no file, must be a new player, so lets get confirmation of
	 * the password.  Return control to the higher level dispatch,
	 * since the rest of this just deals with loading of the file. */
	if (!fp)
	{
		confirm_password(op);
		return;
	}

	if (fstat(fileno(fp), &statbuf))
	{
		logger_print(LOG(BUG), "Unable to stat %s?", filename);
		elapsed_save_time = 0;
	}
	else
	{
		elapsed_save_time = time(NULL) - statbuf.st_mtime;

		if (elapsed_save_time < 0)
		{
			logger_print(LOG(BUG), "Player file %s was saved in the future? (%"FMT64U" time)", filename, (uint64) elapsed_save_time);
			elapsed_save_time = 0;
		}
	}

	if (fgets(bufall, sizeof(bufall), fp))
	{
		if (sscanf(bufall, "password %s\n", buf))
		{
			correct = check_password(pl->write_buf + 1, buf);
		}
	}

	if (!correct)
	{
		wrong_password(pl);
		return;
	}

#ifdef SAVE_INTERVAL
	pl->last_save_time = time(NULL);
#endif

	pl->party = NULL;

	pl->orig_stats.Str = 0;
	pl->orig_stats.Dex = 0;
	pl->orig_stats.Con = 0;
	pl->orig_stats.Int = 0;
	pl->orig_stats.Pow = 0;
	pl->orig_stats.Wis = 0;
	pl->orig_stats.Cha = 0;
	strcpy(pl->savebed_map, first_map_path);
	pl->bed_x = 0;
	pl->bed_y = 0;

	/* Loop through the file, loading the rest of the values. */
	while (fgets(bufall, sizeof(bufall), fp))
	{
		sscanf(bufall, "%s %d\n", buf, &value);

		if (!strcmp(buf, "endplst"))
		{
			break;
		}
		else if (!strcmp(buf, "no_shout"))
		{
			pl->no_shout = value;
		}
		else if (!strcmp(buf, "tcl"))
		{
			pl->tcl = value;
		}
		else if (!strcmp(buf, "tgm"))
		{
			pl->tgm = value;
		}
		else if (!strcmp(buf, "tsi"))
		{
			pl->tsi = value;
		}
		else if (!strcmp(buf, "tli"))
		{
			pl->tli = value;
		}
		else if (!strcmp(buf, "tls"))
		{
			pl->tls = value;
		}
		else if (!strcmp(buf, "gen_hp"))
		{
			pl->gen_hp = value;
		}
		else if (!strcmp(buf, "gen_sp"))
		{
			pl->gen_sp = value;
		}
		else if (!strcmp(buf, "digestion"))
		{
			pl->digestion = value;
		}
		else if (!strcmp(buf, "map"))
		{
			sscanf(bufall, "map %s", pl->maplevel);
		}
		else if (!strcmp(buf, "savebed_map"))
		{
			sscanf(bufall, "savebed_map %s", pl->savebed_map);
		}
		else if (!strcmp(buf, "bed_x"))
		{
			pl->bed_x = value;
		}
		else if (!strcmp(buf, "bed_y"))
		{
			pl->bed_y = value;
		}
		else if (!strcmp(buf, "Str"))
		{
			pl->orig_stats.Str = value;
		}
		else if (!strcmp(buf, "Dex"))
		{
			pl->orig_stats.Dex = value;
		}
		else if (!strcmp(buf, "Con"))
		{
			pl->orig_stats.Con = value;
		}
		else if (!strcmp(buf, "Int"))
		{
			pl->orig_stats.Int = value;
		}
		else if (!strcmp(buf, "Pow"))
		{
			pl->orig_stats.Pow = value;
		}
		else if (!strcmp(buf, "Wis"))
		{
			pl->orig_stats.Wis = value;
		}
		else if (!strcmp(buf, "Cha"))
		{
			pl->orig_stats.Cha = value;
		}
		else if (!strcmp(buf, "cmd_permission"))
		{
			char *cp = strchr(bufall, '\n');

			*cp = '\0';
			cp = strchr(bufall, ' ');
			cp++;

			pl->num_cmd_permissions++;
			pl->cmd_permissions = realloc(pl->cmd_permissions, sizeof(char *) * pl->num_cmd_permissions);
			pl->cmd_permissions[pl->num_cmd_permissions - 1] = strdup(cp);
		}
		else if (!strcmp(buf, "faction"))
		{
			char faction_id[MAX_BUF];
			sint64 rep;

			if (sscanf(bufall, "faction %s %"FMT64, faction_id, &rep) == 2)
			{
				pl->faction_ids = realloc(pl->faction_ids, sizeof(*pl->faction_ids) * (pl->num_faction_ids + 1));
				pl->faction_reputation = realloc(pl->faction_reputation, sizeof(*pl->faction_reputation) * (pl->num_faction_ids + 1));
				pl->faction_ids[pl->num_faction_ids] = add_string(faction_id);
				pl->faction_reputation[pl->num_faction_ids] = rep;
				pl->num_faction_ids++;
			}
		}
		else if (!strcmp(buf, "fame"))
		{
			pl->fame = value;
		}
		else if (!strcmp(buf, "rmap"))
		{
			char *cp = strchr(bufall, '\n');

			*cp = '\0';
			cp = strchr(bufall, ' ');
			cp++;

			pl->region_maps = realloc(pl->region_maps, sizeof(*pl->region_maps) * (pl->num_region_maps + 1));
			pl->region_maps[pl->num_region_maps] = strdup(cp);
			pl->num_region_maps++;
		}
	}

	/* Take the player object out from the void */
	if (!QUERY_FLAG(op, FLAG_REMOVED))
	{
		object_remove(op, 0);
	}

	/* We transfer it to a new object */
	op->custom_attrset = NULL;

	object_destroy(op);

	/* Create a new object for the real player data */
	op = get_object();

	/* This loads the standard objects values. */
	mybuffer = create_loader_buffer(fp);
	load_object(fp, op, mybuffer, LO_REPEAT, 0);
	delete_loader_buffer(mybuffer);
	fclose(fp);

	/* The inventory of players is reverse loaded, so let's exchange the
	 * order here. */
	if (op->inv)
	{
		reorder_inventory(op);
	}

	op->custom_attrset = pl;
	pl->ob = op;
	CLEAR_FLAG(op, FLAG_NO_FIX_PLAYER);

	op->type = PLAYER;

	/* This is a funny thing: what happens when the autosave function saves a player
	 * with negative hp? Well, the sever tries to create a gravestone and heals the
	 * player... and then server tries to insert gravestone and anim on a map - but
	 * player is still in login! So, we are nice and set hp to 1 if here negative. */
	if (op->stats.hp < 0)
	{
		op->stats.hp = 1;
	}

#ifdef AUTOSAVE
	pl->last_save_tick = pticks;
#endif
	op->carrying = sum_weight(op);

	fix_player(op);
	link_player_skills(op);

	pl->state = ST_PLAYING;

	/* Display Message of the Day */
	display_motd(op);

	draw_info_flags_format(NDI_ALL, COLOR_DK_ORANGE, NULL, "%s has entered the game.", query_name(pl->ob, NULL));

	/* Trigger the global LOGIN event */
	trigger_global_event(GEVENT_LOGIN, pl, pl->socket.host);

	esrv_new_player(pl, op->weight + op->carrying);
	esrv_send_inventory(op, op);

	if (!QUERY_FLAG(op, FLAG_FRIENDLY))
	{
		logger_print(LOG(BUG), "Player %s was loaded without friendly flag!", query_name(op, NULL));
		SET_FLAG(op, FLAG_FRIENDLY);
	}

	enter_exit(op, NULL);

	pl->socket.update_tile = 0;
	pl->socket.look_position = 0;
	pl->socket.ext_title_flag = 1;

	/* No direction; default to southeast. */
	if (!op->direction)
	{
		op->direction = SOUTHEAST;
	}

	op->anim_last_facing = op->anim_last_facing_last = op->facing = op->direction;
	/* We assume that players always have a valid animation. */
	SET_ANIMATION(op, (NUM_ANIMATIONS(op) / NUM_FACINGS(op)) * op->direction);
	esrv_new_player(pl, op->weight + op->carrying);
	send_quickslots(pl);

	if (op->map && op->map->events)
	{
		trigger_map_event(MEVENT_LOGIN, op->map, op, NULL, NULL, NULL, 0);
	}
}

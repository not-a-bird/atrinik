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
 * Map effects handling. */

#include <include.h>

/** Linked list of possible effects. */
static effect_struct *effects = NULL;
/** Current effect. */
static effect_struct *current_effect = NULL;

/**
 * Initialize effects from file. */
void effects_init()
{
	FILE *fp;
	char buf[MAX_BUF], *cp;
	effect_struct *effect = NULL;
	effect_sprite_def *sprite_def = NULL;

	/* Try to deinitialize all effects first. */
	effects_deinit();

	fp = server_file_open(SERVER_FILE_EFFECTS);

	if (!fp)
	{
		return;
	}

	/* Read the file... */
	while (fgets(buf, sizeof(buf) - 1, fp))
	{
		/* Ignore comments and blank lines. */
		if (buf[0] == '#' || buf[0] == '\n')
		{
			continue;
		}

		/* End a previous 'effect xxx' or 'sprite xxx' block. */
		if (!strcmp(buf, "end\n"))
		{
			/* Inside effect block. */
			if (effect)
			{
				/* Inside sprite block. */
				if (sprite_def)
				{
					/* Add this sprite to the linked list. */
					sprite_def->next = effect->sprite_defs;
					effect->sprite_defs = sprite_def;
					/* Update total chance value. */
					effect->chance_total += sprite_def->chance;
					sprite_def = NULL;
				}
				/* Inside effect block. */
				else
				{
					/* Add this effect to the linked list of effects. */
					effect->next = effects;
					effects = effect;
					effect = NULL;
				}
			}

			continue;
		}

		cp = strrchr(buf, '\n');

		/* Eliminate newline. */
		if (cp)
		{
			*cp = '\0';
		}

		/* Parse definitions inside sprite block. */
		if (sprite_def)
		{
			if (!strncmp(buf, "chance ", 7))
			{
				sprite_def->chance = atoi(buf + 7);
			}
			else if (!strncmp(buf, "weight ", 7))
			{
				sprite_def->weight = atof(buf + 7);
			}
			else if (!strncmp(buf, "weight_mod ", 11))
			{
				sprite_def->weight_mod = atof(buf + 11);
			}
			else if (!strncmp(buf, "delay ", 6))
			{
				sprite_def->delay = atoi(buf + 6);
			}
			else if (!strncmp(buf, "wind ", 5))
			{
				sprite_def->wind = atoi(buf + 5);
			}
			else if (!strncmp(buf, "wiggle ", 7))
			{
				sprite_def->wiggle = atof(buf + 7);
			}
			else if (!strncmp(buf, "wind_mod ", 9))
			{
				sprite_def->wind_mod = atof(buf + 9);
			}
			else if (!strncmp(buf, "x ", 2))
			{
				sprite_def->x = atoi(buf + 2);
			}
			else if (!strncmp(buf, "y ", 2))
			{
				sprite_def->y = atoi(buf + 2);
			}
			else if (!strncmp(buf, "xpos ", 5))
			{
				sprite_def->xpos = atoi(buf + 5);
			}
			else if (!strncmp(buf, "ypos ", 5))
			{
				sprite_def->ypos = atoi(buf + 5);
			}
			else if (!strncmp(buf, "reverse ", 8))
			{
				sprite_def->reverse = atoi(buf + 8);
				sprite_def->y_mod = -sprite_def->y_mod;
			}
			else if (!strncmp(buf, "y_rndm ", 7))
			{
				sprite_def->y_rndm = atof(buf + 7);
			}
			else if (!strncmp(buf, "x_mod ", 6))
			{
				sprite_def->x_mod = atof(buf + 6);
			}
			else if (!strncmp(buf, "y_mod ", 6))
			{
				sprite_def->y_mod = atof(buf + 6);
			}
			else if (!strncmp(buf, "x_check_mod ", 12))
			{
				sprite_def->x_check_mod = atoi(buf + 12);
			}
			else if (!strncmp(buf, "y_check_mod ", 12))
			{
				sprite_def->y_check_mod = atoi(buf + 12);
			}
			else if (!strncmp(buf, "kill_sides ", 11))
			{
				sprite_def->kill_side_left = sprite_def->kill_side_right = atoi(buf + 11);
			}
			else if (!strncmp(buf, "kill_side_left ", 15))
			{
				sprite_def->kill_side_left = atoi(buf + 15);
			}
			else if (!strncmp(buf, "kill_side_right ", 16))
			{
				sprite_def->kill_side_right = atoi(buf + 16);
			}
			else if (!strncmp(buf, "zoom ", 5))
			{
				sprite_def->zoom = atoi(buf + 5);
			}
		}
		/* Parse definitions inside effect block. */
		else if (effect)
		{
			if (!strncmp(buf, "wind_chance ", 12))
			{
				effect->wind_chance = atof(buf + 12);
			}
			else if (!strncmp(buf, "sprite_chance ", 14))
			{
				effect->sprite_chance = atof(buf + 14);
			}
			else if (!strncmp(buf, "delay ", 6))
			{
				effect->delay = atoi(buf + 6);
			}
			else if (!strncmp(buf, "wind_blow_dir ", 14))
			{
				effect->wind_blow_dir = atoi(buf + 14);
			}
			else if (!strncmp(buf, "max_sprites ", 12))
			{
				effect->max_sprites = atoi(buf + 12);
			}
			else if (!strncmp(buf, "wind_mod ", 9))
			{
				effect->wind_mod = atof(buf + 9);
			}
			else if (!strncmp(buf, "sprites_per_move ", 17))
			{
				effect->sprites_per_move = atoi(buf + 17);
			}
			/* Start of sprite block. */
			else if (!strncmp(buf, "sprite ", 7))
			{
				sprite_def = calloc(1, sizeof(*sprite_def));
				/* Store the sprite ID. */
				sprite_def->id = get_bmap_id(buf + 7);
				/* Initialize default values. */
				sprite_def->chance = 1;
				sprite_def->weight = 1.0;
				sprite_def->weight_mod = 2.0;
				sprite_def->wind = 1;
				sprite_def->wiggle = 1.0;
				sprite_def->wind_mod = 1.0;
				sprite_def->x = -1;
				sprite_def->y = -1;
				sprite_def->reverse = 0;
				sprite_def->y_rndm = 60.0;
				sprite_def->x_mod = 1.0;
				sprite_def->y_mod = 1.0;
				sprite_def->x_check_mod = 1;
				sprite_def->y_check_mod = 1;
				sprite_def->kill_side_left = 1;
				sprite_def->kill_side_right = 0;
				sprite_def->zoom = 0;
			}
		}
		/* Start of effect block. */
		else if (!strncmp(buf, "effect ", 7))
		{
			effect = calloc(1, sizeof(effect_struct));
			/* Store the effect unique name. */
			strncpy(effect->name, buf + 7, sizeof(effect->name) - 1);
			effect->name[sizeof(effect->name) - 1] = '\0';
			/* Initialize default values. */
			effect->wind_chance = 0.98;
			effect->sprite_chance = 60.0;
			effect->wind_blow_dir = WIND_BLOW_RANDOM;
			effect->wind_mod = 1.0;
			effect->max_sprites = -1;
			effect->sprites_per_move = 1;
		}
	}

	/* Close the file. */
	fclose(fp);
}

/**
 * Deinitialize ::effects linked list. */
void effects_deinit()
{
	effect_struct *effect, *effect_next;
	effect_sprite_def *sprite_def, *sprite_def_next;

	/* Deinitialize all effects. */
	for (effect = effects; effect; effect = effect_next)
	{
		effect_next = effect->next;

		/* Deinitialize the effect's sprite definitions. */
		for (sprite_def = effect->sprite_defs; sprite_def; sprite_def = sprite_def_next)
		{
			sprite_def_next = sprite_def->next;
			effect_sprite_def_free(sprite_def);
		}

		/* Deinitialize the shown sprites and the actual effect. */
		effect_sprites_free(effect);
		effect_free(effect);
	}

	effects = current_effect = NULL;
}

/**
 * Deinitialize shown sprites of a single effect.
 * @param effect The effect to have shown sprites deinitialized. */
void effect_sprites_free(effect_struct *effect)
{
	effect_sprite *tmp, *next;

	for (tmp = effect->sprites; tmp; tmp = next)
	{
		next = tmp->next;
		effect_sprite_free(tmp);
	}

	effect->sprites = effect->sprites_end = NULL;
}

/**
 * Deinitialize a single effect.
 * @param effect Effect that will be freed. */
void effect_free(effect_struct *effect)
{
	free(effect);
}

/**
 * Deinitialize a single sprite definition.
 * @param sprite_def Sprite definition that will be freed. */
void effect_sprite_def_free(effect_sprite_def *sprite_def)
{
	free(sprite_def);
}

/**
 * Deinitialize a single shown sprite.
 * @param sprite Sprite that will be freed. */
void effect_sprite_free(effect_sprite *sprite)
{
	free(sprite);
}

/**
 * Remove a single shown sprite from the linked list and free it.
 * @param sprite Sprite to remove and free. */
void effect_sprite_remove(effect_sprite *sprite)
{
	if (!sprite || !current_effect)
	{
		return;
	}

	if (sprite->prev)
	{
		sprite->prev->next = sprite->next;
	}
	else
	{
		current_effect->sprites = sprite->next;
	}

	if (sprite->next)
	{
		sprite->next->prev = sprite->prev;
	}
	else
	{
		current_effect->sprites_end = sprite->prev;
	}

	effect_sprite_free(sprite);
}

/**
 * Allocate a new sprite object and add it to the link of currently shown sprites.
 *
 * A random sprite definition object will be chosen.
 * @param effect Effect this is being done for.
 * @return The created sprite. */
static effect_sprite *effect_sprite_create(effect_struct *effect)
{
	int roll;
	effect_sprite_def *tmp;
	effect_sprite *sprite;

	/* Choose which sprite to use. */
	roll = rndm(1, effect->chance_total) - 1;

	for (tmp = effect->sprite_defs; tmp; tmp = tmp->next)
	{
		roll -= tmp->chance;

		if (roll < 0)
		{
			break;
		}
	}

	if (!tmp)
	{
		return NULL;
	}

	/* Allocate a new sprite. */
	sprite = calloc(1, sizeof(*sprite));
	sprite->def = tmp;

	/* Add it to the linked list. */
	if (!effect->sprites)
	{
		effect->sprites = effect->sprites_end = sprite;
	}
	else
	{
		effect->sprites_end->next = sprite;
		sprite->prev = effect->sprites_end;
		effect->sprites_end = sprite;
	}

	return sprite;
}

/**
 * Try to play effect sprites. */
void effect_sprites_play()
{
	effect_sprite *tmp, *next;
	int num_sprites = 0;
	int x_check, y_check;

	/* No current effect or not playing, quit. */
	if (!current_effect || GameStatus != GAME_STATUS_PLAY)
	{
		return;
	}

	for (tmp = current_effect->sprites; tmp; tmp = next)
	{
		next = tmp->next;

		x_check = y_check = 0;

		if (tmp->def->x_check_mod)
		{
			x_check = FaceList[tmp->def->id].sprite->bitmap->w;
		}

		if (tmp->def->y_check_mod)
		{
			y_check = FaceList[tmp->def->id].sprite->bitmap->h;
		}

		/* Off-screen? */
		if ((tmp->def->kill_side_left && tmp->x + x_check < 0) || (tmp->def->kill_side_right && tmp->x - x_check > ScreenSurfaceMap->w) || tmp->y + y_check < 0 || tmp->y - y_check > ScreenSurfaceMap->h)
		{
			effect_sprite_remove(tmp);
			continue;
		}

		/* Show the sprite. */
		sprite_blt_map(FaceList[tmp->def->id].sprite, tmp->x, tmp->y, NULL, NULL, 0, tmp->def->zoom);
		num_sprites++;

		/* Move it if there is no delay configured or if enough time has passed. */
		if (!tmp->def->delay || !tmp->delay_ticks || SDL_GetTicks() - tmp->delay_ticks > tmp->def->delay)
		{
			int ypos = tmp->def->weight * tmp->def->weight_mod;

			if (tmp->def->reverse)
			{
				ypos = -ypos;
			}

			tmp->y += ypos;
			tmp->x += (-1.0 + 3.0 * RANDOM() / (RAND_MAX + 1.0)) * tmp->def->wiggle;

			/* Apply wind. */
			if (tmp->def->wind && current_effect->wind_blow_dir != WIND_BLOW_NONE)
			{
				tmp->x += ((double) current_effect->wind / tmp->def->weight + tmp->def->weight * tmp->def->weight_mod * ((-1.0 + 2.0 * RANDOM() / (RAND_MAX + 1.0)) * tmp->def->wind_mod));
			}

			tmp->delay_ticks = SDL_GetTicks();
			map_redraw_flag = 1;
		}
	}

	/* Change wind direction... */
	if (current_effect->wind_blow_dir == WIND_BLOW_RANDOM && current_effect->wind_chance != 1.0 && (current_effect->wind_chance == 0.0 || RANDOM() / (RAND_MAX + 1.0) >= current_effect->wind_chance))
	{
		current_effect->wind += (-2.0 + 4.0 * RANDOM() / (RAND_MAX + 1.0)) * current_effect->wind_mod;
	}

	if (current_effect->wind_blow_dir == WIND_BLOW_LEFT)
	{
		current_effect->wind = -1.0 * current_effect->wind_mod;
	}
	else if (current_effect->wind_blow_dir == WIND_BLOW_RIGHT)
	{
		current_effect->wind = 1.0 * current_effect->wind_mod;
	}

	if ((current_effect->max_sprites == -1 || num_sprites < current_effect->max_sprites) && (!current_effect->delay || !current_effect->delay_ticks || SDL_GetTicks() - current_effect->delay_ticks > current_effect->delay) && RANDOM() / (RAND_MAX + 1.0) >= (100.0 - current_effect->sprite_chance) / 100.0)
	{
		int i;
		effect_sprite *sprite;

		for (i = 0; i < current_effect->sprites_per_move; i++)
		{
			/* Add the sprite. */
			sprite = effect_sprite_create(current_effect);

			if (!sprite)
			{
				return;
			}

			/* Invalid sprite. */
			if (sprite->def->id == -1 || !FaceList[sprite->def->id].sprite)
			{
				LOG(llevInfo, "Invalid sprite ID %d\n", sprite->def->id);
				effect_sprite_remove(sprite);
				return;
			}

			if (sprite->def->x != -1)
			{
				sprite->x = sprite->def->x;
			}
			else
			{
				/* Calculate where to put the sprite. */
				sprite->x = (double) ScreenSurfaceMap->w * RANDOM() / (RAND_MAX + 1.0) * sprite->def->x_mod;
			}

			if (sprite->def->reverse)
			{
				sprite->y = ScreenSurfaceMap->h - FaceList[sprite->def->id].sprite->bitmap->h;
			}
			else if (sprite->def->y != -1)
			{
				sprite->y = sprite->def->y;
			}

			sprite->y += sprite->def->y_rndm * (RANDOM() / (RAND_MAX + 1.0) * sprite->def->y_mod);

			sprite->x += sprite->def->xpos;
			sprite->y += sprite->def->ypos;
			map_redraw_flag = 1;
		}

		current_effect->delay_ticks = SDL_GetTicks();
	}
}

/**
 * Start an effect identified by its name.
 * @param name Name of the effect to start. 'none' is a reserved effect name
 * and will stop any currently playing effect.
 * @return 1 if the effect was started, 0 otherwise. */
int effect_start(const char *name)
{
	effect_struct *tmp;

	/* Stop playing any effect. */
	if (!strcmp(name, "none"))
	{
		effect_stop();
		return 1;
	}

	/* Already playing the same effect? */
	if (current_effect && !strcmp(current_effect->name, name))
	{
		return 1;
	}

	/* Find the effect... */
	for (tmp = effects; tmp; tmp = tmp->next)
	{
		/* Found it? */
		if (!strcmp(tmp->name, name))
		{
			/* Stop current effect (if any) */
			effect_stop();
			/* Reset wind direction. */
			tmp->wind = 0;
			/* Load it up. */
			current_effect = tmp;
			return 1;
		}
	}

	return 0;
}

/**
 * Used for debugging effects code using /d_effect command.
 * @param type What debugging command to run. */
void effect_debug(const char *type)
{
	if (!strcmp(type, "num"))
	{
		uint32 num = 0;
		uint64 bytes;
		double kbytes;
		effect_sprite *tmp;

		if (!current_effect)
		{
			draw_info("No effect is currently playing.", COLOR_RED);
			return;
		}

		for (tmp = current_effect->sprites; tmp; tmp = tmp->next)
		{
			num++;
		}

		bytes = ((uint64) sizeof(effect_sprite)) * num;
		kbytes = (double) bytes / 1024;

		draw_info_format(COLOR_WHITE, "Visible sprites: <green>%d</green> using <green>%"FMT64U"</green> bytes (<green>%2.2f</green> KB)", num, bytes, kbytes);
	}
	else if (!strcmp(type, "sizeof"))
	{
		draw_info("Information about various data structures used by effects:\n", COLOR_WHITE);
		draw_info_format(COLOR_WHITE, "Size of a single sprite definition: <green>%"FMT64U"</green>", (uint64) sizeof(effect_sprite_def));
		draw_info_format(COLOR_WHITE, "Size of a single visible sprite: <green>%"FMT64U"</green>", (uint64) sizeof(effect_sprite));
		draw_info_format(COLOR_WHITE, "Size of a single effect structure: <green>%"FMT64U"</green>", (uint64) sizeof(effect_struct));
	}
	else
	{
		draw_info_format(COLOR_RED, "No such debug option '%s'.", type);
	}
}

/**
 * Stop currently playing effect. */
void effect_stop()
{
	if (!current_effect)
	{
		return;
	}

	effect_sprites_free(current_effect);
	current_effect = NULL;
}

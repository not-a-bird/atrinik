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
 * This file handles all text window related functions */

#include <include.h>

static int old_slider_pos = 0;
static SDL_Surface *txtwinbg = NULL;
static int old_txtwin_alpha = -1;
static widgetdata *selection_widget = NULL;
static Uint32 textwin_border, textwin_border_selected;

/**
 * Initialize text window variables. */
void textwin_init()
{
	textwin_border = SDL_MapRGBA(ScreenSurface->format, 0x60, 0x60, 0x60, 255);
	textwin_border_selected = SDL_MapRGBA(ScreenSurface->format, 177, 126, 5, 255);
}

/**
 * Makes sure textwin's scroll value is a sane number.
 * @param widget Text window's widget. */
void textwin_scroll_adjust(widgetdata *widget)
{
	textwin_struct *textwin = TEXTWIN(widget);

	if (textwin->scroll < TEXTWIN_ROWS_VISIBLE(widget))
	{
		textwin->scroll = TEXTWIN_ROWS_VISIBLE(widget);
	}

	if (textwin->scroll > (int) textwin->num_entries)
	{
		textwin->scroll = textwin->num_entries;
	}
}

/**
 * Readjust text window's scroll/entries counts due to a font size
 * change.
 * @param widget Text window's widget. */
void textwin_readjust(widgetdata *widget)
{
	textwin_struct *textwin = TEXTWIN(widget);
	SDL_Rect box;
	int scroll;

	if (!textwin->entries)
	{
		return;
	}

	box.w = widget->wd - Bitmaps[BITMAP_SLIDER]->bitmap->w - 7;
	box.h = 0;
	box.x = 0;
	box.y = 0;
	string_blt(NULL, textwin->font, textwin->entries, 3, 0, COLOR_SIMPLE(COLOR_WHITE), TEXTWIN_TEXT_FLAGS(widget) | TEXT_HEIGHT, &box);
	scroll = box.h / FONT_HEIGHT(textwin->font);

	/* Adjust the counts. */
	textwin->scroll = scroll;
	textwin->num_entries = scroll;
}

/**
 * Draw info with format arguments.
 * @param flags Various flags, like color.
 * @param format Format arguments. */
void draw_info_format(int flags, char *format, ...)
{
	char buf[4096];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	draw_info(flags, buf);
}

/**
 * Add string to the text window.
 * @param flags Various flags, like color.
 * @param str The string. */
void draw_info(int flags, const char *str)
{
	widgetdata *widget;
	textwin_struct *textwin;
	size_t len, scroll;
	SDL_Rect box;

	if (flags & NDI_PLAYER)
	{
		widget = cur_widget[CHATWIN_ID];
	}
	else
	{
		widget = cur_widget[MSGWIN_ID];
	}

	textwin = TEXTWIN(widget);
	WIDGET_REDRAW(widget);

	box.w = widget->wd - Bitmaps[BITMAP_SLIDER]->bitmap->w - 7;
	box.h = 0;

	len = strlen(str);
	/* Resize the characters array as needed. */
	textwin->entries = realloc(textwin->entries, textwin->entries_size + len + 4);
	/* Tells the text module what color to use. */
	textwin->entries[textwin->entries_size] = '\r';
	textwin->entries[textwin->entries_size + 1] = (flags & 0xff) + 1;
	textwin->entries_size += 2;
	/* Add the string, newline and terminating \0. */
	strcpy(textwin->entries + textwin->entries_size, str);
	textwin->entries[textwin->entries_size + len] = '\n';
	textwin->entries[textwin->entries_size + len + 1] = '\0';
	textwin->entries_size += len + 1;

	box.y = 0;
	/* Get the string's height. */
	string_blt(NULL, textwin->font, str, 3, 0, COLOR_SIMPLE(COLOR_WHITE), TEXTWIN_TEXT_FLAGS(widget) | TEXT_HEIGHT, &box);
	scroll = box.h / FONT_HEIGHT(textwin->font) + 1;

	/* Adjust the counts. */
	textwin->scroll += scroll;
	textwin->num_entries += scroll;

	/* Have the entries gone over maximum allowed lines? */
	if (textwin->entries && textwin->num_entries >= (size_t) options.chat_max_lines)
	{
		char *cp;

		while (textwin->num_entries >= (size_t) options.chat_max_lines && (cp = strchr(textwin->entries, '\n')))
		{
			size_t pos = cp - textwin->entries + 1;
			char *buf = malloc(pos + 1);

			/* Copy the string together with the newline to a temporary
			 * buffer. */
			memcpy(buf, textwin->entries, pos);
			buf[pos] = '\0';

			/* Get the string's height. */
			box.h = 0;
			string_blt(NULL, textwin->font, buf, 3, 0, COLOR_SIMPLE(COLOR_WHITE), TEXTWIN_TEXT_FLAGS(widget) | TEXT_HEIGHT, &box);
			scroll = box.h / FONT_HEIGHT(textwin->font);

			free(buf);

			/* Move the string after the found newline to the beginning,
			 * effectively erasing the previous line. */
			textwin->entries_size -= pos;
			memcpy(textwin->entries, textwin->entries + pos, textwin->entries_size);
			textwin->entries[textwin->entries_size] = '\0';

			/* Adjust the counts. */
			textwin->scroll -= scroll;
			textwin->num_entries -= scroll;
		}
	}
}

/**
 * Handle ctrl+C. */
void textwin_handle_copy()
{
	sint64 start, end;
	textwin_struct *textwin;
	char *str, *cp;
	size_t i, pos;

	/* Nothing to copy? */
	if (!selection_widget)
	{
		return;
	}

	textwin = TEXTWIN(selection_widget);

	start = textwin->selection_start;
	end = textwin->selection_end;

	if (end < start)
	{
		start = textwin->selection_end;
		end = textwin->selection_start;
	}

	if (end - start <= 0)
	{
		return;
	}

	/* Get the string to copy, depending on the start and end positions. */
	str = malloc(sizeof(char) * (end - start + 1 + 1));
	memcpy(str, textwin->entries + start, end - start + 1);
	str[end - start + 1] = '\0';

	cp = malloc(sizeof(char) * (end - start + 1 + 1));
	i = 0;

	/* Remove the special \r color changers. */
	for (pos = 0; pos < (size_t) (end - start + 1); pos++)
	{
		if (str[pos] == '\r')
		{
			pos++;
		}
		else
		{
			cp[i] = str[pos];
			i++;
		}
	}

	cp[i] = '\0';

	clipboard_set(cp);
	free(str);
	free(cp);
}

/**
 * Draw a text window.
 * @param actWin The text window ID.
 * @param x X position of the text window.
 * @param y Y position of the text window.
 * @param bltfx The surface. */
static void show_window(widgetdata *widget, int x, int y, _BLTFX *bltfx)
{
	textwin_struct *textwin = TEXTWIN(widget);
	SDL_Rect box;
	int temp;

	textwin->x = x;
	textwin->y = y;
	x = y = 0;

	/* Show the text entries, if any. */
	if (textwin->entries)
	{
		box.w = widget->wd - Bitmaps[BITMAP_SLIDER]->bitmap->w - 7;
		box.h = widget->ht;
		box.y = MAX(0, textwin->scroll - TEXTWIN_ROWS_VISIBLE(widget));
		text_set_selection(&textwin->selection_start, &textwin->selection_end, &textwin->selection_started);
		string_blt(bltfx->surface, textwin->font, textwin->entries, x + 3, y + 1, COLOR_SIMPLE(COLOR_WHITE), TEXTWIN_TEXT_FLAGS(widget) | TEXT_HEIGHT, &box);
		text_set_selection(NULL, NULL, NULL);
	}

	/* Only draw scrollbar if needed */
	if (textwin->num_entries > (size_t) TEXTWIN_ROWS_VISIBLE(widget))
	{
		SDL_Rect box;

		box.x = box.y = 0;
		box.w = Bitmaps[BITMAP_SLIDER]->bitmap->w;
		box.h = widget->ht - 12;

		/* No textinput-line */
		temp = -11;
		sprite_blt(Bitmaps[BITMAP_SLIDER_UP], x + widget->wd - 11, y + 1, NULL, bltfx);
		sprite_blt(Bitmaps[BITMAP_SLIDER_DOWN], x + widget->wd - 11, y + temp + widget->ht, NULL, bltfx);
		sprite_blt(Bitmaps[BITMAP_SLIDER], x + widget->wd - 11, y + Bitmaps[BITMAP_SLIDER_UP]->bitmap->h + 2 + temp, &box, bltfx);
		box.h += temp - 1;
		box.w -= 2;

		/* Between 0.0 <-> 1.0 */
		textwin->slider_h = box.h * TEXTWIN_ROWS_VISIBLE(widget) / textwin->num_entries;
		textwin->slider_y = ((textwin->scroll - TEXTWIN_ROWS_VISIBLE(widget)) * box.h) / textwin->num_entries;

		if (textwin->slider_h < 1)
		{
			textwin->slider_h = 1;
		}

		if (textwin->scroll - TEXTWIN_ROWS_VISIBLE(widget) > 0 && textwin->slider_y + textwin->slider_h < box.h)
		{
			textwin->slider_y++;
		}

		box.h = textwin->slider_h;
		sprite_blt(Bitmaps[BITMAP_TWIN_SCROLL], x + widget->wd - 9, y + Bitmaps[BITMAP_SLIDER_UP]->bitmap->h + 2 + textwin->slider_y, &box, bltfx);

		if (textwin->highlight == TW_HL_UP)
		{
			box.x = x + widget->wd - 11;
			box.y = y + 1;
			box.h = Bitmaps[BITMAP_SLIDER_UP]->bitmap->h;
			box.w = 1;
			SDL_FillRect(bltfx->surface, &box, -1);
			box.x += Bitmaps[BITMAP_SLIDER_UP]->bitmap->w - 1;
			SDL_FillRect(bltfx->surface, &box, -1);
			box.w = Bitmaps[BITMAP_SLIDER_UP]->bitmap->w - 1;
			box.h = 1;
			box.x = x + widget->wd - 11;
			SDL_FillRect(bltfx->surface, &box, -1);
			box.y += Bitmaps[BITMAP_SLIDER_UP]->bitmap->h - 1;
			SDL_FillRect(bltfx->surface, &box, -1);
		}
		else if (textwin->highlight == TW_ABOVE)
		{
			box.x = x + widget->wd - 9;
			box.y = y + Bitmaps[BITMAP_SLIDER_UP]->bitmap->h + 1;
			box.h = textwin->slider_y + 1;
			box.w = 5;
			SDL_FillRect(bltfx->surface, &box, 0);
		}
		else if (textwin->highlight == TW_HL_SLIDER)
		{
			box.x = x + widget->wd - 9;
			box.y = y + Bitmaps[BITMAP_SLIDER_UP]->bitmap->h + 2 + textwin->slider_y;
			box.w = 1;
			SDL_FillRect(bltfx->surface, &box, -1);
			box.x += 4;
			SDL_FillRect(bltfx->surface, &box, -1);
			box.x -= 4;
			box.h = 1;
			box.w = 4;
			SDL_FillRect(bltfx->surface, &box, -1);
			box.y += textwin->slider_h - 1;
			SDL_FillRect(bltfx->surface, &box, -1);
		}
		else if (textwin->highlight == TW_UNDER)
		{
			box.x = x + widget->wd - 9;
			box.y = y + Bitmaps[BITMAP_SLIDER_UP]->bitmap->h + 2 + textwin->slider_y + box.h;
			box.h = widget->ht - 13 - textwin->slider_y - textwin->slider_h - 10;
			box.w = 5;
			SDL_FillRect(bltfx->surface, &box, 0);
		}
		else if (textwin->highlight == TW_HL_DOWN)
		{
			box.x = x + widget->wd - 11;
			box.y = y + widget->ht - 13 + 2;
			box.h = Bitmaps[BITMAP_SLIDER_UP]->bitmap->h;
			box.w = 1;
			SDL_FillRect(bltfx->surface, &box, -1);
			box.x += Bitmaps[BITMAP_SLIDER_UP]->bitmap->w - 1;
			SDL_FillRect(bltfx->surface, &box, -1);
			box.w = Bitmaps[BITMAP_SLIDER_UP]->bitmap->w - 1;
			box.h = 1;
			box.x = x + widget->wd - 11;
			SDL_FillRect(bltfx->surface, &box, -1);
			box.y += Bitmaps[BITMAP_SLIDER_UP]->bitmap->h - 1;
			SDL_FillRect(bltfx->surface, &box, -1);
		}
	}
	else
	{
		textwin->slider_h = 0;
	}
}

/**
 * Display the message text window, without handling scrollbar/mouse
 * actions.
 * @param x X position.
 * @param y Y position.
 * @param w Maximum width.
 * @param h Maximum height. */
void textwin_show(int x, int y, int w, int h)
{
	widgetdata *widget = cur_widget[MSGWIN_ID];
	textwin_struct *textwin = TEXTWIN(widget);
	SDL_Rect box;
	int scroll;

	box.w = w - 3;
	box.h = 0;
	box.x = 0;
	box.y = 0;
	string_blt(NULL, textwin->font, textwin->entries, 3, 0, COLOR_SIMPLE(COLOR_WHITE), TEXTWIN_TEXT_FLAGS(widget) | TEXT_HEIGHT, &box);
	scroll = box.h / FONT_HEIGHT(textwin->font);

	box.x = x;
	box.y = y;
	box.w = w;
	box.h = h;
	SDL_FillRect(ScreenSurface, &box, 0);
	draw_frame(ScreenSurface, x, y, box.w, box.h);

	box.w = w - 3;
	box.h = h;

	box.y = MAX(0, scroll - (h / FONT_HEIGHT(textwin->font)));

	string_blt(ScreenSurface, textwin->font, textwin->entries, x + 3, y + 1, COLOR_SIMPLE(COLOR_WHITE), TEXTWIN_TEXT_FLAGS(widget) | TEXT_HEIGHT, &box);
}

/**
 * Display widget text windows.
 * @param widget The widget object. */
void widget_textwin_show(widgetdata *widget)
{
	int len;
	SDL_Rect box, box2;
	_BLTFX bltfx;
	textwin_struct *textwin = TEXTWIN(widget);
	int x = widget->x1;
	int y = widget->y1;
	int mx, my;

	/* Sanity check. */
	if (!textwin)
	{
		return;
	}

	box.x = box.y = 0;
	box.w = widget->wd;
	box.h = len = widget->ht;

	/* If we don't have a backbuffer, create it */
	if (!widget->widgetSF)
	{
		/* Need to do this, or the foreground could be semi-transparent too. */
		SDL_SetAlpha(Bitmaps[BITMAP_TEXTWIN_MASK]->bitmap, SDL_SRCALPHA | SDL_RLEACCEL, 255);
		widget->widgetSF = SDL_ConvertSurface(Bitmaps[BITMAP_TEXTWIN_MASK]->bitmap, Bitmaps[BITMAP_TEXTWIN_MASK]->bitmap->format, Bitmaps[BITMAP_TEXTWIN_MASK]->bitmap->flags);
		SDL_SetColorKey(widget->widgetSF, SDL_SRCCOLORKEY | SDL_RLEACCEL, SDL_MapRGB(widget->widgetSF->format, 0, 0, 0));
	}

	if (old_txtwin_alpha != options.textwin_alpha)
	{
		if (txtwinbg)
		{
			SDL_FreeSurface(txtwinbg);
		}

		SDL_SetAlpha(Bitmaps[BITMAP_TEXTWIN_MASK]->bitmap, SDL_SRCALPHA | SDL_RLEACCEL, options.textwin_alpha);
		txtwinbg = SDL_DisplayFormatAlpha(Bitmaps[BITMAP_TEXTWIN_MASK]->bitmap);
		SDL_SetAlpha(Bitmaps[BITMAP_TEXTWIN_MASK]->bitmap, SDL_SRCALPHA | SDL_RLEACCEL, 255);
		old_txtwin_alpha = options.textwin_alpha;

		WIDGET_REDRAW(widget);
	}

	box2.x = x;
	box2.y = y;
	SDL_BlitSurface(txtwinbg, &box, ScreenSurface, &box2);

	if (widget_mouse_event.owner != widget && widget == selection_widget && textwin->selection_start >= 0 && textwin->selection_end >= 0)
	{
		textwin->selection_start = -1;
		textwin->selection_end = -1;
		WIDGET_REDRAW(widget);
	}

	/* Let's draw the widgets in the backbuffer */
	if (widget->redraw)
	{
		widget->redraw = 0;

		SDL_FillRect(widget->widgetSF, NULL, SDL_MapRGBA(widget->widgetSF->format, 0, 0, 0, options.textwin_alpha));

		bltfx.surface = widget->widgetSF;
		bltfx.flags = 0;
		bltfx.alpha = 0;

		show_window(widget, x, y - 2, &bltfx);
	}

	box.x = x;
	box.y = y;
	box2.x = 0;
	box2.y = 0;
	box2.w = widget->wd;
	box2.h = widget->ht;

	SDL_BlitSurface(widget->widgetSF, &box2, ScreenSurface, &box);

	SDL_GetMouseState(&mx, &my);

	if (mx < widget->x1 || mx > widget->x1 + widget->wd || my < widget->y1 || my > widget->y1 + widget->ht)
	{
		textwin->flags &= ~TW_SCROLL;
		widget->resize_flags = 0;
	}

	box.x = x;
	box.y = y;
	box.h = 1;
	box.w = widget->wd;
	SDL_FillRect(ScreenSurface, &box, widget->resize_flags & RESIZE_TOP ? textwin_border_selected : textwin_border);
	box.x = x;
	box.y = y + widget->ht - 1;
	box.h = 1;
	box.w = widget->wd;
	SDL_FillRect(ScreenSurface, &box, widget->resize_flags & RESIZE_BOTTOM ? textwin_border_selected : textwin_border);
	box.x = x;
	box.y = y;
	box.w = 1;
	box.h = widget->ht;
	SDL_FillRect(ScreenSurface, &box, widget->resize_flags & RESIZE_LEFT ? textwin_border_selected : textwin_border);
	box.x = x + widget->wd - 1;
	box.y = y;
	box.w = 1;
	box.h = widget->ht;
	SDL_FillRect(ScreenSurface, &box, widget->resize_flags & RESIZE_RIGHT ? textwin_border_selected : textwin_border);
}

/**
 * Handle text window mouse events.
 * @param event SDL event type.
 * @param WidgetID Widget ID. */
void textwin_event(widgetdata *widget, SDL_Event *event)
{
	textwin_struct *textwin = TEXTWIN(widget);

	if (!textwin)
	{
		return;
	}

	if (event->button.button == SDL_BUTTON_LEFT)
	{
		if (event->type == SDL_MOUSEBUTTONUP)
		{
			textwin->selection_started = 0;
			textwin->flags &= ~TW_SCROLL;
		}
		else if (event->type == SDL_MOUSEBUTTONDOWN)
		{
			textwin->selection_started = 0;
			textwin->selection_start = -1;
			textwin->selection_end = -1;
			selection_widget = widget;
			WIDGET_REDRAW(widget);
		}
		else if (event->type == SDL_MOUSEMOTION)
		{
			WIDGET_REDRAW(widget);
			textwin->selection_started = 1;
		}
	}

	if (event->type == SDL_MOUSEBUTTONDOWN)
	{
		int old_scroll;

		/* Scrolling */
		if (textwin->flags & TW_SCROLL)
		{
			return;
		}

		old_scroll = textwin->scroll;

		if (event->button.button == SDL_BUTTON_WHEELUP)
		{
			textwin->scroll--;
		}
		else if (event->button.button == SDL_BUTTON_WHEELDOWN)
		{
			textwin->scroll++;
		}
		else if (event->button.button == SDL_BUTTON_LEFT)
		{
			/* Clicked scroller button up */
			if (textwin->highlight == TW_HL_UP)
			{
				textwin->scroll--;
			}
			/* Clicked above the slider */
			else if (textwin->highlight == TW_ABOVE)
			{
				textwin->scroll -= TEXTWIN_ROWS_VISIBLE(widget);
			}
			/* Clicked on the slider */
			else if (textwin->highlight == TW_HL_SLIDER)
			{
				textwin->flags |= TW_SCROLL;
				textwin->highlight = TW_HL_SLIDER;
				old_slider_pos = event->motion.y - textwin->slider_y;
				WIDGET_REDRAW(widget);
			}
			/* Clicked under the slider */
			else if (textwin->highlight == TW_UNDER)
			{
				textwin->scroll += TEXTWIN_ROWS_VISIBLE(widget);
			}
			/* Clicked scroller button down */
			else if (textwin->highlight == TW_HL_DOWN)
			{
				textwin->scroll++;
			}
		}

		textwin_scroll_adjust(widget);

		if (textwin->scroll != old_scroll)
		{
			WIDGET_REDRAW(widget);
		}
	}
	else if (event->type == SDL_MOUSEMOTION)
	{
		int old_highlight = textwin->highlight;

		textwin->highlight = TW_HL_NONE;

		/* Highlighting */
		if (event->motion.x > widget->x1 + widget->wd - 11 && event->motion.y > widget->y1 + 2 && event->motion.x < widget->x1 + widget->wd - 2 && event->motion.y < widget->y1 + widget->ht - 2)
		{
			if (event->motion.y < textwin->y + Bitmaps[BITMAP_SLIDER_UP]->bitmap->h + 3)
			{
				textwin->highlight = TW_HL_UP;
			}
			else if (event->motion.y < textwin->y + Bitmaps[BITMAP_SLIDER_UP]->bitmap->h + 3 + textwin->slider_y)
			{
				textwin->highlight = TW_ABOVE;
			}
			else if (event->motion.y < textwin->y + Bitmaps[BITMAP_SLIDER_UP]->bitmap->h + 3 + textwin->slider_y + textwin->slider_h + 3)
			{
				textwin->highlight = TW_HL_SLIDER;
			}
			else if (event->motion.y < widget->y1 + widget->ht - 12)
			{
				textwin->highlight = TW_UNDER;
			}
			else if (event->motion.y < widget->y1 + widget->ht)
			{
				textwin->highlight = TW_HL_DOWN;
			}
		}

		if (textwin->highlight != old_highlight)
		{
			WIDGET_REDRAW(widget);
		}

		/* Slider scrolling */
		if (textwin->flags & TW_SCROLL && event->button.button == SDL_BUTTON_LEFT)
		{
			textwin->slider_y = event->motion.y - old_slider_pos;

			textwin->scroll = TEXTWIN_ROWS_VISIBLE(widget) + MAX(0, textwin->slider_y) * textwin->num_entries / (widget->ht - 20);
			textwin_scroll_adjust(widget);

			WIDGET_REDRAW(widget);
		}
	}
}

/**
 * Change the font used for drawing text in text windows.
 * @param font ID of the font to change to. */
void change_textwin_font(int font)
{
	font = FONT_ARIAL10 + font;

	if (TEXTWIN(cur_widget[MSGWIN_ID])->font != font)
	{
		TEXTWIN(cur_widget[MSGWIN_ID])->font = font;
		textwin_readjust(cur_widget[MSGWIN_ID]);
		WIDGET_REDRAW(cur_widget[MSGWIN_ID]);
	}

	if (TEXTWIN(cur_widget[CHATWIN_ID])->font != font)
	{
		TEXTWIN(cur_widget[CHATWIN_ID])->font = font;
		textwin_readjust(cur_widget[CHATWIN_ID]);
		WIDGET_REDRAW(cur_widget[CHATWIN_ID]);
	}
}

/**
 * The 'Clear' menu action for text windows.
 * @param widget The text window widget. */
void menu_textwin_clear(widgetdata *widget, int x, int y)
{
	textwin_struct *textwin;

	(void) x;
	(void) y;

	textwin = TEXTWIN(widget);
	free(textwin->entries);
	textwin->entries = NULL;
	textwin->num_entries = textwin->entries_size = textwin->scroll = textwin->slider_h = textwin->slider_y = 0;
	WIDGET_REDRAW(widget);
}

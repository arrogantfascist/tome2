/* File: modules.c */

/* Purpose: T-engine modules */

/*
 * Copyright (c) 2003 DarkGod
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"

static void module_reset_dir_aux(cptr *dir, cptr new_path)
{
	char buf[1024];

	/* Build the new path */
	strnfmt(buf, sizeof (buf), "%s%s%s", *dir, PATH_SEP, new_path);

	string_free(*dir);
	*dir = string_make(buf);

	/* Make it if needed */
	if (!private_check_user_directory(*dir))
		quit(format("Unable to create module dir %s\n", *dir));
}

void module_reset_dir(cptr dir, cptr new_path)
{
	cptr *d = 0;
	char buf[1025];

	if (!strcmp(dir, "apex")) d = &ANGBAND_DIR_APEX;
	if (!strcmp(dir, "core")) d = &ANGBAND_DIR_CORE;
	if (!strcmp(dir, "dngn")) d = &ANGBAND_DIR_DNGN;
	if (!strcmp(dir, "data")) d = &ANGBAND_DIR_DATA;
	if (!strcmp(dir, "edit")) d = &ANGBAND_DIR_EDIT;
	if (!strcmp(dir, "file")) d = &ANGBAND_DIR_FILE;
	if (!strcmp(dir, "help")) d = &ANGBAND_DIR_HELP;
	if (!strcmp(dir, "info")) d = &ANGBAND_DIR_INFO;
	if (!strcmp(dir, "scpt")) d = &ANGBAND_DIR_SCPT;
	if (!strcmp(dir, "patch")) d = &ANGBAND_DIR_PATCH;
	if (!strcmp(dir, "pref")) d = &ANGBAND_DIR_PREF;
	if (!strcmp(dir, "xtra")) d = &ANGBAND_DIR_XTRA;
	if (!strcmp(dir, "user")) d = &ANGBAND_DIR_USER;
	if (!strcmp(dir, "note")) d = &ANGBAND_DIR_NOTE;
	if (!strcmp(dir, "cmov")) d = &ANGBAND_DIR_CMOV;
	if (
#ifdef PRIVATE_USER_PATH_APEX
	    !strcmp(dir, "apex") ||
#endif
	    !strcmp(dir, "user") ||
	    !strcmp(dir, "note") ||
	    !strcmp(dir, "cmov"))
	{
		char user_path[1024];
		/* copied from init_file_paths */
		path_parse(user_path, 1024, PRIVATE_USER_PATH);
		strcat(user_path, USER_PATH_VERSION);
		strnfmt(buf, 1024, "%s%s%s", user_path, PATH_SEP, new_path);
		string_free(*d);
		*d = string_make(buf);
	}
#ifdef PRIVATE_USER_PATH_DATA
	else if (!strcmp(dir, "data"))
	{
		module_reset_dir_aux(&ANGBAND_DIR_DATA, new_path);
	}
#endif
	else if (!strcmp(dir, "save"))
	{
		module_reset_dir_aux(&ANGBAND_DIR_SAVE, new_path);
	}
	else
	{
		/* Build the new path */
		strnfmt(buf, 1024, "%s%s%s%s%s", ANGBAND_DIR_MODULES, PATH_SEP, new_path, PATH_SEP, dir);

		string_free(*d);
		*d = string_make(buf);
	}
}

static void dump_modules(int sel, int max)
{
	int i;

	char buf[40], pre = ' ', post = ')';

	char ind;


	for (i = 0; i < max; i++)
	{
		ind = I2A(i % 26);
		if (i >= 26) ind = toupper(ind);

		if (sel == i)
		{
			pre = '[';
			post = ']';
		}
		else
		{
			pre = ' ';
			post = ')';
		}

		strnfmt(buf, 40, "%c%c%c %s", pre, ind, post, modules[i].meta.name);

		if (sel == i)
		{
			print_desc_aux(modules[i].meta.desc, 5, 0);

			c_put_str(TERM_L_BLUE, buf, 10 + (i / 4), 20 * (i % 4));
		}
		else
			put_str(buf, 10 + (i / 4), 20 * (i % 4));
	}
}

static void activate_module(int module_idx)
{
	module_type *module_ptr = &modules[module_idx];

	/* Initialize the module table */
	game_module_idx = module_idx;

	/* Do misc inits  */
	max_plev = module_ptr->max_plev;

	RANDART_WEAPON = module_ptr->randarts.weapon_chance;
	RANDART_ARMOR = module_ptr->randarts.armor_chance;
	RANDART_JEWEL = module_ptr->randarts.jewelry_chance;

	VERSION_MAJOR = module_ptr->meta.version.major;
	VERSION_MINOR = module_ptr->meta.version.minor;
	VERSION_PATCH = module_ptr->meta.version.patch;
	version_major = VERSION_MAJOR;
	version_minor = VERSION_MINOR;
	version_patch = VERSION_PATCH;

	/* Change window name if needed */
	if (strcmp(game_module, "ToME"))
	{
		strnfmt(angband_term_name[0], 79, "T-Engine: %s", game_module);
		Term_xtra(TERM_XTRA_RENAME_MAIN_WIN, 0);
	}

	/* Reprocess the player name, just in case */
	process_player_base();
}

static void init_module(module_type *module_ptr)
{
	/* Set up module directories? */
	cptr dir = module_ptr->meta.module_dir;
	if (dir) {
		module_reset_dir("apex", dir);
		module_reset_dir("core", dir);
		module_reset_dir("data", dir);
		module_reset_dir("dngn", dir);
		module_reset_dir("edit", dir); 
		module_reset_dir("file", dir);
		module_reset_dir("help", dir);
		module_reset_dir("note", dir);
		module_reset_dir("save", dir);
		module_reset_dir("scpt", dir);
		module_reset_dir("user", dir);
		module_reset_dir("pref", dir);
	}
}

bool_ module_savefile_loadable(cptr savefile_mod)
{
	return (strcmp(savefile_mod, modules[game_module_idx].meta.save_file_tag) == 0);
}

/* Did the player force a module on command line */
cptr force_module = NULL;

/* Display possible modules and select one */
bool_ select_module()
{
	s32b k, sel, max;

	/* Init some lua */
	init_lua();

	/* How many modules? */
	max = MAX_MODULES;

	/* No need to bother the player if there is only one module */
	sel = -1;
	if (force_module) {
		/* Find module by name */
		int i=0;
		for (i=0; i<MAX_MODULES; i++) {
			if (strcmp(force_module, modules[i].meta.name) == 0) {
				break;
			}
		}
		if (i<MAX_MODULES) {
			sel = i;
		}
	}
	/* Only a single choice */
	if (max == 1) {
		sel = 0;
	}
	/* No module selected */
	if (sel != -1)
	{
		/* Process the module */
		init_module(&modules[sel]);
		game_module = string_make(modules[sel].meta.name);

		activate_module(sel);

		return FALSE;
	}

	sel = 0;

	/* Preprocess the basic prefs, we need them to have movement keys */
	process_pref_file("pref.prf");

	while (TRUE)
	{
		/* Clear screen */
		Term_clear();

		/* Let the user choose */
		c_put_str(TERM_YELLOW, "Welcome to ToME, you must select a module to play,", 1, 12);
		c_put_str(TERM_YELLOW, "either ToME official module or third party ones.", 2, 13);
		put_str("Press 8/2/4/6 to move, Return to select and Esc to quit.", 4, 3);

		dump_modules(sel, max);

		k = inkey();

		if (k == ESCAPE)
		{
			quit(NULL);
		}
		if (k == '6')
		{
			sel++;
			if (sel >= max) sel = 0;
			continue;
		}
		else if (k == '4')
		{
			sel--;
			if (sel < 0) sel = max - 1;
			continue;
		}
		else if (k == '2')
		{
			sel += 4;
			if (sel >= max) sel = sel % max;
			continue;
		}
		else if (k == '8')
		{
			sel -= 4;
			if (sel < 0) sel = (sel + max - 1) % max;
			continue;
		}
		else if (k == '\r')
		{
			if (sel < 26) k = I2A(sel);
			else k = toupper(I2A(sel));
		}

		{
			int x;

			if (islower(k)) x = A2I(k);
			else x = A2I(tolower(k)) + 26;

			if ((x < 0) || (x >= max)) continue;

			/* Process the module */
			init_module(&modules[x]);
			game_module = string_make(modules[x].meta.name);

			activate_module(x);

			return (FALSE);
		}
	}

	/* Shouldnt happen */
	return (FALSE);
}

static bool_ dleft(byte c, cptr str, int y, int o)
{
	int i = strlen(str);
	int x = 39 - (strlen(str) / 2) + o;
	while (i > 0)
	{
		int a = 0;
		int time = 0;

		if (str[i-1] != ' ')
		{
			while (a < x + i - 1)
			{
				Term_putch(a - 1, y, c, 32);
				Term_putch(a, y, c, str[i-1]);
				time = time + 1;
				if (time >= 4)
				{
					Term_xtra(TERM_XTRA_DELAY, 1);
					time = 0;
				}
				Term_redraw_section(a - 1, y, a, y);
				a = a + 1;

				inkey_scan = TRUE;
				if (inkey()) {
					return TRUE;
				}
			}
		}

		i = i - 1;
	}
	return FALSE;
}

static bool_ dright(byte c, cptr str, int y, int o)
{
	int x = 39 - (strlen(str) / 2) + o;
	int i = 1;
	while (i <= strlen(str))
	{
		int a = 79;
		int time = 0;

		if (str[i-1] != ' ') {
			while (a >= x + i - 1)
			{
				Term_putch(a + 1, y, c, 32);
				Term_putch(a, y, c, str[i-1]);
				time = time + 1;
				if (time >= 4) {
					Term_xtra(TERM_XTRA_DELAY, 1);
					time = 0;
				}
				Term_redraw_section(a, y, a + 1, y);
				a = a - 1;

				inkey_scan = TRUE;
				if (inkey()) {
					return TRUE;
				}
			}
		}

		i = i + 1;
	}
	return FALSE;
}

typedef struct intro_text intro_text;
struct intro_text
{
	bool_ (*drop_func)(byte, cptr, int, int);
	byte color;
	cptr text;
	int y0;
	int x0;
};

static bool_ show_intro(intro_text intro_texts[])
{
	int i = 0;

	Term_clear();
	for (i = 0; ; i++)
	{
		intro_text *it = &intro_texts[i];
		if (it->drop_func == NULL)
		{
			break;
		}
		else if (it->drop_func(it->color, it->text, it->y0, it->x0))
		{
			/* Abort */
			return TRUE;
		}
	}

	/* Wait for key */
	Term_putch(0, 0, TERM_DARK, 32);
	inkey_scan = FALSE;
	inkey();

	/* Continue */
	return FALSE;
}

void tome_intro()
{
	intro_text intro1[] =
	{
		{ dleft , TERM_L_BLUE, "Art thou an adventurer,", 10, 0, },
		{ dright, TERM_L_BLUE, "One who passes through the waterfalls we call danger", 11, -1, },
		{ dleft , TERM_L_BLUE, "to find the true nature of the legends beyond them?", 12, 0, },
		{ dright, TERM_L_BLUE, "If this is so, then seeketh me.", 13, -1, },
		{ dleft , TERM_WHITE , "[Press any key to continue]", 23, -1, },
		{ NULL, }
	};
	intro_text intro2[] =
	{
		{ dleft , TERM_L_BLUE , "DarkGod", 8, 0, },
		{ dright, TERM_WHITE  , "in collaboration with", 9, -1, },
		{ dleft , TERM_L_GREEN, "Eru Iluvatar,", 10, 0, },
		{ dright, TERM_L_GREEN, "Manwe", 11, -1, },
		{ dleft , TERM_WHITE  , "and", 12, 0, },
		{ dright, TERM_L_GREEN, "All the T.o.M.E. contributors(see credits.txt)", 13, -1, },
		{ dleft , TERM_WHITE  , "present", 15, 1, },
		{ dright, TERM_YELLOW , "T.o.M.E.", 16, 0, },
		{ dleft , TERM_WHITE  , "[Press any key to continue]", 23, -1, },
		{ NULL, }
	};

	screen_save();

	/* Intro 1 */
	if (show_intro(intro1))
	{
		goto exit;
	}

	/* Intro 2 */
	if (show_intro(intro2))
	{
		goto exit;
	}

exit:
	screen_load();
}

void theme_intro()
{
	struct intro_text intro1[] =
	{
		{ dleft , TERM_L_BLUE , "Three Rings for the Elven-kings under the sky,", 10, 0, },
		{ dright, TERM_L_BLUE , "Seven for the Dwarf-lords in their halls of stone,", 11, -1, },
		{ dleft , TERM_L_BLUE , "Nine for Mortal Men doomed to die,", 12, 0, },
		{ dright, TERM_L_BLUE , "One for the Dark Lord on his dark throne", 13, -1, },
		{ dleft , TERM_L_BLUE , "In the land of Mordor, where the Shadows lie.", 14, 0, },
		{ dright, TERM_L_BLUE , "One Ring to rule them all, One Ring to find them,", 15, -1, },
		{ dleft , TERM_L_BLUE , "One Ring to bring them all and in the darkness bind them", 16, 0, },
		{ dright, TERM_L_BLUE , "In the land of Mordor, where the Shadows lie.", 17, -1, },
		{ dright, TERM_L_GREEN, "--J.R.R. Tolkien", 18, 0, },
		{ dleft , TERM_WHITE  , "[Press any key to continue]", 23, -1, },
		{ NULL, },
	};
	struct intro_text intro2[] =
	{
		{ dleft , TERM_L_BLUE , "furiosity", 8, 0, },
		{ dright, TERM_WHITE  , "in collaboration with", 9, -1, },
		{ dleft , TERM_L_GREEN, "DarkGod and all the ToME contributors,", 10, 0, },
		{ dright, TERM_L_GREEN, "module creators, t-o-m-e.net forum posters,", 11, -1, },
		{ dleft , TERM_WHITE  , "and", 12, 0, },
		{ dright, TERM_L_GREEN, "by the grace of the Valar", 13, -1, },
		{ dleft , TERM_WHITE  , "present", 15, 1, },
		{ dright, TERM_YELLOW , "Theme (a module for ToME)", 16, 0, },
		{ dleft , TERM_WHITE  , "[Press any key to continue]", 23, -1, },
		{ NULL, },
	};
	
       	screen_save();
 
	/* Intro 1 */
	if (show_intro(intro1))
	{
		goto exit;
	}

	/* Intro 2 */
	if (show_intro(intro2))
	{
		goto exit;
	}

exit:
	screen_load();
}

static bool_ auto_stat_gain_hook(void *data, void *in, void *out)
{
	while (p_ptr->last_rewarded_level * 5 <= p_ptr->lev)
	{
		do_inc_stat(A_STR);
		do_inc_stat(A_INT);
		do_inc_stat(A_WIS);
		do_inc_stat(A_DEX);
		do_inc_stat(A_CON);
		do_inc_stat(A_CHR);

		p_ptr->last_rewarded_level += 1;
	}

	return FALSE;
}

static bool_ drunk_takes_wine(void *data, void *in_, void *out)
{
	hook_give_in *in = (hook_give_in *) in_;
	monster_type *m_ptr = &m_list[in->m_idx];
	object_type *o_ptr = get_object(in->item);

	if ((m_ptr->r_idx == test_monster_name("Singing, happy drunk")) &&
	    (o_ptr->tval == TV_FOOD) &&
	    ((o_ptr->sval == 38) ||
	     (o_ptr->sval == 39)))
	{
		cmsg_print(TERM_YELLOW, "'Hic!'");

		/* Destroy item */
		inc_stack_size_ex(in->item, -1, OPTIMIZE, NO_DESCRIBE);

		/* Create empty bottle */
		{
			object_type forge;
			object_prep(&forge, lookup_kind(TV_BOTTLE,1));
			drop_near(&forge, 50, p_ptr->py, p_ptr->px);
			return TRUE;
		}
	}
	else
	{
		return FALSE;
	}
}

static bool_ hobbit_food(void *data, void *in_, void *out)
{
	hook_give_in *in = (hook_give_in *) in_;
	monster_type *m_ptr = &m_list[in->m_idx];
	object_type *o_ptr = get_object(in->item);

	if ((m_ptr->r_idx == test_monster_name("Scruffy-looking hobbit")) &&
	    (o_ptr->tval == TV_FOOD))
	{
		cmsg_print(TERM_YELLOW, "'Yum!'");

		inc_stack_size_ex(in->item, -1, OPTIMIZE, NO_DESCRIBE);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void init_hooks_module()
{
	/*
	 * Common hooks
	 */
	add_hook_new(HOOK_GIVE,
		     drunk_takes_wine,
		     "drunk_takes_wine",
		     NULL);

	/*
	 * Module-specific hooks
	 */
	switch (game_module_idx)
	{
	case MODULE_TOME:
	{
		break;
	}

	case MODULE_THEME:
	{
		add_hook_new(HOOK_PLAYER_LEVEL,
			     auto_stat_gain_hook,
			     "auto_stat_gain",
			     NULL);

		add_hook_new(HOOK_GIVE,
			     hobbit_food,
			     "hobbit_food",
			     NULL);

		break;
	}

	default:
		assert(FALSE);
	}
}

/*************************************************************************
 *           Atrinik, a Multiplayer Online Role Playing Game             *
 *                                                                       *
 *   Copyright (C) 2009-2014 Alex Tokar and Atrinik Development Team     *
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
 * Atrinik Python plugin object related code.
 *
 * @author Alex Tokar
 */

#include <plugin_python.h>
#include <bresenham.h>
#include <artifact.h>
#include <packet.h>

/**
 * All the possible fields of an object.
 */
static fields_struct fields[] = {
    {"below", FIELDTYPE_OBJECT, offsetof(object, below), FIELDFLAG_READONLY, 0,
            "Object stacked below this one.; Atrinik.Object.Object or None "
            "(readonly)"},
    {"above", FIELDTYPE_OBJECT, offsetof(object, above), FIELDFLAG_READONLY, 0,
            "Object stacked above this one.; Atrinik.Object.Object or None "
            "(readonly)"},
    {"inv", FIELDTYPE_OBJECT, offsetof(object, inv), FIELDFLAG_READONLY, 0,
            "First object in the inventory.; Atrinik.Object.Object or None "
            "(readonly)"},
    {"env", FIELDTYPE_OBJECT, offsetof(object, env), FIELDFLAG_READONLY, 0,
            "Inventory the object is in.; Atrinik.Object.Object or None "
            "(readonly)"},
    {"head", FIELDTYPE_OBJECT, offsetof(object, head), FIELDFLAG_READONLY, 0,
            "Head part of a linked object.; Atrinik.Object.Object or None "
            "(readonly)"},
    {"more", FIELDTYPE_OBJECT, offsetof(object, more), FIELDFLAG_READONLY, 0,
            "Next linked object.; Atrinik.Object.Object or None (readonly)"},
    {"map", FIELDTYPE_MAP, offsetof(object, map), FIELDFLAG_READONLY, 0,
            "Map the object is on.; Atrinik.Map.Map or None (readonly)"},
    {"name", FIELDTYPE_SHSTR, offsetof(object, name), FIELDFLAG_PLAYER_READONLY,
            0, "Name of the object.; str (player readonly)"},
    {"custom_name", FIELDTYPE_SHSTR, offsetof(object, custom_name), 0, 0,
            "Custom name given to the object.; str or None"},
    {"glow", FIELDTYPE_SHSTR, offsetof(object, glow), 0, 0,
            "Glow color, eg, 'ff0000'; str or None"},
    {"title", FIELDTYPE_SHSTR, offsetof(object, title), 0, 0,
            "Title of the object.; str or None"},
    {"race", FIELDTYPE_SHSTR, offsetof(object, race), 0, 0,
            "Race associated with the object.; str or None"},
    {"slaying", FIELDTYPE_SHSTR, offsetof(object, slaying), 0, 0,
            "The slaying field. Used for different purposes, depending on the "
            "object's type.; str or None"},
    {"msg", FIELDTYPE_SHSTR, offsetof(object, msg), 0, 0,
            "The object's story.; str or None"},
    {"artifact", FIELDTYPE_SHSTR, offsetof(object, artifact), 0, 0,
            "Artifact name.; str or None"},
    {"weight", FIELDTYPE_INT32, offsetof(object, weight), 0, 0,
            "Weight of the object in grams.; int"},
    {"count", FIELDTYPE_UINT32, offsetof(object, count), FIELDFLAG_READONLY, 0,
            "Object's unique identifier.; int (readonly)"},

    {"weight_limit", FIELDTYPE_UINT32, offsetof(object, weight_limit), 0, 0,
            "Maximum weight the object's inventory can hold, in grams.; int"},
    {"carrying", FIELDTYPE_INT32, offsetof(object, carrying), 0, 0,
            "Weight the object is currently carrying in its inventory, in "
            "grams.; int"},
    {"path_attuned", FIELDTYPE_UINT32, offsetof(object, path_attuned), 0, 0,
            "Spell paths the object is attuned to.; int"},
    {"path_repelled", FIELDTYPE_UINT32, offsetof(object, path_repelled), 0, 0,
            "Spell paths the object is repelled from.; int"},
    {"path_denied", FIELDTYPE_UINT32, offsetof(object, path_denied), 0, 0,
            "Spell paths the object is denied access to.; int"},
    {"value", FIELDTYPE_INT64, offsetof(object, value), 0, 0,
            "Value of the object.; int"},
    {"nrof", FIELDTYPE_UINT32, offsetof(object, nrof), 0, 0,
            "Amount of objects.; int"},

    {"enemy", FIELDTYPE_OBJECTREF, offsetof(object, enemy),
            FIELDFLAG_PLAYER_READONLY, offsetof(object, enemy_count),
            "Enemy of the object.; Atrinik.Object.Object or None (player "
            "readonly)"},
    {"attacked_by", FIELDTYPE_OBJECTREF, offsetof(object, attacked_by),
            FIELDFLAG_READONLY, offsetof(object, attacked_by_count),
            "Who is attacking the object.; Atrinik.Object.Object or None "
            "(readonly)"},
    {"owner", FIELDTYPE_OBJECTREF, offsetof(object, owner), FIELDFLAG_READONLY,
            offsetof(object, ownercount), "Owner of the object.; "
            "Atrinik.Object.Object or None (readonly)"},

    {"x", FIELDTYPE_INT16, offsetof(object, x), FIELDFLAG_READONLY, 0,
            "If the object is on a :attr:`~Atrinik.Object.Object.map`, X "
            "coordinate on the map the object is on.; int (readonly)"},
    {"y", FIELDTYPE_INT16, offsetof(object, y), FIELDFLAG_READONLY, 0,
            "If the object is on a :attr:`~Atrinik.Object.Object.map`, Y "
            "coordinate on the map the object is on.; int (readonly)"},
    {"attacked_by_distance", FIELDTYPE_INT16,
            offsetof(object, attacked_by_distance), 0, 0,
            "Distance the object was attacked from.; int"},
    {"last_damage", FIELDTYPE_UINT16, offsetof(object, last_damage), 0, 0,
            "Last damage done to the object.; int"},
    {"terrain_type", FIELDTYPE_UINT16, offsetof(object, terrain_type), 0, 0,
            "Terrain type of the object.; int"},
    {"terrain_flag", FIELDTYPE_UINT16, offsetof(object, terrain_flag), 0, 0,
            "Terrains that this object allows its wearer to walk on.; int"},
    {"material", FIELDTYPE_UINT16, offsetof(object, material), 0, 0,
            "What materials this object consists of.; int"},
    {"material_real", FIELDTYPE_INT16, offsetof(object, material_real), 0, 0,
            "Holds the real material value like what kind of steel.; int"},

    {"last_heal", FIELDTYPE_INT16, offsetof(object, last_heal), 0, 0,
            "Last heal value.; int"},
    {"last_sp", FIELDTYPE_INT16, offsetof(object, last_sp), 0, 0,
            "Last mana value.; int"},
    {"last_grace", FIELDTYPE_INT16, offsetof(object, last_grace), 0, 0,
            "Last grace value.; int"},
    {"last_eat", FIELDTYPE_INT16, offsetof(object, last_eat), 0, 0,
            "Last eat value.; int"},

    {"magic", FIELDTYPE_INT8, offsetof(object, magic), 0, 0,
            "Magical bonus of this object.; int"},
    {"state", FIELDTYPE_UINT8, offsetof(object, state), 0, 0,
            "Object's animation state.; int"},
    {"level", FIELDTYPE_INT8, offsetof(object, level),
            FIELDFLAG_PLAYER_READONLY, 0, "Level of the object.; int (player "
            "readonly)"},
    {"direction", FIELDTYPE_INT8, offsetof(object, direction), 0, 0,
            "Direction the object is facing.; int"},
    {"quick_pos", FIELDTYPE_UINT8, offsetof(object, quick_pos), 0, 0,
            "For head object, number of tail parts, for tail part, the tail's "
            "offset.; int"},
    {"quickslot", FIELDTYPE_UINT8, offsetof(object, quickslot),
            FIELDFLAG_READONLY, 0, "Quickslot ID of the object.; "
            "int (readonly)"},

    {"type", FIELDTYPE_UINT8, offsetof(object, type), 0, 0,
            "Type of the object.; int"},
    {"sub_type", FIELDTYPE_UINT8, offsetof(object, sub_type), 0, 0,
            "Object's sub-type.; int"},
    {"item_quality", FIELDTYPE_UINT8, offsetof(object, item_quality), 0, 0,
            "Object's maximum quality.; int"},
    {"item_condition", FIELDTYPE_UINT8, offsetof(object, item_condition), 0, 0,
            "Current object condition.; int"},
    {"item_race", FIELDTYPE_UINT8, offsetof(object, item_race), 0, 0,
            "Item race, eg, orcish, dwarvish, etc.; int"},
    {"item_level", FIELDTYPE_UINT8, offsetof(object, item_level), 0, 0,
            "Level required to use the item.; int"},
    {"item_skill", FIELDTYPE_UINT8, offsetof(object, item_skill), 0, 0,
            "ID of the skill required to use the item.; int"},
    {"glow_radius", FIELDTYPE_INT8, offsetof(object, glow_radius), 0, 0,
            "How much light the object emits.; int"},
    {"move_status", FIELDTYPE_INT8, offsetof(object, move_status), 0, 0,
            "Stage in move move.; int"},
    {"move_type", FIELDTYPE_UINT8, offsetof(object, move_type), 0, 0,
            "What kind of movement the object performs.; int"},

    {"anim_speed", FIELDTYPE_UINT8, offsetof(object, anim_speed), 0, 0,
            "Object's animation speed.; int"},
    {"behavior", FIELDTYPE_UINT8, offsetof(object, behavior), 0, 0,
            "Monster/NPC behavior flags.; int"},
    {"run_away", FIELDTYPE_UINT8, offsetof(object, run_away), 0, 0,
            "Monster runs away if its HP goes below this percentage.; int"},

    {"layer", FIELDTYPE_UINT8, offsetof(object, layer), 0, 0,
            "Layer the object is on.; int"},
    {"sub_layer", FIELDTYPE_UINT8, offsetof(object, sub_layer), 0, 0,
            "Sub-layer the object is on.; int"},
    {"speed", FIELDTYPE_FLOAT, offsetof(object, speed),
            FIELDFLAG_PLAYER_READONLY, 0, "Speed of the object.; "
            "float (player readonly)"},
    {"speed_left", FIELDTYPE_FLOAT, offsetof(object, speed_left), 0, 0,
            "How much speed is left to spend this round.; float"},
    {"weapon_speed", FIELDTYPE_FLOAT, offsetof(object, weapon_speed), 0, 0,
            "Weapon speed.; float"},
    {"weapon_speed_left", FIELDTYPE_FLOAT, offsetof(object, weapon_speed_left),
            0, 0, "Weapon speed left this round.; float"},
    {"exp", FIELDTYPE_INT64, offsetof(object, stats.exp), 0, 0,
            "Experience of the object.; int"},

    {"hp", FIELDTYPE_INT32, offsetof(object, stats.hp), 0, 0,
            "Object's current HP.; int"},
    {"maxhp", FIELDTYPE_INT32, offsetof(object, stats.maxhp),
            FIELDFLAG_PLAYER_READONLY, 0, "Maximum HP of the object.; "
            "int (player readonly)"},
    {"sp", FIELDTYPE_INT16, offsetof(object, stats.sp), 0, 0,
            "Object's current mana points.; int"},
    {"maxsp", FIELDTYPE_INT16, offsetof(object, stats.maxsp),
            FIELDFLAG_PLAYER_READONLY, 0, "Maximum mana points of the object.; "
            "int (player readonly)"},

    {"food", FIELDTYPE_INT16, offsetof(object, stats.food), 0, 0,
            "How much food the object gives when eaten.; int"},
    {"dam", FIELDTYPE_INT16, offsetof(object, stats.dam),
            FIELDFLAG_PLAYER_READONLY, 0, "Damage of the object.; "
            "int (player readonly)"},
    {"wc", FIELDTYPE_INT16, offsetof(object, stats.wc),
            FIELDFLAG_PLAYER_READONLY, 0, "WC attribute of the object.; "
            "int (player readonly)"},
    {"ac", FIELDTYPE_INT16, offsetof(object, stats.ac),
            FIELDFLAG_PLAYER_READONLY, 0, "AC attribute of the object.; "
            "int (player readonly)"},
    {"wc_range", FIELDTYPE_UINT8, offsetof(object, stats.wc_range), 0, 0,
            "WC range attribute.; int"},

    {"Str", FIELDTYPE_INT8, offsetof(object, stats.Str), FIELDFLAG_PLAYER_FIX,
            0, "Strength of the object (or how much it gives when equipped).; "
            "int"},
    {"Dex", FIELDTYPE_INT8, offsetof(object, stats.Dex), FIELDFLAG_PLAYER_FIX,
            0, "Dexterity of the object (or how much it gives when equipped).; "
            "int"},
    {"Con", FIELDTYPE_INT8, offsetof(object, stats.Con), FIELDFLAG_PLAYER_FIX,
            0, "Constitution of the object (or how much it gives when "
            "equipped).; int"},
    {"Wis", FIELDTYPE_INT8, offsetof(object, stats.Wis), FIELDFLAG_PLAYER_FIX,
            0, "Wisdom of the object (or how much it gives when equipped).; "
            "int"},
    {"Cha", FIELDTYPE_INT8, offsetof(object, stats.Cha), FIELDFLAG_PLAYER_FIX,
            0, "Charisma of the object (or how much it gives when equipped).; "
            "int"},
    {"Int", FIELDTYPE_INT8, offsetof(object, stats.Int), FIELDFLAG_PLAYER_FIX,
            0, "Intelligence of the object (or how much it gives when "
            "equipped).; int"},
    {"Pow", FIELDTYPE_INT8, offsetof(object, stats.Pow), FIELDFLAG_PLAYER_FIX,
            0, "Power of the object (or how much it gives when equipped).; "
            "int"},

    {"arch", FIELDTYPE_ARCH, offsetof(object, arch), 0, 0,
            "Archetype of the object.; Atrinik.Archetype.Archetype"},
    {"z", FIELDTYPE_INT16, offsetof(object, z), 0, 0,
            "Z-position on the map (in pixels) for this object.; int"},
    {"zoom_x", FIELDTYPE_INT16, offsetof(object, zoom_x), 0, 0,
            "How much to zoom the object horizontally.; int"},
    {"zoom_y", FIELDTYPE_INT16, offsetof(object, zoom_y), 0, 0,
            "How much to zoom the object vertically.; int"},
    {"rotate", FIELDTYPE_INT16, offsetof(object, rotate), 0, 0,
            "Object's rotation value in degrees.; int"},
    {"align", FIELDTYPE_INT16, offsetof(object, align), 0, 0,
            "X align of the object on the actual rendered map, in "
            "pixels.; int"},
    {"alpha", FIELDTYPE_UINT8, offsetof(object, alpha), 0, 0,
            "Alpha value of the object.; int"},
    {"glow_speed", FIELDTYPE_UINT8, offsetof(object, glow_speed), 0, 0,
            "Glowing animation speed.; int"},
    {"face", FIELDTYPE_FACE, offsetof(object, face), 0, 0,
            "The object's face in a tuple containing the face name as a "
            "string, and the face ID as integer.\nThere are a few different "
            "ways to set object's face. You can use the face name (obj.face = "
            "'eyes.101'), the ID (obj.face = 1000), or the tuple returned by a "
            "previous call to obj.face.; str or int or tuple"},
    {"animation", FIELDTYPE_ANIMATION, offsetof(object, animation_id), 0, 0,
            "Returns the object's animation in a tuple containing the "
            "animation name as string, and the animation ID as integer.\nThere "
            "are a few different ways to set object's animation. You can use "
            "the animation name (obj.animation = 'raas'), the ID"
            "(obj.animation = 100), or the tuple returned by a previous call "
            "to obj.animation.; str or int or tuple"},
    {"inv_animation", FIELDTYPE_ANIMATION, offsetof(object, inv_animation_id),
            0, 0, "Returns the object's inventory animation in a tuple "
            "containing the animation name as string, and the animation ID as"
            "integer.\nThere are a few different ways to set object's inventory"
            " animation. You can use the animation name (obj.inv_animation ="
            "'raas'), the ID (obj.inv_animation = 100), or the tuple returned"
            "by a previous call to obj.inv_animation.; str or int or tuple"},
    {"other_arch", FIELDTYPE_ARCH, offsetof(object, other_arch), 0, 0,
            "Archetype used for various things, depending on the object's "
            "type.; Atrinik.Archetype.Archetype or None"},
    {"connected", FIELDTYPE_CONNECTION, 0, 0, 0,
            "Connection ID. Used to connect together buttons with gates, for "
            "example.; int"},
    {"randomitems", FIELDTYPE_TREASURELIST, offsetof(object, randomitems), 0,
            0, "Treasure list the object generates.; str or None"},
};

/** Documentation for object flag attributes. */
static char *doc_object_flag_names[NUM_FLAGS + 1] = {
    "The object is asleep.",
    "The object is confused.",
    NULL,
    "The object is scared.",
    "The object is blind.",
    "The object is invisible.",
    "The object is ethereal.",
    "The object is good aligned.",
    "The object cannot be picked up.",
    "The object generates an event when it's walked upon.",
    "The object blocks passage.",
    "The object is animated.",
    "The object slows movement when moving through it.",
    "The object is flying.",
    "The object is a monster.",
    "The object is friendly.",
    NULL,
    "The object has been applied before.",
    "Automatically does something when loaded onto a map, such as shop floors "
            " that generate random treasure.",
    NULL,
    "The object is neutrally aligned.",
    "The object can see invisible objects.",
    "The object can be pushed.",
    "When the object is triggered, its connection state is immediately reset.",
    "The object can turn.",
    "The object generates an event when something walks off of it.",
    "The object generates an event when it's flown upon.",
    "The object generates an event when something flies off of it.",
    "The object disappears when its :attr:`Atrinik.Object.Object.food` "
            "attribute reaches zero.",
    "The object is identified.",
    "The object reflects off of surfaces.",
    "The object is changing.",
    "The object can split into parts.",
    "The object hits back immediately upon being hit.",
    "The object disappears when it's dropped.",
    "The object blocks line of sight.",
    "The object is undead.",
    "The object can be stacked.",
    "The object is not aggressive.",
    "The object reflects missile projectiles.",
    "The object reflects spell projectiles.",
    "The object blocks magic use.",
    "Used to disable object updates.",
    "The object is evil aligned.",
    NULL,
    "The object runs away when its HP gets low enough.",
    "The object allows passage for objects with "
            ":attr:`~Atrinik.Object.Object.f_can_pass_thru` set, even if it "
            "otherwise normally blocks passage.",
    "The object can pass through blocked objects with "
            ":attr:`~Atrinik.Object.Object.f_pass_thru` set.",
    "Outdoor tile.",
    "The object is unique.",
    "The object cannot be dropped.",
    "The object cannot be damaged.",
    "The object can cast spells.",
    NULL,
    "The object requires two hands to wield.",
    "The object can use bows.",
    "The object can use armour.",
    "The object can use weapons.",
    "The object's connection is not activated when 'pushed'.",
    "The object's connection is not activated when 'released'.",
    "The object has a readied bow.",
    "The object has (and/or gives) x-ray vision.",
    NULL,
    "The object is a floor.",
    "The object saves a player's life once, then destructs itself.",
    "The object is magical.",
    NULL,
    "The object will not move.",
    "The object will move randomly.",
    "The object will evaporate if it has no enemy.",
    NULL,
    "The object is in stealth and can pass more quietly past monsters, with "
            "smaller chance of being spotted.",
    NULL, NULL,
    "The object is cursed.",
    "The object is damned (*very* cursed).",
    "The object can be built upon.",
    "The object disallows PvP.",
    NULL, NULL,
    "The object can be thrown.",
    NULL, NULL,
    "The object is a male.",
    "The object is a female.",
    "The object is currently applied.",
    "The object is locked and cannot be dropped.",
    NULL, NULL, NULL,
    "The object has a weapon ready.",
    "The object won't give experience for using skills with it.",
    NULL,
    "The object can see even in darkness.",
    "The object is a cauldron.",
    "The object is a powder.",
    NULL,
    "The object hits once, then evaporates.",
    "Always draw the object twice.",
    "The object is in a rage and will attack friends as well.",
    "The object will never attack.",
    "The object cannot be killed, and enemies will not consider it for "
            "attacking.",
    "The object is a quest item.",
    "The object is trapped.",
    NULL, NULL, NULL, NULL, NULL, NULL,
    "The object is a system object.",
    "The object always teleports items exactly on the coordinates it leads to.",
    "The object hasn't been paid for yet.",
    "The object cannot be seen, ever.",
    "The object makes its wearer invisible.",
    "The object makes its wearer ethereal.",
    "The object is a player.",
    "The object is named.",
    NULL,
    "The object will not be teleported by teleporters.",
    "The object will drop its corpse when killed.",
    "The object will *always* drop its corpse when killed.",
    "Only players can enter a tile that has an object with this flag set.",
    NULL,
    "The object is a one-drop item.",
    "The object is permanently cursed.",
    "The object is permanently damned.",
    "The object is a closed door.",
    "The object is a spell.",
    "The object is a missile.",
    "The object is shown based on its direction and the player's position. ",
    "The object does even more damage to the race specified in the "
            ":attr:`~Atrinik.Object.Object.slaying` attribute.",
    NULL,
    "The object was moved. Used internally.",
    "The object won't be saved.",
    NULL,
};

/** Documentation for Atrinik_Object_ActivateRune(). */
static const char doc_Atrinik_Object_ActivateRune[] =
".. method:: ActivateRune(who).\n\n"
"Activate a rune.\n\n"
":param who: Who should be affected by the effects of the rune.\n"
":type who: :class:`Atrinik.Object.Object`\n"
":raises TypeError: If self is not of type :attr:`Atrinik.Type.RUNE`";

/**
 * Implements Atrinik.Object.Object.ActivateRune() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_ActivateRune(Atrinik_Object *obj,
        PyObject *args)
{
    Atrinik_Object *who;

    if (!PyArg_ParseTuple(args, "O!", &Atrinik_ObjectType, &who)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);
    OBJEXISTCHECK(who);

    if (obj->obj->type != RUNE) {
        PyErr_SetString(PyExc_TypeError, "self is not a rune.");
        return NULL;
    }

    hooks->rune_spring(obj->obj, who->obj);

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_TeleportTo(). */
static const char doc_Atrinik_Object_TeleportTo[] =
".. method:: TeleportTo(path, x=0, y=0).\n\n"
"Teleports the object to the specified coordinates on a map.\n\n"
":param path: The map path.\n"
":type path: :attr:`str`\n"
":param x: X coordinate on the map.\n"
":type x: :attr:`int`\n"
":param y: Y coordinate on the map.\n"
":type y: :attr:`int`";

/**
 * Implements Atrinik.Object.Object.TeleportTo() Python method.
 * @copydoc PyMethod_VARARGS_KEYWORDS
 */
static PyObject *Atrinik_Object_TeleportTo(Atrinik_Object *obj, PyObject *args,
        PyObject *keywds)
{
    static char *kwlist[] = {"path", "x", "y", NULL};
    const char *path;
    int x, y;
    mapstruct *m;

    x = y = 0;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|ii", kwlist, &path, &x,
            &y)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);
    m = hooks->ready_map_name(path, NULL, 0);

    if (!m) {
        PyErr_Format(AtrinikError, "Could not load map %s.", path);
        return NULL;
    }

    hooks->object_enter_map(obj->obj, NULL, m, x, y, 1);

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_InsertInto(). */
static const char doc_Atrinik_Object_InsertInto[] =
".. method:: InsertInto(where).\n\n"
"Inserts the object into some other object.\n\n"
":param where: Where to insert the object.\n"
":type where: :class:`Atrinik.Object.Object`\n"
":returns: The inserted object, which may be different from the original (due"
"to merging, for example). None is returned on failure.\n"
":rtype: class:`Atrinik.Object.Object` or :attr:`None`";

/**
 * Implements Atrinik.Object.Object.InsertInto() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_InsertInto(Atrinik_Object *obj, PyObject *args)
{
    Atrinik_Object *where;

    if (!PyArg_ParseTuple(args, "O!", &Atrinik_ObjectType, &where)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);
    OBJEXISTCHECK(where);

    if (!QUERY_FLAG(obj->obj, FLAG_REMOVED)) {
        hooks->object_remove(obj->obj, 0);
    }

    object *ret = hooks->insert_ob_in_ob(obj->obj, where->obj);
    if (ret == NULL) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    return wrap_object(ret);
}

/** Documentation for Atrinik_Object_Apply(). */
static const char doc_Atrinik_Object_Apply[] =
".. method:: Apply(what, flags=Atrinik.APPLY_NORMAL).\n\n"
"Makes the object apply the specified object.\n\n"
":param what: What object to apply.\n"
":type what: :class:`Atrinik.Object.Object`\n"
":param flags: Reasonable combination of :attr:`~Atrinik.APPLY_NORMAL`, "
":attr:`~Atrinik.APPLY_ALWAYS`, :attr:`~Atrinik.APPLY_ALWAYS_UNAPPLY`, "
":attr:`~Atrinik.APPLY_NO_MERGE`, :attr:`~Atrinik.APPLY_IGNORE_CURSE`, "
":attr:`~Atrinik.APPLY_NO_EVENT`.\n"
":type flags: :attr:`int`\n"
":returns: One of OBJECT_METHOD_xxx, eg, :attr:`~Atrinik.OBJECT_METHOD_OK`.\n"
":rtype: :attr:`int`";

/**
 * Implements Atrinik.Object.Object.Apply() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_Apply(Atrinik_Object *obj, PyObject *args)
{
    Atrinik_Object *what;
    int flags = 0;

    if (!PyArg_ParseTuple(args, "O!|i", &Atrinik_ObjectType, &what, &flags)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);
    OBJEXISTCHECK(what);

    return Py_BuildValue("i", hooks->manual_apply(obj->obj, what->obj, flags));
}

/** Documentation for Atrinik_Object_Take(). */
static const char doc_Atrinik_Object_Take[] =
".. method:: Take(what).\n\n"
"Forces the object to pick up the specified object.\n\n"
":param what: What object to pick up. Can be a string instead, in which case "
"it's equivalent of the /take command.\n"
":type what: :class:`Atrinik.Object.Object` or :attr:`str`";

/**
 * Implements Atrinik.Object.Object.Take() Python method.
 * @copydoc PyMethod_OBJECT
 */
static PyObject *Atrinik_Object_Take(Atrinik_Object *obj, PyObject *what)
{
    OBJEXISTCHECK(obj);

    if (PyObject_TypeCheck(what, &Atrinik_ObjectType)) {
        OBJEXISTCHECK((Atrinik_Object *) what);
        hooks->pick_up(obj->obj, ((Atrinik_Object *) what)->obj, 0);
    } else if (PyString_Check(what)) {
        hooks->command_take(obj->obj, "take", PyString_AsString(what));
    } else {
        PyErr_SetString(PyExc_TypeError,
                "Argument 'what' must be either Atrinik object or string.");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_Drop(). */
static const char doc_Atrinik_Object_Drop[] =
".. method:: Drop(what).\n\n"
"Forces the object to drop the specified object.\n\n"
":param what: What object to drop. Can be a string instead, in which case "
"it's equivalent of the /drop command.\n"
":type what: :class:`Atrinik.Object.Object` or :attr:`str`";

/**
 * Implements Atrinik.Object.Object.Drop() Python method.
 * @copydoc PyMethod_OBJECT
 */
static PyObject *Atrinik_Object_Drop(Atrinik_Object *obj, PyObject *what)
{
    OBJEXISTCHECK(obj);

    if (PyObject_TypeCheck(what, &Atrinik_ObjectType)) {
        OBJEXISTCHECK((Atrinik_Object *) what);
        hooks->drop(obj->obj, ((Atrinik_Object *) what)->obj, 0);
    } else if (PyString_Check(what)) {
        hooks->command_drop(obj->obj, "drop", PyString_AsString(what));
    } else {
        PyErr_SetString(PyExc_TypeError,
                "Argument 'what' must be either Atrinik object or string.");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_Say(). */
static const char doc_Atrinik_Object_Say[] =
".. method:: Say(message).\n\n"
"Makes the object object say a message to everybody in range.\n\n"
":param message: The message to say.\n"
":type message: :attr:`str`";

/**
 * Implements Atrinik.Object.Object.Say() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_Say(Atrinik_Object *obj, PyObject *args)
{
    const char *message;
    char buf[HUGE_BUF];

    if (!PyArg_ParseTuple(args, "s", &message)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    snprintf(VS(buf), "%s says: %s", hooks->query_name(obj->obj, NULL),
            message);
    hooks->draw_info_map(CHAT_TYPE_GAME, NULL, COLOR_NAVY, obj->obj->map,
            obj->obj->x, obj->obj->y, MAP_INFO_NORMAL, NULL, NULL, buf);

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_GetGender(). */
static const char doc_Atrinik_Object_GetGender[] =
".. method:: GetGender().\n\n"
"Acquire object's gender.\n\n"
":returns: One of the gender constants defined in :mod:`Atrinik.Gender`.\n"
":rtype: :attr:`int`";

/**
 * Implements Atrinik.Object.Object.GetGender() Python method.
 * @copydoc PyMethod_NOARGS
 */
static PyObject *Atrinik_Object_GetGender(Atrinik_Object *obj)
{
    OBJEXISTCHECK(obj);

    return Py_BuildValue("i", hooks->object_get_gender(obj->obj));
}

/** Documentation for Atrinik_Object_SetGender(). */
static const char doc_Atrinik_Object_SetGender[] =
".. method:: SetGender(gender).\n\n"
"Set object's gender.\n\n"
":param gender: The gender to set. One of the gender constants defined in "
":mod:`Atrinik.Gender`.\n"
":type gender: :attr:`int`";

/**
 * Implements Atrinik.Object.Object.SetGender() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_SetGender(Atrinik_Object *obj, PyObject *args)
{
    int gender;

    if (!PyArg_ParseTuple(args, "i", &gender)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    /* Set object to neuter */
    CLEAR_FLAG(obj->obj, FLAG_IS_MALE);
    CLEAR_FLAG(obj->obj, FLAG_IS_FEMALE);

    if (gender == GENDER_MALE || gender == GENDER_HERMAPHRODITE) {
        SET_FLAG(obj->obj, FLAG_IS_MALE);
    }

    if (gender == GENDER_FEMALE || gender == GENDER_HERMAPHRODITE) {
        SET_FLAG(obj->obj, FLAG_IS_FEMALE);
    }

    /* Update the player's client if object was a player. */
    if (obj->obj->type == PLAYER) {
        CONTR(obj->obj)->socket.ext_title_flag = 1;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_Update(). */
static const char doc_Atrinik_Object_Update[] =
".. method:: Update().\n\n"
"Recalculate player's or monster's stats depending on equipment, forces, "
"skills, etc.";

/**
 * Implements Atrinik.Object.Object.Update() Python method.
 * @copydoc PyMethod_NOARGS
 */
static PyObject *Atrinik_Object_Update(Atrinik_Object *obj)
{
    OBJEXISTCHECK(obj);

    hooks->living_update(obj->obj);

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_Hit(). */
static const char doc_Atrinik_Object_Hit[] =
".. method:: Hit(target, damage).\n\n"
"Makes the object hit the target object for the specified amount of damage.\n\n"
":param target: The target object to hit.\n"
":type target: :class:`Atrinik.Object.Object`\n"
":param damage: How much damage to deal. If -1, the target object will be "
"killed, otherwise the actual damage done is calculated depending on the "
"object's attack types, the target's protections, etc.\n"
":type damage: :attr:`int`\n"
":raises ValueError: If the target is not on a map or is not alive.";

/**
 * Implements Atrinik.Object.Object.Hit() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_Hit(Atrinik_Object *obj, PyObject *args)
{
    Atrinik_Object *target;
    int damage;

    if (!PyArg_ParseTuple(args, "O!i", &Atrinik_ObjectType, &target, &damage)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);
    OBJEXISTCHECK(target);

    /* Cannot kill objects that are not alive or on map. */
    if (!target->obj->map || !IS_LIVE(target->obj)) {
        PyErr_SetString(PyExc_ValueError, "Invalid object to hit/kill.");
        return NULL;
    }

    /* Kill the target. */
    if (damage == -1) {
        hooks->kill_object(target->obj, obj->obj);
    } else {
        /* Do damage. */
        hooks->hit_player(target->obj, damage, obj->obj);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_Cast(). */
static const char doc_Atrinik_Object_Cast[] =
".. method:: Hit(spell, target=None, mode=-1, direction=0, option=None).\n\n"
"Cast the specified spell.\n\n"
":param spell: ID of the spell to cast.\n"
":type spell: :attr:`int`\n"
":param target: Target object for spells that require a valid target.\n"
":type target: :class:`Atrinik.Object.Object` or :attr:`None`\n"
":param mode: One of the CAST_xxx constants defined in :mod:`Atrinik`, eg, "
":attr:`~Atrinik.CAST_NORMAL`. If -1, will try to figure out the appropriate "
"mode automatically.\n"
":type mode: :attr:`int`\n"
":param direction: The direction to cast the spell in.\n"
":type direction: :attr:`int`\n"
":param option: Additional string option, required by some spells (create food "
"for example).\n"
":type option: :attr:`str` or :attr:`None`";

/**
 * Implements Atrinik.Object.Object.Cast() Python method.
 * @copydoc PyMethod_VARARGS_KEYWORDS
 */
static PyObject *Atrinik_Object_Cast(Atrinik_Object *obj, PyObject *args,
        PyObject *keywds)
{
    static char *kwlist[] = {"target", "spell", "mode", "direction", "option",
            NULL};
    Atrinik_Object *target = NULL;
    int spell, direction = 0, mode = -1;
    const char *option = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "i|O!iis", kwlist, &spell,
            &Atrinik_ObjectType, &target, &mode, &direction, &option)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    if (target != NULL) {
        OBJEXISTCHECK(target);
    }

    /* Figure out the mode automatically. */
    if (mode == -1) {
        if (obj->obj->type != PLAYER) {
            mode = CAST_NPC;
        } else {
            mode = CAST_NORMAL;
        }
    } else if (mode == CAST_NORMAL && target != NULL && target != obj &&
            obj->obj->type != PLAYER) {
        /* Ensure the mode is valid. */
        mode = CAST_NPC;
    }

    hooks->cast_spell(target != NULL ? target->obj : obj->obj, obj->obj,
            direction, spell, 1, mode, option);

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_CreateForce(). */
static const char doc_Atrinik_Object_CreateForce[] =
".. method:: CreateForce(name, time=0).\n\n"
"Create a force object in object's inventory.\n\n"
":param name: ID of the force object.\n"
":type name: :attr:`str`\n"
":param time: If non-zero, the force will be removed again after time / 0.02 "
"ticks.\n"
":type time: :attr:`int`\n"
":returns: The created force object.\n"
":rtype: :class:`Atrinik.Object.Object`";

/**
 * Implements Atrinik.Object.Object.CreateForce() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_CreateForce(Atrinik_Object *obj, PyObject *args)
{
    const char *name;
    int expire_time = 0;
    object *force;

    if (!PyArg_ParseTuple(args, "s|i", &name, &expire_time)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    force = hooks->arch_get("force");

    if (expire_time > 0) {
        SET_FLAG(force, FLAG_IS_USED_UP);
        force->stats.food = expire_time;
        force->speed = 0.02f;
    } else {
        force->speed = 0.0;
    }

    hooks->update_ob_speed(force);
    FREE_AND_COPY_HASH(force->name, name);

    return wrap_object(hooks->insert_ob_in_ob(force, obj->obj));
}

/** Documentation for Atrinik_Object_CreateObject(). */
static const char doc_Atrinik_Object_CreateObject[] =
".. method:: CreateObject(archname, nrof=1, value=-1, identified=True).\n\n"
"Creates a new object from archname and inserts it into the object.\n\n"
":param archname: Name of the arch to create.\n"
":type archname: :attr:`str`\n"
":param nrof: Number of objects to create.\n"
":type nrof: :attr:`int`\n"
":param value: If not -1, will be used as value for the new object.\n"
":type value: :attr:`int`\n"
":param identified: If False, the object will not be identified.\n"
":type identified: :attr:`bool`\n"
":returns: The created (and inserted) object, None on failure.\n"
":rtype: :class:`Atrinik.Object.Object` or :attr:`None`\n"
":raises Atrinik.AtrinikError: If archname references an invalid "
"archetype.";

/**
 * Implements Atrinik.Object.Object.CreateObject() Python method.
 * @copydoc PyMethod_VARARGS_KEYWORDS
 */
static PyObject *Atrinik_Object_CreateObject(Atrinik_Object *obj,
        PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"archname", "nrof", "value", "identified", NULL};
    const char *archname;
    uint32_t nrof = 1;
    int64_t value = -1;
    int identified = 1;
    archetype_t *at;
    object *tmp;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|ILi", kwlist, &archname,
            &nrof, &value, &identified)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    at = hooks->arch_find(archname);

    if (at == NULL) {
        PyErr_Format(AtrinikError, "The archetype '%s' doesn't exist.",
                archname);
        return NULL;
    }

    tmp = hooks->arch_to_object(at);

    if (value != -1) {
        tmp->value = value;
    }

    if (nrof > 1) {
        tmp->nrof = nrof;
    }

    if (identified) {
        SET_FLAG(tmp, FLAG_IDENTIFIED);
    }

    tmp = hooks->insert_ob_in_ob(tmp, obj->obj);

    return wrap_object(tmp);
}

/** @cond */

/**
 * Helper function for Atrinik_Object_FindObject() to recursively
 * check inventories. */
static object *object_find_object(object *tmp, int mode, shstr *archname,
        shstr *name, shstr *title, int type, PyObject *list, bool unpaid)
{
    for ( ; tmp != NULL; tmp = tmp->below) {
        if ((archname == NULL || tmp->arch->name == archname) &&
                (name == NULL || tmp->name == name) &&
                (title == NULL || tmp->title == title) &&
                (type == -1 || tmp->type == type) &&
                (!unpaid || QUERY_FLAG(tmp, FLAG_UNPAID))) {
            if (list != NULL) {
                PyList_Append(list, wrap_object(tmp));
            } else {
                return tmp;
            }
        }

        if (tmp->inv != NULL && (mode == INVENTORY_ALL ||
                (mode == INVENTORY_CONTAINERS && tmp->type == CONTAINER))) {
            object *tmp2 = object_find_object(tmp->inv, mode, archname, name,
                    title, type, list, unpaid);
            if (tmp2 != NULL) {
                return tmp2;
            }
        }
    }

    return NULL;
}
/** @endcond */

/** Documentation for Atrinik_Object_FindObject(). */
static const char doc_Atrinik_Object_FindObject[] =
".. method:: FindObject(mode=Atrinik.INVENTORY_ONLY, archname=None, name=None, "
"title=None, type=-1, multiple=False, unpaid=False).\n\n"
"Looks for a certain object in object's inventory.\n\n"
":param mode: How to search the inventory. One of the INVENTORY_xxx constants "
"defined in the :mod:`Atrinik` module, eg, :attr:`~Atrinik.INVENTORY_ALL`.\n"
":type mode: :attr:`int`\n"
":param archname: Arch name of the object to search for. If None, can be any.\n"
":type archname: :attr:`str` or :attr:`None`\n"
":param name: Name of the object. If None, can be any.\n"
":type name: :attr:`str` or :attr:`None`\n"
":param title: Title of the object. If None, can be any.\n"
":type title: :attr:`str` or :attr:`None`\n"
":param type: Type of the object. If -1, can be any.\n"
":type type: :attr:`int`\n"
":param multiple: If True, the return value will be a list of all matching "
"objects, instead of just the first one found.\n"
":type multiple: :attr:`bool`\n"
":param unpaid: Only match unpaid objects.\n"
":type unpaid: :attr:`bool`\n"
":returns: The object we wanted if found, None (or an empty list) otherwise\n"
":rtype: :class:`Atrinik.Object.Object` or :attr:`None` or :attr:`list`\n"
":raises ValueError: If there were no conditions to search for.";

/**
 * Implements Atrinik.Object.Object.CreateObject() Python method.
 * @copydoc PyMethod_VARARGS_KEYWORDS
 */
static PyObject *Atrinik_Object_FindObject(Atrinik_Object *obj, PyObject *args,
        PyObject *keywds)
{
    static char *kwlist[] = {"mode", "archname", "name", "title", "type",
            "multiple", "unpaid", NULL};
    uint8_t mode = INVENTORY_ONLY;
    int type = -1, multiple = 0, unpaid = 0;
    const char *archname = NULL, *name = NULL, *title = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|bzzziii", kwlist, &mode,
            &archname, &name, &title, &type, &multiple, &unpaid)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    if (archname == NULL && name == NULL && title == NULL && type == -1 &&
            !unpaid) {
        PyErr_SetString(PyExc_ValueError, "No conditions to search for given.");
        return NULL;
    }

    PyObject *list = multiple ? PyList_New(0) : NULL;

    /* Try to find the strings we got from Python in the shared strings
     * library. If they are not found, it is impossible that the inventory
     * lookups succeed. */

    shstr *archname_sh = NULL;
    if (archname != NULL) {
        archname_sh = hooks->find_string(archname);
        if (archname_sh == NULL) {
            goto done;
        }
    }

    shstr *name_sh = NULL;
    if (name != NULL) {
        name_sh = hooks->find_string(name);
        if (name_sh == NULL) {
            goto done;
        }
    }

    shstr *title_sh = NULL;
    if (title != NULL) {
        title_sh = hooks->find_string(title);
        if (title_sh == NULL) {
            goto done;
        }
    }

    object *match = object_find_object(obj->obj->inv, mode, archname_sh,
            name_sh, title_sh, type, list, unpaid);
    if (match != NULL) {
        return wrap_object(match);
    }

done:
    if (multiple) {
        return list;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_Remove(). */
static const char doc_Atrinik_Object_Remove[] =
".. method:: Remove().\n\n"
"Takes the object out of whatever map or inventory it is in. The object can "
"then be inserted or teleported somewhere else.\n\n"
"Be careful when removing one of the objects involved in the event activation "
"(such as the activator/event/etc). It is recommended you use :meth:"
"`Atrinik.SetReturnValue(1)` or similar before the script exits if doing so.";

/**
 * Implements Atrinik.Object.Object.Remove() Python method.
 * @copydoc PyMethod_NOARGS
 */
static PyObject *Atrinik_Object_Remove(Atrinik_Object *obj)
{
    OBJEXISTCHECK(obj);
    hooks->object_remove(obj->obj, 0);

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_Destroy(). */
static const char doc_Atrinik_Object_Destroy[] =
".. method:: Destroy().\n\n"
"Frees all data associated with the object.";

/**
 * Implements Atrinik.Object.Object.Destroy() Python method.
 * @copydoc PyMethod_NOARGS
 */
static PyObject *Atrinik_Object_Destroy(Atrinik_Object *obj)
{
    OBJEXISTCHECK(obj);

    if (!QUERY_FLAG(obj->obj, FLAG_REMOVED)) {
        hooks->object_remove(obj->obj, 0);
    }

    hooks->object_destroy(obj->obj);

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_SetPosition(). */
static const char doc_Atrinik_Object_SetPosition[] =
".. method:: SetPosition().\n\n"
"Sets new position coordinates for the object.\n\nCannot be used to move "
"objects out of containers, use :meth:`~Atrinik.Drop` or :meth:"
"`~Atrinik.TeleportTo` for that."
":param x: New X position on the same map.\n"
":type x: :attr:`int`\n"
":param Y: New Y position on the same map.\n"
":type y: :attr:`int`";

/**
 * Implements Atrinik.Object.Object.SetPosition() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_SetPosition(Atrinik_Object *obj, PyObject *args)
{
    int x, y;

    if (!PyArg_ParseTuple(args, "ii", &x, &y)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    hooks->transfer_ob(obj->obj, x, y, 0, NULL, NULL);

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_CastIdentify(). */
static const char doc_Atrinik_Object_CastIdentify[] =
".. method:: CastIdentify(target, mode, marked=None).\n\n"
"Cast identify on target.\n\n"
":param target: The target object.\n"
":type target: :class:`Atrinik.Object.Object`\n"
":param mode: One of IDENTIFY_xxx, eg, :attr:`~Atrinik.IDENTIFY_NORMAL`.\n"
":type mode: :attr:`int`\n"
":param marked: Marked item.\n"
":type marked: :class:`Atrinik.Object.Object` or :attr:`None`";

/**
 * Implements Atrinik.Object.Object.CastIdentify() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_CastIdentify(Atrinik_Object *obj,
        PyObject *args)
{
    Atrinik_Object *target;
    PyObject *marked = NULL;
    object *ob = NULL;
    int mode;

    if (!PyArg_ParseTuple(args, "O!i|O", &Atrinik_ObjectType, &target, &mode,
            &marked)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);
    OBJEXISTCHECK(target);

    if (marked && marked != Py_None) {
        if (!PyObject_TypeCheck(marked, &Atrinik_ObjectType)) {
            PyErr_SetString(PyExc_TypeError, "Must be Atrinik.Object");
            return NULL;
        }

        OBJEXISTCHECK((Atrinik_Object *) marked);
        ob = ((Atrinik_Object *) marked)->obj;
    }

    hooks->cast_identify(target->obj, obj->obj->level, ob, mode);
    hooks->play_sound_map(obj->obj->map, CMD_SOUND_EFFECT,
            hooks->spells[SP_IDENTIFY].sound, obj->obj->x, obj->obj->y, 0, 0);

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_Save(). */
static const char doc_Atrinik_Object_Save[] =
".. method:: Save().\n\n"
"Dump an object, as if it was being saved to map or player file. Useful for "
"saving the object somewhere for loading later with :meth:"
"`~Atrinik.Object.Object.Save`.";

/**
 * Implements Atrinik.Object.Object.Save() Python method.
 * @copydoc PyMethod_NOARGS
 */
static PyObject *Atrinik_Object_Save(Atrinik_Object *obj)
{
    OBJEXISTCHECK(obj);

    StringBuffer *sb = hooks->stringbuffer_new();
    hooks->dump_object_rec(obj->obj, sb);
    char *result = hooks->stringbuffer_finish(sb);
    PyObject *ret = Py_BuildValue("s", result);
    efree(result);

    return ret;
}

/** Documentation for Atrinik_Object_GetCost(). */
static const char doc_Atrinik_Object_GetCost[] =
".. method:: GetCost(flag=Atrinik.COST_TRUE).\n\n"
"Get cost of an object in integer value.\n\n"
":param flag: One of the COST_xxx constants, eg, :attr:`~Atrinik.COST_BUY`.\n"
":type flag: :attr:`int`\n"
":returns: The cost of the item.\n"
":rtype: :attr:`int`";

/**
 * Implements Atrinik.Object.Object.GetCost() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_GetCost(Atrinik_Object *obj, PyObject *args)
{
    int flag = COST_TRUE;

    if (!PyArg_ParseTuple(args, "|i", &flag)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    return Py_BuildValue("L", hooks->query_cost(obj->obj, flag));
}

/** Documentation for Atrinik_Object_GetMoney(). */
static const char doc_Atrinik_Object_GetMoney[] =
".. method:: GetMoney().\n\n"
"Get all the money the object is carrying as integer.\n\n"
"Can only be used on player or container objects.\n\n"
":returns: The amount of money the object is carrying.\n"
":rtype: :attr:`int`";

/**
 * Implements Atrinik.Object.Object.GetMoney() Python method.
 * @copydoc PyMethod_NOARGS
 */
static PyObject *Atrinik_Object_GetMoney(Atrinik_Object *obj)
{
    OBJEXISTCHECK(obj);

    return Py_BuildValue("L", hooks->query_money(obj->obj));
}

/** Documentation for Atrinik_Object_PayAmount(). */
static const char doc_Atrinik_Object_PayAmount[] =
".. method:: PayAmount(value).\n\n"
"Makes the object pay a specified amount of money.\n\n"
":param value: The amount of money to pay.\n"
":type value: :attr:`int`"
":returns: Whether the value was paid successfully (had enough money).\n"
":rtype: :attr:`bool`";

/**
 * Implements Atrinik.Object.Object.PayAmount() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_PayAmount(Atrinik_Object *obj, PyObject *args)
{
    int64_t value;

    if (!PyArg_ParseTuple(args, "L", &value)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    Py_ReturnBoolean(hooks->pay_for_amount(value, obj->obj));
}

/** Documentation for Atrinik_Object_Clone(). */
static const char doc_Atrinik_Object_Clone[] =
".. method:: Clone(inventory=True).\n\n"
"Clone an object.\nGenerally, you should do something with the clone.\n\n"
":meth:`~Atrinik.Object.Object.TeleportTo` or "
":meth:`~Atrinik.Object.Object.InsertInto` are useful methods for that.\n\n"
":param inventory: Whether to clone the inventory of the object.\n"
":type inventory: :attr:`bool`\n"
":returns: Cloned object.\n"
":rtype: :class:`Atrinik.Object.Object`";

/**
 * Implements Atrinik.Object.Object.Clone() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_Clone(Atrinik_Object *obj, PyObject *args)
{
    int inventory = 1;

    if (!PyArg_ParseTuple(args, "|i", &inventory)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    object *clone_ob;
    if (inventory) {
        clone_ob = hooks->object_create_clone(obj->obj);
    } else {
        clone_ob = hooks->get_object();
        hooks->copy_object(obj->obj, clone_ob, 0);
    }

    if (clone_ob->type == PLAYER || QUERY_FLAG(clone_ob, FLAG_IS_PLAYER)) {
        clone_ob->type = MONSTER;
        CLEAR_FLAG(clone_ob, FLAG_IS_PLAYER);
    }

    return wrap_object(clone_ob);
}

/** Documentation for Atrinik_Object_ReadKey(). */
static const char doc_Atrinik_Object_ReadKey[] =
".. method:: ReadKey(key).\n\n"
"Get key value of an object.\n\n"
":param key: Key to look for.\n"
":type key: :attr:`str`\n"
":returns: Value for the key if found, None otherwise.\n"
":rtype: :attr:`str` or :attr:`None`";

/**
 * Implements Atrinik.Object.Object.ReadKey() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_ReadKey(Atrinik_Object *obj, PyObject *args)
{
    const char *key;

    if (!PyArg_ParseTuple(args, "s", &key)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    return Py_BuildValue("s", hooks->object_get_value(obj->obj, key));
}

/** Documentation for Atrinik_Object_WriteKey(). */
static const char doc_Atrinik_Object_WriteKey[] =
".. method:: WriteKey(key, value=None, add_key=True).\n\n"
"Set the key value of an object.\n\n"
":param key: Key to set.\n"
":type key: :attr:`str`\n"
":param value: Value to set for the key. If None, will clear the key's value "
"if the key is found.\n"
":type value: :attr:`str` or :attr:`None`\n"
":param add_key: Whether to add the key if it's not found in the object.\n"
":type add_key: :attr:`bool`\n"
":returns: Whether the operation was successful.\n"
":rtype: :attr:`bool`";

/**
 * Implements Atrinik.Object.Object.WriteKey() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_WriteKey(Atrinik_Object *obj, PyObject *args)
{
    const char *key, *value = NULL;
    int add_key = 1;

    if (!PyArg_ParseTuple(args, "s|si", &key, &value, &add_key)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    Py_ReturnBoolean(hooks->object_set_value(obj->obj, key, value, add_key));
}

/** Documentation for Atrinik_Object_GetName(). */
static const char doc_Atrinik_Object_GetName[] =
".. method:: GetName(caller=None).\n\n"
"Acquire verbose textual representation of the object's name.\n\n"
":param caller: Who wants to see the name.\n"
":type caller: :class:`Atrinik.Object.Object` or :attr:`None`\n"
":returns: Full name of the object, including material, name, title, etc.\n"
":rtype: :attr:`str`";

/**
 * Implements Atrinik.Object.Object.GetName() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_GetName(Atrinik_Object *what, PyObject *args)
{
    Atrinik_Object *ob = NULL;

    if (!PyArg_ParseTuple(args, "|O!", &Atrinik_ObjectType, &ob)) {
        return NULL;
    }

    OBJEXISTCHECK(what);

    if (ob != NULL) {
        OBJEXISTCHECK(ob);
    }

    return Py_BuildValue("s", hooks->query_short_name(what->obj,
            ob != NULL ? ob->obj : what->obj));
}

/** Documentation for Atrinik_Object_Controller(). */
static const char doc_Atrinik_Object_Controller[] =
".. method:: Controller().\n\n"
"Get object's controller (the player).\n\n"
":returns: The controller if there is one, None otherwise.\n"
":rtype: :class:`Atrinik.Player.Player` or :attr:`None`\n"
":raises Atrinik.AtrinikError: If the object is not a player.";

/**
 * Implements Atrinik.Object.Object.Controller() Python method.
 * @copydoc PyMethod_NOARGS
 */
static PyObject *Atrinik_Object_Controller(Atrinik_Object *what)
{
    OBJEXISTCHECK(what);

    if (what->obj->type != PLAYER) {
        RAISE("Can only be used on players.");
    }

    return wrap_player(CONTR(what->obj));
}

/** Documentation for Atrinik_Object_Protection(). */
static const char doc_Atrinik_Object_Protection[] =
".. method:: Protection(protection).\n\n"
"Get object's protection value for the given protection ID.\n\n"
":param protection: One of the ATNR_xxx constants, eg, "
":attr:`~Atrinik.ATNR_SLASH`.\n"
":type protection: :attr:`int`\n"
":returns: The protection value.\n"
":rtype: :attr:`int`\n"
":raises IndexError: If the protection ID is invalid.";

/**
 * Implements Atrinik.Object.Object.Protection() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_Protection(Atrinik_Object *what, PyObject *args)
{
    int protection;

    if (!PyArg_ParseTuple(args, "i", &protection)) {
        return NULL;
    }

    OBJEXISTCHECK(what);

    if (protection < 0 || protection >= NROFATTACKS) {
        PyErr_SetString(PyExc_IndexError, "Protection ID is invalid.");
        return NULL;
    }

    return Py_BuildValue("b", what->obj->protection[protection]);
}

/** Documentation for Atrinik_Object_SetProtection(). */
static const char doc_Atrinik_Object_SetProtection[] =
".. method:: SetProtection(protection, value).\n\n"
"Set object's protection value for the given protection ID.\n\n"
":param protection: One of the ATNR_xxx constants, eg, "
":attr:`~Atrinik.ATNR_SLASH`.\n"
":type protection: :attr:`int`\n"
":param value: The value to set.\n"
":type value: :attr:`int`\n"
":raises IndexError: If the protection ID is invalid.\n"
":raises OverflowError: If the value to set is not in valid range.";

/**
 * Implements Atrinik.Object.Object.SetProtection() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_SetProtection(Atrinik_Object *what,
        PyObject *args)
{
    int protection;
    int8_t value;

    if (!PyArg_ParseTuple(args, "iB", &protection, &value)) {
        return NULL;
    }

    OBJEXISTCHECK(what);

    if (protection < 0 || protection >= NROFATTACKS) {
        PyErr_SetString(PyExc_IndexError, "Protection ID is invalid.");
        return NULL;
    }

    what->obj->protection[protection] = value;

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_Attack(). */
static const char doc_Atrinik_Object_Attack[] =
".. method:: Attack(attack, value).\n\n"
"Get object's attack value for the given attack ID.\n\n"
":param attack: One of the ATNR_xxx constants, eg, "
":attr:`~Atrinik.ATNR_SLASH`.\n"
":type attack: :attr:`int`\n"
":returns: The attack value.\n"
":rtype: :attr:`int`\n"
":raises IndexError: If the attack ID is invalid.";

/**
 * Implements Atrinik.Object.Object.Attack() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_Attack(Atrinik_Object *what, PyObject *args)
{
    int attack;

    if (!PyArg_ParseTuple(args, "i", &attack)) {
        return NULL;
    }

    OBJEXISTCHECK(what);

    if (attack < 0 || attack >= NROFATTACKS) {
        PyErr_SetString(PyExc_IndexError, "Attack ID is invalid.");
        return NULL;
    }

    return Py_BuildValue("B", what->obj->attack[attack]);
}

/** Documentation for Atrinik_Object_SetAttack(). */
static const char doc_Atrinik_Object_SetAttack[] =
".. method:: SetAttack(attack, value).\n\n"
"Set object's attack value for the given attack ID.\n\n"
":param attack: One of the ATNR_xxx constants, eg, "
":attr:`~Atrinik.ATNR_SLASH`.\n"
":type attack: :attr:`int`\n"
":param value: The value to set.\n"
":type value: :attr:`int`\n"
":raises IndexError: If the attack ID is invalid.\n"
":raises OverflowError: If the value to set is not in valid range.";

/**
 * Implements Atrinik.Object.Object.SetAttack() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_SetAttack(Atrinik_Object *what, PyObject *args)
{
    int attack;
    uint8_t value;

    if (!PyArg_ParseTuple(args, "ib", &attack, &value)) {
        return NULL;
    }

    OBJEXISTCHECK(what);

    if (attack < 0 || attack >= NROFATTACKS) {
        PyErr_SetString(PyExc_IndexError, "Attack ID is invalid.");
        return NULL;
    }

    what->obj->attack[attack] = value;

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_Decrease(). */
static const char doc_Atrinik_Object_Decrease[] =
".. method:: Decrease(num=1).\n\n"
"Decreases an object, removing it if there's nothing left to decrease.\n\n"
":param num: How much to decrease the object by.\n"
":type num: :attr:`int`\n"
":returns: The object if something is left, None otherwise.\n"
":rtype: :class:`Atrinik.Object.Object` or :attr:`None`";

/**
 * Implements Atrinik.Object.Object.Decrease() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_Decrease(Atrinik_Object *what, PyObject *args)
{
    uint32_t num = 1;

    if (!PyArg_ParseTuple(args, "|I", &num)) {
        return NULL;
    }

    OBJEXISTCHECK(what);

    return wrap_object(hooks->decrease_ob_nr(what->obj, num));
}

/** Documentation for Atrinik_Object_SquaresAround(). */
static const char doc_Atrinik_Object_SquaresAround[] =
".. method:: SquaresAround(range, type=Atrinik.AROUND_ALL, beyond=False, "
"callable=None).\n\n"
"Looks around the specified object and returns a list of tuples containing the "
"squares around it in a specified range. The tuples have a format of "
"**(map, x, y)**.\n\n"
"Example that ignores walls and floors with grass::\n\n"
"    from Atrinik import *\n"
"    activator = WhoIsActivator()\n"
"    def cmp_squares(m, x, y, obj):\n"
"        try:\n"
"            return m.GetLayer(x, y, LAYER_FLOOR)[0].name == \"grass\"\n"
"        # Exception was raised; ignore it, as it probably means there is no\n"
"        # floor.\n"
"        except:\n"
"            return False\n\n"
"    for (m, x, y) in activator.SquaresAround(1, type=AROUND_WALL,\n"
"                                             callable=cmp_squares):\n"
"        for ob in m.GetLayer(x, y, LAYER_FLOOR):\n"
"            print(ob)\n"
"\n\n"
":param range: Range around which to look at the squares. Must be higher"
"than 0.\n"
":type range: :attr:`int`\n"
":param type: One of or a combination of the AROUND_xxx constants, eg,"
":attr:`~Atrinik.AROUND_WALL`.\n"
":type type: :attr:`int`\n"
":param beyond: If True and one of checks from *type* parameter matches, all "
"squares beyond the one being checked will be ignored as well (think line of "
"sight).\n"
":type beyond: :attr:`bool`\n"
":param callable: Defines function to call for comparisons. The function "
"should have parameters in the order of **map, x, y, obj** where map is the"
"map, x/y are the coordinates and obj is the object that :meth:"
"`~Atrinik.Object.Object.SquaresAround()` was called for. The function should"
"return True if the square should be considered ignored, False otherwise."
"*type* being :attr:`~Atrinik.AROUND_ALL` takes no effect if this is set, but "
"it can be combined with the other AROUND_xxx constants.\n"
":type callable: :attr:`collections.Callable` or :attr:`None`\n"
":returns: A list containing tuples of the squares.\n"
":rtype: :attr:`list` of :attr:`tuple`";

/**
 * Implements Atrinik.Object.Object.SquaresAround() Python method.
 * @copydoc PyMethod_VARARGS_KEYWORDS
 */
static PyObject *Atrinik_Object_SquaresAround(Atrinik_Object *what,
        PyObject *args, PyObject *keywds)
{
    uint8_t range, type = AROUND_ALL;
    static char *kwlist[] = {"range", "type", "beyond", "callable", NULL};
    int beyond = 0;
    PyObject *callable = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "b|biO", kwlist, &range,
            &type, &beyond, &callable)) {
        return NULL;
    }

    OBJEXISTCHECK(what);

    if (range == 0) {
        PyErr_SetString(PyExc_ValueError, "'range' must be higher than 0.");
        return NULL;
    }

    if (callable != NULL && !PyCallable_Check(callable)) {
        PyErr_SetString(PyExc_TypeError,
                "Argument 'callable' must be callable.");
        return NULL;
    }

#define SQUARES_AROUND_SKIP(m, x, y)                                           \
    ((type & AROUND_BLOCKSVIEW && GET_MAP_FLAGS(m, x, y) & P_BLOCKSVIEW) ||    \
    (type & AROUND_PLAYER_ONLY && GET_MAP_FLAGS(m, x, y) & P_PLAYER_ONLY) ||   \
    (type & AROUND_WALL && hooks->wall(m, x, y)) || (callable != NULL &&       \
    python_call_int(callable, Py_BuildValue("(OiiO)", wrap_map(m), x, y,       \
    wrap_object(what->obj)))))

    PyObject *list = PyList_New(0);

    /* Go through the squares in the specified range. */
    for (int i = -range; i <= range; i++) {
        for (int j = -range; j <= range; j++) {
            int xt = what->obj->x + i;
            int yt = what->obj->y + j;

            /* Skip ourselves. */
            if (xt == what->obj->x && yt == what->obj->y) {
                continue;
            }

            mapstruct *m = hooks->get_map_from_coord(what->obj->map, &xt, &yt);
            if (m == NULL) {
                continue;
            }

            if (type == AROUND_ALL && callable == NULL) {
                /* We want all squares. */
                SQUARES_AROUND_ADD(m, xt, yt);
            } else if (beyond) {
                /* Only those that are not blocked by view, or beyond a wall,
                 * etc, so use the Bresenham algorithm. */

                mapstruct *m2 = what->obj->map;
                int xt2 = what->obj->x;
                int yt2 = what->obj->y;

                rv_vector rv;
                if (!hooks->get_rangevector_from_mapcoords(m2, xt2, yt2, m, xt,
                        yt, &rv, RV_NO_DISTANCE)) {
                    continue;
                }

                int fraction, dx2, dy2, stepx, stepy;
                BRESENHAM_INIT(rv.distance_x, rv.distance_y, fraction, stepx,
                        stepy, dx2, dy2);

                for ( ; ; ) {
                    BRESENHAM_STEP(xt2, yt2, fraction, stepx, stepy, dx2, dy2);
                    m2 = hooks->get_map_from_coord(m2, &xt2, &yt2);

                    if (m2 == NULL || SQUARES_AROUND_SKIP(m2, xt2, yt2)) {
                        break;
                    }

                    if (m2 == m && xt2 == xt && yt2 == yt) {
                        SQUARES_AROUND_ADD(m, xt, yt);
                        break;
                    }
                }
            } else {
                /* We only want to ignore squares that either block view, or have
                 * a wall, etc, but not any squares behind them. */
                if (SQUARES_AROUND_SKIP(m, xt, yt)) {
                    continue;
                }

                SQUARES_AROUND_ADD(m, xt, yt);
            }
        }
    }

#undef SQUARES_AROUND_SKIP

    return list;
}

/** Documentation for Atrinik_Object_GetRangeVector(). */
static const char doc_Atrinik_Object_GetRangeVector[] =
".. method:: GetRangeVector(to, flags=0).\n\n"
"Get the distance and direction from one object to another.\n\n"
":param to: Object to which the distance is calculated.\n"
":type to: :class:`Atrinik.Object.Object`\n"
":param flags: One or a combination of RV_xxx, eg, :attr:"
"`~Atrinik.RV_MANHATTAN_DISTANCE`\n"
":type flags: :attr:`int`\n"
":returns: None if the distance couldn't be calculated, otherwise a tuple "
"containing:\n\n"
"  * Direction *object* should head to reach *to*, eg, :attr:`~Atrinik.NORTH`\n"
"  * Distance between *object* and *to*.\n"
"  * X distance.\n"
"  * Y distance.\n"
"  * Part of the *object* that is closest.\n"
":rtype: :attr:`tuple` or :attr:`None`";

/**
 * Implements Atrinik.Object.Object.GetRangeVector() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_GetRangeVector(Atrinik_Object *obj, PyObject *args)
{
    Atrinik_Object *to;
    int flags = 0;

    if (!PyArg_ParseTuple(args, "O!|i", &Atrinik_ObjectType, &to, &flags)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);
    OBJEXISTCHECK(to);

    rv_vector rv;
    if (!hooks->get_rangevector(obj->obj, to->obj, &rv, flags)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    PyObject *tuple = PyTuple_New(5);
    PyTuple_SET_ITEM(tuple, 0, Py_BuildValue("i", rv.direction));
    PyTuple_SET_ITEM(tuple, 1, Py_BuildValue("i", rv.distance));
    PyTuple_SET_ITEM(tuple, 2, Py_BuildValue("i", rv.distance_x));
    PyTuple_SET_ITEM(tuple, 3, Py_BuildValue("i", rv.distance_y));
    PyTuple_SET_ITEM(tuple, 4, wrap_object(rv.part));

    return tuple;
}

/** Documentation for Atrinik_Object_CreateTreasure(). */
static const char doc_Atrinik_Object_CreateTreasure[] =
".. method:: CreateTreasure(treasure=None, level=0, flags=0, "
"a_chance=Atrinik.ART_CHANCE_UNSET).\n\n"
"Create treasure inside (or below, if :attr:`~Atrinik.GT_ENVIRONMENT` flag was "
"set) the object.\n\n"
":param treasure: Treasure list name to generate. If None, will try to "
"generate treasure based on the object's randomitems.\n"
":type treasure: :attr:`str` or :attr:`None`\n"
":param level: Level of the generated items. If 0, will try to guess the level "
"to use based on the object's level or the difficulty value of the map the "
"object is on. If neither is applicable, will use :attr:`~Atrinik.MAXLEVEL`.\n"
":type level: :attr:`int`\n"
":param flags: One or a combination of GT_xxx, eg, :attr:"
"`~Atrinik.GT_ENVIRONMENT`\n"
":type flags: :attr:`int`\n"
":param a_chance: Chance for the treasure to become artifact, if possible. A "
"value of 0 will disable any chance for artifacts.\n"
":type a_chance: :attr:`int`\n"
":raises ValueError: If treasure is not valid.";

/**
 * Implements Atrinik.Object.Object.CreateTreasure() Python method.
 * @copydoc PyMethod_VARARGS_KEYWORDS
 */
static PyObject *Atrinik_Object_CreateTreasure(Atrinik_Object *obj,
        PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"treasure", "level", "flags", "a_chance", NULL};
    const char *treasure_name = NULL;
    int level = 0, flags = 0, a_chance = ART_CHANCE_UNSET;
    treasurelist *t;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|ziii", kwlist,
            &treasure_name, &level, &flags, &a_chance)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    /* Figure out the treasure list. */
    if (treasure_name != NULL) {
        t = hooks->find_treasurelist(treasure_name);
    } else {
        t = obj->obj->randomitems;
    }

    /* Invalid treasure list. */
    if (t == NULL) {
        if (treasure_name) {
            PyErr_Format(PyExc_ValueError, "'%s' is not a valid treasure list.",
                    treasure_name);
        } else {
            PyErr_SetString(PyExc_ValueError, "Object has no treasure list.");
        }

        return NULL;
    }

    /* Figure out the level if none was given. */
    if (level == 0) {
        /* Try the object's level first. */
        if (obj->obj->level != 0) {
            level = obj->obj->level;
        } else if (obj->obj->map != NULL) {
            /* Otherwise the map's difficulty. */
            level = obj->obj->map->difficulty;
        } else {
            /* Default to MAXLEVEL. */
            level = MAXLEVEL;
        }
    }

    /* Create the treasure. */
    hooks->create_treasure(t, obj->obj, flags, level, T_STYLE_UNSET, a_chance,
            0, NULL);

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_Move(). */
static const char doc_Atrinik_Object_Move[] =
".. method:: Move(direction).\n\n"
"Move the object in the specified direction. The object must have the correct "
"(combination of) :meth:`~Atrinik.Object.Object.terrain_flag` set in order to "
"able to move onto the new square.\n\n"
":param direction: Direction to move into, eg, :attr:`~Atrinik.EAST`.\n"
":type direction: :attr:`int`\n"
":returns: 0 if the object is not able to move to the desired space, -1 if the "
"object was not able to move there yet but some sort of action was performed "
"that might allow us to move there (door opening for example), direction "
"number that the object ended up moving in otherwise.\n"
":rtype: :attr:`int`";

/**
 * Implements Atrinik.Object.Object.Move() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_Move(Atrinik_Object *obj, PyObject *args)
{
    int direction;

    if (!PyArg_ParseTuple(args, "i", &direction)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    if (obj->obj->map == NULL) {
        PyErr_SetString(AtrinikError, "Object not on map.");
        return NULL;
    }

    return Py_BuildValue("i", hooks->move_ob(obj->obj, direction, obj->obj));
}

/** Documentation for Atrinik_Object_ConnectionTrigger(). */
static const char doc_Atrinik_Object_ConnectionTrigger[] =
".. method:: ConnectionTrigger(push=True, button=False).\n\n"
"Triggers the object's connection, if any.\n\n"
":param push: If true, send a 'push' signal; 'release' signal otherwise.\n"
":type push: :attr:`bool`\n"
":param button: If true, handle the connection like a button.\n"
":type button: :attr:`bool`";

/**
 * Implements Atrinik.Object.Object.ConnectionTrigger() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_ConnectionTrigger(Atrinik_Object *obj,
        PyObject *args)
{
    int push = 1, button = 0;

    if (!PyArg_ParseTuple(args, "|ii", &push, &button)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    if (button) {
        hooks->connection_trigger_button(obj->obj, push);
    } else {
        hooks->connection_trigger(obj->obj, push);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_Artificate(). */
static const char doc_Atrinik_Object_Artificate[] =
".. method:: Artificate(name).\n\n"
"Copies artifact abilities to the specified object.\n\n"
":param name: Name of the artifact to copy abilities from.\n"
":type name: :attr:`str`\n"
":raises Atrinik.AtrinikError: If the object already has artifact "
"abilities.\n"
":raises Atrinik.AtrinikError: If the object's type doesn't match "
"any artifact list.\n"
":raises Atrinik.AtrinikError: If the artifact name is invalid.";

/**
 * Implements Atrinik.Object.Object.Artificate() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_Artificate(Atrinik_Object *obj, PyObject *args)
{
    const char *name = NULL;

    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    if (obj->obj->artifact) {
        PyErr_SetString(AtrinikError, "Object already has artifact abilities.");
        return NULL;
    }

    artifact_list_t *artlist = hooks->artifact_list_find(
            obj->obj->arch->clone.type);
    if (artlist == NULL) {
        PyErr_SetString(AtrinikError,
                "No artifact list matching the object's type.");
        return NULL;
    }

    for (artifact_t *art = artlist->items; art != NULL; art = art->next) {
        if (strcmp(art->def_at->name, name) == 0) {
            hooks->artifact_change_object(art, obj->obj);
            Py_INCREF(Py_None);
            return Py_None;
        }
    }

    PyErr_SetString(AtrinikError, "Invalid artifact name.");
    return NULL;
}

/** Documentation for Atrinik_Object_Load(). */
static const char doc_Atrinik_Object_Load[] =
".. method:: Load(name).\n\n"
"Load archetype-like attribute/value pairs into the object. For example, "
"**attack_protect 20\ndam 10**\n\n"
":param lines: Lines to load into the object.\n"
":type lines: :attr:`str`";

/**
 * Implements Atrinik.Object.Object.Load() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_Load(Atrinik_Object *obj, PyObject *args)
{
    const char *lines;

    if (!PyArg_ParseTuple(args, "s", &lines)) {
        return NULL;
    }

    hooks->set_variable(obj->obj, lines);

    Py_INCREF(Py_None);
    return Py_None;
}

/** Documentation for Atrinik_Object_GetPacket(). */
static const char doc_Atrinik_Object_GetPacket[] =
".. method:: GetPacket(pl, flags=0).\n\n"
"Constructs packet data about the object, suitable for "
":meth:`Atrinik.Player.Player.SendPacket`.\n\n"
":param pl: Player that will receive the item data.\n"
":type pl: :class:`Atrinik.Player.Player`\n"
":param flags: A combination of UPD_xxx flags, eg, :attr:`Atrinik.UPD_FACE`.\n"
":type flags: :attr:`int`\n"
":returns: A tuple containing the format specifier and the actual data list.\n"
":rtype: :attr:`tuple`";

/**
 * Implements Atrinik.Object.Object.GetPacket() Python method.
 * @copydoc PyMethod_VARARGS
 */
static PyObject *Atrinik_Object_GetPacket(Atrinik_Object *obj, PyObject *args)
{
    Atrinik_Player *pl;
    uint16_t flags = 0;

    if (!PyArg_ParseTuple(args, "O!|H", &Atrinik_PlayerType, &pl, &flags)) {
        return NULL;
    }

    OBJEXISTCHECK(obj);

    packet_struct *packet = hooks->packet_new(0, 128, 128);
    hooks->add_object_to_packet(packet, obj->obj, pl->pl->ob,
            CMD_APPLY_ACTION_NORMAL, flags, 0);
    PyObject *data = PyBytes_FromStringAndSize((const char *) packet->data,
            packet->len);
    hooks->packet_free(packet);

    PyObject *fmt = Py_BuildValue("s", "Hx");
    PyObject *list = PyList_New(2);
    PyList_SetItem(list, 0, Py_BuildValue("H", flags));
    PyList_SetItem(list, 1, data);

    PyObject *tuple = PyTuple_New(2);
    PyTuple_SET_ITEM(tuple, 0, fmt);
    PyTuple_SET_ITEM(tuple, 1, list);

    return tuple;
}

/** Available Python methods for the Atrinik.Object.Object object */
static PyMethodDef methods[] = {
    {"ActivateRune", (PyCFunction) Atrinik_Object_ActivateRune, METH_VARARGS,
            doc_Atrinik_Object_ActivateRune},
    {"TeleportTo", (PyCFunction) Atrinik_Object_TeleportTo,
            METH_VARARGS | METH_KEYWORDS, doc_Atrinik_Object_TeleportTo},
    {"InsertInto", (PyCFunction) Atrinik_Object_InsertInto, METH_VARARGS,
            doc_Atrinik_Object_InsertInto},
    {"Apply", (PyCFunction) Atrinik_Object_Apply, METH_VARARGS,
            doc_Atrinik_Object_Apply},
    {"Take", (PyCFunction) Atrinik_Object_Take, METH_O,
            doc_Atrinik_Object_Take},
    {"Drop", (PyCFunction) Atrinik_Object_Drop, METH_O,
            doc_Atrinik_Object_Drop},
    {"Say", (PyCFunction) Atrinik_Object_Say, METH_VARARGS,
            doc_Atrinik_Object_Say},
    {"GetGender", (PyCFunction) Atrinik_Object_GetGender, METH_NOARGS,
            doc_Atrinik_Object_GetGender},
    {"SetGender", (PyCFunction) Atrinik_Object_SetGender, METH_VARARGS,
            doc_Atrinik_Object_SetGender},
    {"Update", (PyCFunction) Atrinik_Object_Update, METH_NOARGS, 0},
    {"Hit", (PyCFunction) Atrinik_Object_Hit, METH_VARARGS,
            doc_Atrinik_Object_Hit},
    {"Cast", (PyCFunction) Atrinik_Object_Cast, METH_VARARGS | METH_KEYWORDS,
            doc_Atrinik_Object_Cast},
    {"CreateForce", (PyCFunction) Atrinik_Object_CreateForce, METH_VARARGS,
            doc_Atrinik_Object_CreateForce},
    {"CreateObject", (PyCFunction) Atrinik_Object_CreateObject,
            METH_VARARGS | METH_KEYWORDS, doc_Atrinik_Object_CreateObject},
    {"FindObject", (PyCFunction) Atrinik_Object_FindObject,
            METH_VARARGS | METH_KEYWORDS, doc_Atrinik_Object_FindObject},
    {"Remove", (PyCFunction) Atrinik_Object_Remove, METH_NOARGS,
            doc_Atrinik_Object_Remove},
    {"Destroy", (PyCFunction) Atrinik_Object_Destroy, METH_NOARGS,
            doc_Atrinik_Object_Destroy},
    {"SetPosition", (PyCFunction) Atrinik_Object_SetPosition, METH_VARARGS,
            doc_Atrinik_Object_SetPosition},
    {"CastIdentify", (PyCFunction) Atrinik_Object_CastIdentify, METH_VARARGS,
            doc_Atrinik_Object_CastIdentify},
    {"Save", (PyCFunction) Atrinik_Object_Save, METH_NOARGS,
            doc_Atrinik_Object_Save},
    {"GetCost", (PyCFunction) Atrinik_Object_GetCost, METH_VARARGS,
            doc_Atrinik_Object_GetCost},
    {"GetMoney", (PyCFunction) Atrinik_Object_GetMoney, METH_NOARGS,
            doc_Atrinik_Object_GetMoney},
    {"PayAmount", (PyCFunction) Atrinik_Object_PayAmount, METH_VARARGS,
            doc_Atrinik_Object_PayAmount},
    {"Clone", (PyCFunction) Atrinik_Object_Clone, METH_VARARGS,
            doc_Atrinik_Object_Clone},
    {"ReadKey", (PyCFunction) Atrinik_Object_ReadKey, METH_VARARGS,
            doc_Atrinik_Object_ReadKey},
    {"WriteKey", (PyCFunction) Atrinik_Object_WriteKey, METH_VARARGS,
            doc_Atrinik_Object_WriteKey},
    {"GetName", (PyCFunction) Atrinik_Object_GetName, METH_VARARGS,
            doc_Atrinik_Object_GetName},
    {"Controller", (PyCFunction) Atrinik_Object_Controller, METH_NOARGS,
            doc_Atrinik_Object_Controller},
    {"Protection", (PyCFunction) Atrinik_Object_Protection, METH_VARARGS,
            doc_Atrinik_Object_Protection},
    {"SetProtection", (PyCFunction) Atrinik_Object_SetProtection, METH_VARARGS,
            doc_Atrinik_Object_SetProtection},
    {"Attack", (PyCFunction) Atrinik_Object_Attack, METH_VARARGS,
            doc_Atrinik_Object_Attack},
    {"SetAttack", (PyCFunction) Atrinik_Object_SetAttack, METH_VARARGS,
            doc_Atrinik_Object_SetAttack},
    {"Decrease", (PyCFunction) Atrinik_Object_Decrease, METH_VARARGS,
            doc_Atrinik_Object_Decrease},
    {"SquaresAround", (PyCFunction) Atrinik_Object_SquaresAround,
            METH_VARARGS | METH_KEYWORDS, doc_Atrinik_Object_SquaresAround},
    {"GetRangeVector", (PyCFunction) Atrinik_Object_GetRangeVector,
            METH_VARARGS, doc_Atrinik_Object_GetRangeVector},
    {"CreateTreasure", (PyCFunction) Atrinik_Object_CreateTreasure,
            METH_VARARGS | METH_KEYWORDS, doc_Atrinik_Object_CreateTreasure},
    {"Move", (PyCFunction) Atrinik_Object_Move, METH_VARARGS,
            doc_Atrinik_Object_Move},
    {"ConnectionTrigger", (PyCFunction) Atrinik_Object_ConnectionTrigger,
            METH_VARARGS, doc_Atrinik_Object_ConnectionTrigger},
    {"Artificate", (PyCFunction) Atrinik_Object_Artificate, METH_VARARGS,
            doc_Atrinik_Object_Artificate},
    {"Load", (PyCFunction) Atrinik_Object_Load, METH_VARARGS,
            doc_Atrinik_Object_Load},
    {"GetPacket", (PyCFunction) Atrinik_Object_GetPacket, METH_VARARGS,
            doc_Atrinik_Object_GetPacket},
    {NULL, NULL, 0, 0}
};

/**
 * Get object's attribute.
 * @param obj Python object wrapper.
 * @param context Void pointer to the field.
 * @return Python object with the attribute value, NULL on failure.
 */
static PyObject *Object_GetAttribute(Atrinik_Object *obj, void *context)
{
    OBJEXISTCHECK(obj);
    fields_struct *field = context;

    if (field->offset == offsetof(object, head)) {
        return wrap_object(HEAD(obj->obj));
    }

    return generic_field_getter(field, obj->obj);
}

/**
 * Set attribute of an object.
 * @param obj Python object wrapper.
 * @param value Value to set.
 * @param context Void pointer to the field.
 * @return 0 on success, -1 on failure.
 */
static int Object_SetAttribute(Atrinik_Object *obj, PyObject *value,
        void *context)
{
    fields_struct *field = context;
    int ret;

    OBJEXISTCHECK_INT(obj);

    if ((field->flags & FIELDFLAG_PLAYER_READONLY) &&
            obj->obj->type == PLAYER) {
        INTRAISE("Trying to modify a field that is read-only for player "
                "objects.");
    }

    if (field->offset == offsetof(object, type) &&
            obj->obj->custom_attrset != NULL) {
        INTRAISE("Cannot modify type of object that has custom_attrset.");
    }

    if (obj->obj->map != NULL && (field->offset == offsetof(object, layer) ||
            field->offset == offsetof(object, sub_layer))) {
        hooks->object_remove(obj->obj, 0);
    }

    ret = generic_field_setter(field, obj->obj, value);

    if (field->offset == offsetof(object, layer) ||
            field->offset == offsetof(object, sub_layer)) {
        obj->obj->layer = MIN(NUM_LAYERS, obj->obj->layer);
        obj->obj->sub_layer = MIN(NUM_SUB_LAYERS - 1, obj->obj->sub_layer);

        if (obj->obj->map != NULL) {
            hooks->insert_ob_in_map(obj->obj, obj->obj->map, NULL, 0);
        }
    }

    if (ret == -1) {
        return -1;
    }

    if (field->offset == offsetof(object, type) && obj->obj->type == PLAYER) {
        obj->obj->type = MONSTER;
    }

    hooks->esrv_send_item(obj->obj);

    /* Special handling for some player stuff. */
    if (obj->obj->type == PLAYER) {
        if (field->flags & FIELDFLAG_PLAYER_FIX) {
            hooks->living_update(obj->obj);
        }
    }

    /* Update object's speed. */
    if (field->offset == offsetof(object, speed)) {
        hooks->update_ob_speed(obj->obj);
    } else if (field->offset == offsetof(object, type)) {
        /* Handle object's type changing. */

        /* Changing to a spawn point monster requires special handling:
         * as the object was most likely created and put on active list,
         * we must remove it from the active list, as spawn point monsters
         * are not allowed to be on the list. */
        if (obj->obj->type == SPAWN_POINT_MOB) {
            float old_speed;

            /* Store original speed, as in order to actually remove the object
             * from the active list, we need to set its speed to 0 and make it
             * a non-SPAWN_POINT_MOB type. */
            old_speed = obj->obj->speed;
            obj->obj->speed = 0.0f;
            obj->obj->type = MONSTER;
            /* Remove it from the active list. */
            hooks->update_ob_speed(obj->obj);

            /* Restore original speed and type info. */
            obj->obj->speed = old_speed;
            obj->obj->type = SPAWN_POINT_MOB;
        }
    } else if (field->offset == offsetof(object, direction)) {
        /* Direction. */

        /* If the object is animated and turnable, update its face. */
        if (obj->obj->animation_id && QUERY_FLAG(obj->obj, FLAG_IS_TURNABLE)) {
            SET_ANIMATION(obj->obj, (NUM_ANIMATIONS(obj->obj) /
                    NUM_FACINGS(obj->obj)) * obj->obj->direction +
                    obj->obj->state);
        }
    } else if (field->offset == offsetof(object, enemy)) {
        if (QUERY_FLAG(obj->obj, FLAG_MONSTER)) {
            hooks->monster_enemy_signal(obj->obj, obj->obj->enemy);
        }
    }

    return 0;
}

/**
 * Get object's flag.
 * @param obj Python object wrapper.
 * @param context Void pointer to the flag ID.
 * @retval Py_True The object has the flag set.
 * @retval Py_False The object doesn't have the flag set.
 * @retval NULL An error occurred.
 */
static PyObject *Object_GetFlag(Atrinik_Object *obj, void *context)
{
    size_t flagno = (size_t) context;

    /* Should not happen. */
    if (flagno >= NUM_FLAGS) {
        PyErr_SetString(PyExc_OverflowError, "Invalid flag ID.");
        return NULL;
    }

    OBJEXISTCHECK(obj);

    Py_ReturnBoolean(QUERY_FLAG(obj->obj, flagno));
}

/**
 * Set flag for an object.
 * @param obj Python object wrapper.
 * @param val Value to set. Should be either Py_True or Py_False.
 * @param context Void pointer to the flag ID.
 * @return 0 on success, -1 on failure.
 */
static int Object_SetFlag(Atrinik_Object *obj, PyObject *val, void *context)
{
    size_t flagno = (size_t) context;

    /* Should not happen. */
    if (flagno >= NUM_FLAGS) {
        PyErr_SetString(PyExc_OverflowError, "Invalid flag ID.");
        return -1;
    }

    OBJEXISTCHECK_INT(obj);

    if (val == Py_True) {
        SET_FLAG(obj->obj, flagno);
    } else if (val == Py_False) {
        CLEAR_FLAG(obj->obj, flagno);
    } else {
        PyErr_SetString(PyExc_TypeError, "Flag value must be either True or "
                "False.");
        return -1;
    }

    hooks->esrv_send_item(obj->obj);
    return 0;
}

/**
 * Create a new object wrapper.
 * @param type Type object.
 * @param args Unused.
 * @param kwds Unused.
 * @return The new wrapper.
 */
static PyObject *Atrinik_Object_new(PyTypeObject *type, PyObject *args,
        PyObject *kwds)
{
    Atrinik_Object *self = (Atrinik_Object *) type->tp_alloc(type, 0);

    if (self) {
        self->obj = NULL;
        self->count = 0;
    }

    return (PyObject *) self;
}

/**
 * Free an object wrapper.
 * @param self The wrapper to free.
 */
static void Atrinik_Object_dealloc(PyObject *self)
{
    ((Atrinik_Object *) self)->obj = NULL;
    ((Atrinik_Object *) self)->count = 0;
#ifndef IS_PY_LEGACY
    Py_TYPE(self)->tp_free(self);
#else
    self->ob_type->tp_free(self);
#endif
}

/**
 * Return a string representation of an object.
 * @param self The object type.
 * @return Python object containing the arch name and name of the object.
 */
static PyObject *Atrinik_Object_str(Atrinik_Object *self)
{
    OBJEXISTCHECK(self);
    return PyString_FromFormat("[%s \"%s\"]", STRING_OBJ_ARCH_NAME(self->obj),
            STRING_OBJ_NAME(self->obj));
}

static int Atrinik_Object_InternalCompare(Atrinik_Object *left,
        Atrinik_Object *right)
{
    OBJEXISTCHECK_INT(left);
    OBJEXISTCHECK_INT(right);
    return (left->obj < right->obj ? -1 : (left->obj == right->obj ? 0 : 1));
}

static PyObject *Atrinik_Object_RichCompare(Atrinik_Object *left,
        Atrinik_Object *right, int op)
{
    if (left == NULL || right == NULL ||
            !PyObject_TypeCheck((PyObject *) left, &Atrinik_ObjectType) ||
            !PyObject_TypeCheck((PyObject *) right, &Atrinik_ObjectType)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    int result = Atrinik_Object_InternalCompare(left, right);

    /* Handle removed objects. */
    if (result == -1 && PyErr_Occurred()) {
        return NULL;
    }

    return generic_rich_compare(op, result);
}

/**
 * Start iterating.
 * @param seq Object to start iterating from.
 * @return A new instance of PyObject, containing Atrinik_Object, with a
 * reference to 'seq'.
 */
static PyObject *object_iter(PyObject *seq)
{
    Atrinik_Object *obj, *orig_obj = (Atrinik_Object *) seq;

    OBJEXISTCHECK(orig_obj);

    obj = PyObject_NEW(Atrinik_Object, &Atrinik_ObjectType);
    Py_INCREF(seq);
    obj->iter = (Atrinik_Object *) seq;
    obj->iter_type = OBJ_ITER_TYPE_ONE;

    /* Select which iteration type we're doing. It's possible that
     * an object has both below and above set (it's not the first and
     * not the last object), in which case we will prefer below. */
    if (orig_obj->obj->below) {
        obj->iter_type = OBJ_ITER_TYPE_BELOW;
    } else if (orig_obj->obj->above) {
        obj->iter_type = OBJ_ITER_TYPE_ABOVE;
    }

    return (PyObject *) obj;
}

/**
 * Get next object for iteration.
 * @param obj Previous object.
 * @return Next object, NULL if there is nothing left.
 */
static PyObject *object_iternext(Atrinik_Object *obj)
{
    /* Do we need to iterate? */
    if (obj->iter_type != OBJ_ITER_TYPE_NONE) {
        OBJEXISTCHECK(obj->iter);
        object *tmp = obj->iter->obj;

        /* Check which way we're iterating. */
        if (obj->iter_type == OBJ_ITER_TYPE_BELOW) {
            obj->iter->obj = tmp->below;
        } else if (obj->iter_type == OBJ_ITER_TYPE_ABOVE) {
            obj->iter->obj = tmp->above;
        } else if (obj->iter_type == OBJ_ITER_TYPE_ONE) {
            obj->iter->obj = NULL;
        }

        obj->iter->count = obj->iter->obj != NULL ? obj->iter->obj->count : 0;

        /* Nothing left, so mark iter_type to show that. */
        if (obj->iter->obj == NULL) {
            obj->iter_type = OBJ_ITER_TYPE_NONE;
        }

        return wrap_object(tmp);
    }

    return NULL;
}

/**
 * Atrinik object bool check.
 * @param obj The object.
 */
static int atrinik_object_bool(Atrinik_Object *obj)
{
    if (obj == NULL || obj->obj == NULL || obj->obj->count != obj->count ||
            OBJECT_FREE(obj->obj)) {
        return 0;
    }

    return 1;
}

/** This is filled in when we initialize our object type. */
static PyGetSetDef getseters[NUM_FIELDS + NUM_FLAGS + 1];

/**
 * The number protocol for Atrinik objects. */
static PyNumberMethods AtrinikObjectNumber = {
    NULL,
    NULL,
    NULL,
#ifndef IS_PY3K
    NULL, /* nb_divide */
#endif
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    (inquiry) atrinik_object_bool,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#ifndef IS_PY3K
    NULL, /* nb_coerce */
#endif
    NULL,
    NULL,
    NULL,
#ifndef IS_PY3K
    NULL, /* nb_oct */
    NULL, /* nb_hex */
#endif
    NULL,
    NULL,
    NULL,
#ifndef IS_PY3K
    NULL, /* nb_inplace_divide */
#endif
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/**
 * Our actual Python ObjectType.
 */
PyTypeObject Atrinik_ObjectType = {
#ifdef IS_PY3K
    PyVarObject_HEAD_INIT(NULL, 0)
#else
    PyObject_HEAD_INIT(NULL)
    0,
#endif
    "Atrinik.Object",
    sizeof(Atrinik_Object),
    0,
    (destructor) Atrinik_Object_dealloc,
    NULL, NULL, NULL,
#ifdef IS_PY3K
    NULL,
#else
    (cmpfunc) Atrinik_Object_InternalCompare,
#endif
    NULL,
    &AtrinikObjectNumber,
    0, 0, 0, 0,
    (reprfunc) Atrinik_Object_str,
    0, 0, 0,
    Py_TPFLAGS_DEFAULT,
    "Atrinik objects",
    NULL, NULL,
    (richcmpfunc) Atrinik_Object_RichCompare,
    0,
    (getiterfunc) object_iter,
    (iternextfunc) object_iternext,
    methods,
    0,
    getseters,
    0, 0, 0, 0, 0, 0, 0,
    Atrinik_Object_new,
    0, 0, 0, 0, 0, 0, 0, 0
#ifndef IS_PY_LEGACY
    , 0
#endif
#ifdef Py_TPFLAGS_HAVE_FINALIZE
    , NULL
#endif
};

/**
 * Initialize the object wrapper.
 * @param module The Atrinik Python module.
 * @return 1 on success, 0 on failure.
 */
int Atrinik_Object_init(PyObject *module)
{
    size_t i;

    /* Field getseters */
    for (i = 0; i < NUM_FIELDS; i++) {
        PyGetSetDef *def = &getseters[i];

        def->name = fields[i].name;
        def->get = (getter) Object_GetAttribute;
        def->set = (setter) Object_SetAttribute;
        def->doc = fields[i].doc;
        def->closure = &fields[i];
    }

    /* Flag getseters */
    for (size_t flagno = 0; flagno < NUM_FLAGS; flagno++) {
        if (hooks->object_flag_names[flagno] == NULL) {
            continue;
        }
        PyGetSetDef *def = &getseters[i++];

        char buf[MAX_BUF];
        snprintf(VS(buf), "f_%s", hooks->object_flag_names[flagno]);
        def->name = strdup(buf);

        def->get = (getter) Object_GetFlag;
        def->set = (setter) Object_SetFlag;
        def->doc = doc_object_flag_names[flagno];
        def->closure = (void *) flagno;
    }

    getseters[i].name = NULL;

    Atrinik_ObjectType.tp_new = PyType_GenericNew;

    if (PyType_Ready(&Atrinik_ObjectType) < 0) {
        return 0;
    }

    Py_INCREF(&Atrinik_ObjectType);
    PyModule_AddObject(module, "Object", (PyObject *) &Atrinik_ObjectType);

    return 1;
}

/**
 * Utility method to wrap an object.
 * @param what Object to wrap.
 * @return Python object wrapping the real object.
 */
PyObject *wrap_object(object *what)
{
    Atrinik_Object *wrapper;

    /* Return None if no object was to be wrapped. */
    if (what == NULL || OBJECT_FREE(what)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    wrapper = PyObject_NEW(Atrinik_Object, &Atrinik_ObjectType);
    if (wrapper != NULL) {
        wrapper->obj = what;
        wrapper->count = wrapper->obj->count;
    }

    return (PyObject *) wrapper;
}

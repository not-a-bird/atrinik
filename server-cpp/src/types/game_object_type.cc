/*******************************************************************************
 *               Atrinik, a Multiplayer Online Role Playing Game               *
 *                                                                             *
 *       Copyright (C) 2009-2014 Alex Tokar and Atrinik Development Team       *
 *                                                                             *
 * This program is free software; you can redistribute it and/or modify it     *
 * under the terms of the GNU General Public License as published by the Free  *
 * Software Foundation; either version 2 of the License, or (at your option)   *
 * any later version.                                                          *
 *                                                                             *
 * This program is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for    *
 * more details.                                                               *
 *                                                                             *
 * You should have received a copy of the GNU General Public License along     *
 * with this program; if not, write to the Free Software Foundation, Inc.,     *
 * 675 Mass Ave, Cambridge, MA 02139, USA.                                     *
 *                                                                             *
 * The author can be reached at admin@atrinik.org                              *
 ******************************************************************************/

/**
 * @file
 * Game object type implementation.
 */

#include <game_object_type.h>

using namespace atrinik;
using namespace std;

namespace atrinik {

int GameObjectTypeFactory::guid(0);

GameObjectTypeFactory::MapType& GameObjectTypeFactory::map()
{
    static GameObjectTypeFactory::MapType* map =
            new GameObjectTypeFactory::MapType();

    return *map;
}

GameObjectType* GameObjectTypeFactory::create_instance(const std::string& s)
{
    MapType::iterator it = map().find(s);

    if (it == map().end()) {
        return NULL;
    }

    return it->second();
}

}
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
 * Region object implementation.
 */

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <region_object.h>
#include <logger.h>

using namespace atrinik;
using namespace boost;
using namespace std;

namespace atrinik {

bool RegionObject::load(const std::string& key, const std::string& val)
{
    BOOST_LOG_FUNCTION();

    if (key == "name") {
        name(val);
        return true;
    } else if (key == "parent") {
        parent(val);
        return true;
    } else if (key == "longname") {
        longname(val);
        return true;
    } else if (key == "msg") {
        msg(val);
        return true;
    } else if (key == "jail") {
        stringstream sstream(val);
        mapcoords_t coords;

        sstream >> coords.path >> coords.x >> coords.y;

        if (!sstream.fail()) {
            jail(coords);
        } else {
            LOG(Error) << "Bad value: " << key << " " << val;
        }

        return true;
    } else if (key == "map_first") {
        map_first(val);
        return true;
    } else if (key == "map_bg") {
        map_bg(val);
        return true;
    } else if (key == "map_quest") {
        f_map_quest(true);
        return true;
    }

    return false;
}

std::string RegionObject::dump()
{
    string s = "";

    s += "region " + name() + "\n";

    if (!parent().empty()) {
        s += "parent " + parent() + "\n";
    }

    if (!longname().empty()) {
        s += "longname " + longname() + "\n";
    }

    if (!msg().empty()) {
        s += "msg\n";
        s += msg() + "\n";
        s += "endmsg\n";
    }

    const mapcoords_t coords = jail();

    if (!coords.path.empty()) {
        s += "jail " + coords.path + " " + lexical_cast<string>(coords.x) +
                " " + lexical_cast<string>(coords.y) + "\n";
    }

    if (!map_first().empty()) {
        s += "map_first " + map_first() + "\n";
    }

    if (!map_bg().empty()) {
        s += "map_bg " + map_bg() + "\n";
    }

    if (f_map_quest()) {
        s += "map_quest 1\n";
    }

    s += "end\n";

    return s;
}

void RegionObject::inv_push_back(RegionObject* obj)
{
    inv_.push_back(obj);
    obj->env_ = this;
}

RegionManager RegionManager::manager;

bool RegionManager::add(RegionObject* region)
{
    return regions.insert(make_pair(region->name(), region)).second;
}

RegionObject* RegionManager::get(const std::string& name)
{
    auto it = regions.find(name);

    if (it == regions.end()) {
        return NULL;
    }

    return it->second;
}

RegionManager::RegionsMap::size_type RegionManager::count()
{
    return regions.size();
}

void RegionManager::link_parents_children()
{
    BOOST_LOG_FUNCTION();

    // Link up children/parents
    for (auto it : regions) {
        if (it.second->parent().empty()) {
            continue;
        }

        RegionObject* parent = RegionManager::manager.get(
                it.second->parent());

        if (!parent) {
            LOG(Error) << "Region " << it.second->name() <<
                    " has invalid parent: " << it.second->parent();
            continue;
        }

        parent->inv_push_back(it.second);
    }
}

}

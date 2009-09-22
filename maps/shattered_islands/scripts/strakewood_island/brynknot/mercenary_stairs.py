from Atrinik import *

activator = WhoIsActivator()

guild_tag = "Mercenary"

maps = {
	"merc_guild":
	{
		"map": "/shattered_islands/strakewood_island/brynknot/mercenary_guild",
		"x": 2,
		"y": 1,
	},
	"island":
	{
		"map": "/shattered_islands/world_0112",
		"x": 19,
		"y": 11,
	},
}

SetReturnValue(1)

guild_force = activator.GetGuildForce()

if guild_force.slaying != guild_tag:
	activator.Write("Only members of the Mercenaries can enter here.", 2)
	activator.Write("A strong magic guardian force pushes you back.", 3)
	activator.TeleportTo(maps["island"]["map"], maps["island"]["x"], maps["island"]["y"], 0)
else:
	activator.Write("You can enter.", 2)
	activator.Write("A magic guardian force moves you down the stairs.", 4)
	activator.TeleportTo(maps["merc_guild"]["map"], maps["merc_guild"]["x"], maps["merc_guild"]["y"], 0)

#!/bin/env python3

# In 1.4.7, liquids in tanks were referred to by item ID number.
# In 1.6.4, liquids in tanks are referred to by name.

import xml.etree.ElementTree

import remapper

def id_to_name(id, meta, map):
	name = map[id]
	if name == "teliquid":
		if meta == 0:
			return "redstone"
		elif meta == 64:
			return "ender"
		else:
			assert False
	return name


def remap_tank_compound(tank_compound, map, id_key="Id", meta_key="Meta", result_key="FluidName"):
	id_named = remapper.find_named(tank_compound, id_key)
	meta_named = remapper.find_named(tank_compound, meta_key)
	if id_named is not None and meta_named is not None:
		id_short = id_named[0]
		meta_short = meta_named[0]
		if int(id_short.get("value")) != 0:
			name_named = xml.etree.ElementTree.SubElement(tank_compound, "named", {"name": result_key})
			xml.etree.ElementTree.SubElement(name_named, "string", {"value": id_to_name(int(id_short.get("value")), int(meta_short.get("value")), map)})
			tank_compound.remove(id_named)
			tank_compound.remove(meta_named)


def remap_tank(te_compound, tank_name, map, id_key="Id", meta_key="Meta", result_key="FluidName"):
	if tank_name is not None:
		tank_compound = remapper.find_named_child(te_compound, tank_name)
	else:
		tank_compound = te_compound
	if tank_compound is not None:
		assert tank_compound.tag == "compound"
		remap_tank_compound(tank_compound, map, id_key, meta_key, result_key)


class PipeRemapper(remapper.TERemapper):
	def __init__(self, id, map):
		remapper.TERemapper.__init__(self, id)
		self._map = map

	def remap_te(self, te_compound, map_info):
		# Remap the tanks.
		for tank_name in "tank[0]", "tank[1]", "tank[2]", "tank[3]", "tank[4]", "tank[5]", "tank[6]":
			remap_tank(te_compound, tank_name, self._map)


class TankRemapper(remapper.TERemapper):
	def __init__(self, map, te, tank_name, id_key, meta_key, result_key="FluidName"):
		remapper.TERemapper.__init__(self, te)
		self._map = map
		self._tank_name = tank_name
		self._id_key = id_key
		self._meta_key = meta_key
		self._result_key = result_key

	def remap_te(self, te_compound, map_info):
		# Remap the tank.
		remap_tank(te_compound, self._tank_name, self._map, self._id_key, self._meta_key, self._result_key)


class TanksRemapper(remapper.TERemapper):
	def __init__(self, map, id):
		remapper.TERemapper.__init__(self, id)
		self._map = map

	def remap_te(self, te_compound, map_info):
		# Remap the tanks of a combustion engine.
		remap_tank(te_compound, "fuelTank", self._map)
		remap_tank(te_compound, "coolantTank", self._map)
		# Remap the tanks of a Railcraft steam engine or other machine.
		tanks_list = remapper.find_named_child(te_compound, "tanks")
		if tanks_list is not None:
			assert len(tanks_list) == 0 or tanks_list.get("subtype") == "10"
			for tank_compound in tanks_list:
				remap_tank_compound(tank_compound, self._map)


class EntityTanksRemapper(remapper.EntityRemapper):
	def __init__(self, map, id):
		remapper.EntityRemapper.__init__(self, id)
		self._map = map

	def remap_entity(self, entity_compound, map_info):
		tanks_list = remapper.find_named_child(entity_compound, "tanks")
		if tanks_list is not None:
			assert len(tanks_list) == 0 or tanks_list.get("subtype") == "10"
			for tank_compound in tanks_list:
				remap_tank_compound(tank_compound, self._map)


class RefineryRemapper(remapper.TERemapper):
	def __init__(self, map):
		remapper.TERemapper.__init__(self, "net.minecraft.src.buildcraft.factory.Refinery")
		self._map = map

	def remap_te(self, te_compound, map_info):
		# Remap the tanks.
		remap_tank(te_compound, "ingredient1", self._map)
		remap_tank(te_compound, "ingredient2", self._map)
		remap_tank(te_compound, "result", self._map)
		for key in "filters_0", "filters_1":
			filters_named = remapper.find_named(te_compound, key)
			if filters_named is not None:
				filters_int = filters_named[0]
				assert filters_int.tag == "int"
				assert int(filters_int.get("value")) == 0 # Not implemented yet


class ForestryMachineRemapper(remapper.TERemapper):
	def __init__(self, map):
		remapper.TERemapper.__init__(self, "forestry.Machine")
		self._map = map

	def remap_te(self, te_compound, map_info):
		machine_compound = remapper.find_named_child(te_compound, "Machine")
		if machine_compound is not None:
			assert machine_compound.tag == "compound"
			for tank_name in ("ProductTank", "ResourceTank"):
				tank_compound = remapper.find_named_child(machine_compound, tank_name)
				if tank_compound is not None:
					assert tank_compound.tag == "compound"
					remap_tank_compound(tank_compound, self._map, "liquidId", "liquidMeta")
					amount_named = remapper.find_named(tank_compound, "quantity")
					if amount_named is not None:
						amount_named.set("name", "Amount")


def create_all_remappers(map):
	ret = []
	ret.append(PipeRemapper("net.minecraft.src.buildcraft.transport.GenericPipe", map))
	ret.append(PipeRemapper("net.minecraft.src.buildcraft.GenericPipe", map))
	ret.append(TankRemapper(map, "net.minecraft.src.buildcraft.factory.TileTank", "tank", "Id", "Meta"))
	ret.append(TanksRemapper(map, "net.minecraft.src.buildcraft.energy.Engine"))
	ret.append(TanksRemapper(map, "RCEngineSteamHobby"))
	ret.append(TanksRemapper(map, "RCEngineSteamLow"))
	ret.append(TanksRemapper(map, "RCEngineSteamHigh"))
	ret.append(RefineryRemapper(map))
	ret += [TanksRemapper(map, id) for id in ("RCIronTankWallTile", "RCIronTankGaugeTile", "RCIronTankValveTile", "RCCokeOvenTile", "RCWaterTankTile")]
	ret += [TanksRemapper(map, id) for id in ("RCBoilerFireboxLiquidTile", "RCBoilerFireboxSolidTile")]
	ret.append(EntityTanksRemapper(map, "Railcraft.railcraft.cart.tank"))
	ret.append(EntityTanksRemapper(map, "Railcraft.cart.tank"))
	ret.append(TankRemapper(map, "forestry.Engine", "FuelSlot", "liquidId", "liquidMeta"))
	ret.append(TankRemapper(map, "forestry.Engine", "HeatingSlot", "liquidId", "liquidMeta"))
	ret.append(TankRemapper(map, "forestry.Engine", None, "CurrentLiquidId", "CurrentLiquidMeta", "currentFluid"))
	ret.append(ForestryMachineRemapper(map))
	ret.append(TankRemapper(map, "forestry.Farm", "LiquidTank", "liquidId", "liquidMeta"))
	return ret

#!/bin/env python3

import xml.etree.ElementTree

import remapper

import buildcraft


class TankFixer(remapper.TERemapper):
	def __init__(self):
		remapper.TERemapper.__init__(self, "forestry.Engine")

	def remap_te(self, te_compound, map_info):
		for slot_name in "HeatingSlot", "FuelSlot":
			slot_compound = remapper.find_named_child(te_compound, slot_name)
			if slot_compound is not None:
				assert slot_compound.tag == "compound"
				quantity_named = remapper.find_named(slot_compound, "quantity")
				if quantity_named is not None:
					quantity_named.set("name", "Amount")


class ForestryMachineRemapper(remapper.TERemapper):
	def __init__(self):
		remapper.TERemapper.__init__(self, "forestry.Machine")

	def remap_te(self, te_compound, map_info):
		machine_compound = remapper.find_named_child(te_compound, "Machine")
		if machine_compound is not None:
			assert machine_compound.tag == "compound"
			items_list = remapper.find_named_child(machine_compound, "Items")
			if items_list is not None:
				assert len(items_list) == 0 or items_list.get("subtype") == "10"
				for item_compound in items_list:
					remapper.remap_item_compound(item_compound, map_info)


def create_all_remappers():
	ret = []
	# Handle engine inventories.
	ret.append(remapper.SimpleItemContainerTERemapper("forestry.Engine", "sockets")) # 1.4.7 name for all engines
	ret.append(remapper.SimpleItemContainerTERemapper("forestry.Engine", "Items")) # 1.4.7 name for all engines
	ret.append(remapper.SimpleItemContainerTERemapper("forestry.EngineTin", "sockets")) # 1.6.4 name for electric engines
	ret.append(remapper.SimpleItemContainerTERemapper("forestry.EngineBronze", "Items")) # 1.6.4 name for biogas engines
	ret.append(remapper.SimpleItemContainerTERemapper("forestry.EngineCopper", "Items")) # 1.6.4 name for peat engines
	# Handle bee housing inventories.
	ret += [remapper.SimpleItemContainerTERemapper(x, "Items") for x in ("forestry.Apiary", "forestry.Alveary", "forestry.AlvearyFan", "forestry.AlvearyHeater", "forestry.AlvearySwarmer")]
	# Handle forestry machines in their 1.4.7 representation.
	ret.append(ForestryMachineRemapper())
	# Handle liquid tanks not using the same name for quantity from 1.4.7 to 1.6.4.
	ret.append(TankFixer())
	return ret


#!/bin/env python3

import xml.etree.ElementTree

import remapper

def remap_liquid(compound, map_info, key_name="liquidId"):
	number = remapper.find_named_child(compound, key_name)
	if number is not None:
		id = remapper.get_number_from_number(number, ("int", "short"))
		if id != 0:
			new_id = map_info.item_map[id]
			number.set("value", str(new_id))

class PipeRemapper(remapper.TERemapper):
	def __init__(self, id):
		remapper.TERemapper.__init__(self, id)

	def remap_te(self, te_compound, map_info):
		# Pipes are internally identified by the item ID of the item used to place them.
		# Remap this item ID.
		pipeId_named = remapper.find_named(te_compound, "pipeId")
		pipeId_int = pipeId_named[0]
		assert pipeId_int.tag == "int"
		pipeId = int(pipeId_int.get("value"))
		new_pipeId = map_info.item_map[pipeId]
		pipeId_int.set("value", str(new_pipeId))

		# BC2 liquid pipes have a bunch of things named "side[0…5]" and "center" which have "liquidId" and "qty" elements in them.
		# BC3 liquid pipes have things named "tank[0…6]" which have "Id" and "Amount" elements in them.
		# I think "center" becomes "tank[6]".
		# First, remap liquid IDs in the old names.
		for tank_name in "side[0]", "side[1]", "side[2]", "side[3]", "side[4]", "side[5]", "center":
			tank_named = remapper.find_named(te_compound, tank_name)
			if tank_named is not None:
				tank_compound = tank_named[0]
				assert tank_compound.tag == "compound"
				remap_liquid(tank_compound, map_info)
		# Second, remap liquid IDs in the new names.
		for tank_name in "tank[0]", "tank[1]", "tank[2]", "tank[3]", "tank[4]", "tank[5]", "tank[6]":
			tank_named = remapper.find_named(te_compound, tank_name)
			if tank_named is not None:
				tank_compound = tank_named[0]
				assert tank_compound.tag == "compound"
				remap_liquid(tank_compound, map_info, "Id")
		# Third, for each BC2 name that exists, if the corresponding BC3 name does not exist, create it.
		# Also, BC3 pipes have a "capacity" key in their tanks which does not exist in BC2; it is not properly auto-created if missing, but is always set to 250, so add it.
		for old_tank_name, new_tank_name in ("side[0]", "tank[0]"), ("side[1]", "tank[1]"), ("side[2]", "tank[2]"), ("side[3]", "tank[3]"), ("side[4]", "tank[4]"), ("side[5]", "tank[5]"), ("center", "tank[6]"):
			old_tank_named = remapper.find_named(te_compound, old_tank_name)
			new_tank_named = remapper.find_named(te_compound, new_tank_name)
			if old_tank_named is not None and new_tank_named is None:
				id = 0
				old_tank_compound = old_tank_named[0]
				old_tank_liquidId_named = remapper.find_named(old_tank_compound, "liquidId")
				if old_tank_liquidId_named is not None:
					old_tank_liquidId_short = old_tank_liquidId_named[0]
					assert old_tank_liquidId_short.tag == "short"
					id = int(old_tank_liquidId_short.get("value"))
				if id != 0:
					old_tank_qty_named = remapper.find_named(old_tank_compound, "qty")
					old_tank_qty_short = old_tank_qty_named[0]
					assert old_tank_qty_short.tag == "short"
					qty = int(old_tank_qty_short.get("value"))
					new_tank_named = xml.etree.ElementTree.SubElement(te_compound, "named", {"name": new_tank_name})
					new_tank_compound = xml.etree.ElementTree.SubElement(new_tank_named, "compound")
					new_tank_id_named = xml.etree.ElementTree.SubElement(new_tank_compound, "named", {"name": "Id"})
					xml.etree.ElementTree.SubElement(new_tank_id_named, "short", {"value": str(id)})
					new_tank_amount_named = xml.etree.ElementTree.SubElement(new_tank_compound, "named", {"name": "Amount"})
					xml.etree.ElementTree.SubElement(new_tank_amount_named, "int", {"value": str(qty)})
					new_tank_capacity_named = xml.etree.ElementTree.SubElement(new_tank_compound, "named", {"name": "capacity"})
					xml.etree.ElementTree.SubElement(new_tank_capacity_named, "int", {"value": "250"})

		# Item pipes have a a "travelingEntities" key containing a list of items.
		# In BC2, each item has an "orientation" number indicating what direction it is travelling.
		# In BC3, each item has an "input" and an "output" number, I guess indicating what direction it was and will be travelling.
		# Let’s be a bit simplistic and just copy orientation to both input and output.
		# That’s probably a little bit derpy for corner pipes, but meh, probably close enough.
		travelingEntities_named = remapper.find_named(te_compound, "travelingEntities")
		if travelingEntities_named is not None:
			travelingEntities_list = travelingEntities_named[0]
			assert travelingEntities_list.tag == "list"
			assert len(travelingEntities_list) == 0 or travelingEntities_list.get("subtype") == "10"
			for traveler_compound in travelingEntities_list:
				item_named = remapper.find_named(traveler_compound, "Item")
				if item_named is not None:
					item_compound = item_named[0]
					assert item_compound.tag == "compound"
					remapper.remap_item_compound(item_compound, map_info)
				orientation_named = remapper.find_named(traveler_compound, "orientation")
				if orientation_named is not None:
					orientation_int = orientation_named[0]
					assert orientation_int.tag == "int"
					orientation = int(orientation_int.get("value"))
					for new_tag in ("input", "output"):
						if remapper.find_named(traveler_compound, new_tag) is None:
							new_named = xml.etree.ElementTree.SubElement(traveler_compound, "named", {"name": new_tag})
							xml.etree.ElementTree.SubElement(new_named, "int", {"value": str(orientation)})

class TankRemapper(remapper.TERemapper):
	def __init__(self):
		remapper.TERemapper.__init__(self, "net.minecraft.src.buildcraft.factory.TileTank")

	def remap_te(self, te_compound, map_info):
		# Tanks identify liquids by an item ID, which needs to be remapped.
		remap_liquid(te_compound, map_info)

		# New tanks use an "Id" key in a "tank" compound instead of a top-level "liquidId" key.
		tank_named = remapper.find_named(te_compound, "tank")
		if tank_named is not None:
			tank_compound = tank_named[0]
			assert tank_compound.tag == "compound"
			remap_liquid(tank_compound, map_info, "Id")

class EngineRemapper(remapper.TERemapper):
	def __init__(self):
		remapper.TERemapper.__init__(self, "net.minecraft.src.buildcraft.energy.Engine")

	def remap_te(self, te_compound, map_info):
		# Combustion engines have three things to deal with: "liquidId" is the item ID of the fuel, "coolantId" is the item ID of the coolant, and "itemInInventory" is the slot.
		# Steam/Stirling engines have only "itemInInventory".
		remap_liquid(te_compound, map_info)
		remap_liquid(te_compound, map_info, "coolantId")

		itemInInventory_named = remapper.find_named(te_compound, "itemInInventory")
		if itemInInventory_named is not None:
			itemInInventory_compound = itemInInventory_named[0]
			assert itemInInventory_compound.tag == "compound"
			remapper.remap_item_compound(itemInInventory_compound, map_info)

		# New-style combustion engines use "fuelTank" and "coolantTank" compounds instead of top-level ID keys, with "Id" keys under them.
		for tank_name in "fuelTank", "coolantTank":
			tank_named = remapper.find_named(te_compound, tank_name)
			if tank_named is not None:
				tank_compound = tank_named[0]
				assert tank_compound.tag == "compound"
				remap_liquid(tank_compound, map_info, "Id")

class FillerRemapper(remapper.TERemapper):
	def __init__(self):
		remapper.TERemapper.__init__(self, "Filler")

	def remap_te(self, te_compound, map_info):
		# BC2 fillers store their items in a variable-length list with each item having a Slot tag.
		# BC3 fillers store their items in a fixed-length list of 9+27=36 elements, one per slot, some of which are empty compounds.
		# We attempt to maximize expected compatibility.
		# On input: gather all the items from the list, ignoring empty compounds, trusting the Slot tag if there is one, otherwise using the item’s position in the list.
		# On output: write out a fixed-size list of 36 elements with the items in the proper places, but also leave Slot tags in them that agree with the position.
		items_named = remapper.find_named(te_compound, "Items")
		if items_named is not None:
			items_list = items_named[0]
			assert items_list.tag == "list"
			assert len(items_list) == 0 or items_list.get("subtype") == "10"
			items = [None] * 36
			for i in range(0, len(items_list)):
				item_compound = items_list[i]
				if len(item_compound) > 0:
					remapper.remap_item_compound(item_compound, map_info)
					slot_named = remapper.find_named(item_compound, "Slot")
					if slot_named is not None:
						slot_byte = slot_named[0]
						assert slot_byte.tag == "byte"
						slot = int(slot_byte.get("value"))
						items[slot] = item_compound
					else:
						slot_named = xml.etree.ElementTree.SubElement(item_compound, "named", {"name": "Slot"})
						xml.etree.ElementTree.SubElement(slot_named, "byte", {"value": str(i)})
						items[i] = item_compound
			items_list.clear()
			items_list.set("subtype", "10")
			for item_compound in items:
				if item_compound is not None:
					items_list.append(item_compound)
				else:
					xml.etree.ElementTree.SubElement(items_list, "compound")

class RefineryRemapper(remapper.TERemapper):
	def __init__(self):
		remapper.TERemapper.__init__(self, "net.minecraft.src.buildcraft.factory.Refinery")

	def remap_te(self, te_compound, map_info):
		# A refinery has three liquid tanks, called "result", "slot1", and "slot2", each having a "liquidId" needing remapping.
		for slot in "result", "slot1", "slot2":
			slot_named = remapper.find_named(te_compound, slot)
			if slot_named is not None:
				slot_compound = slot_named[0]
				assert slot_compound.tag == "compound"
				remap_liquid(slot_compound, map_info)
		# A refinery may alternatively have three tanks called "result", "ingredient1", and "ingredient2", each having and "Id" needing remapping.
		for slot in "result", "ingredient1", "ingredient2":
			slot_named = remapper.find_named(te_compound, slot)
			if slot_named is not None:
				slot_compound = slot_named[0]
				assert slot_compound.tag == "compound"
				remap_liquid(slot_compound, map_info, "Id")
		# A refinery may also have keys "filters_0" and "filters_1" for the in-GUI filter settings, which are top-level and contain liquid IDs directly.
		for key in "filters_0", "filters_1":
			remap_liquid(te_compound, map_info, key)

class ACTRemapper(remapper.SimpleItemContainerTERemapper):
	def __init__(self):
		remapper.SimpleItemContainerTERemapper.__init__(self, "AutoWorkbench", "stackList")

class AssemblyTableRemapper(remapper.TERemapper):
	def __init__(self):
		remapper.TERemapper.__init__(self, "net.minecraft.src.buildcraft.factory.TileAssemblyTable")

	def remap_te(self, te_compound, map_info):
		# The assembly table stores both a collection of input items (in an "items" list) and also a collection of previously selected desired outputs (in a "planned" list).
		# Remap both.
		for items_list_name in "items", "planned":
			items_named = remapper.find_named(te_compound, items_list_name)
			if items_named is not None:
				items_list = items_named[0]
				assert items_list.tag == "list"
				assert len(items_list) == 0 or items_list.get("subtype") == "10"
				for item_compound in items_list:
					remapper.remap_item_compound(item_compound, map_info)

def create_all_remappers():
	return [PipeRemapper("net.minecraft.src.buildcraft.transport.GenericPipe"), PipeRemapper("net.minecraft.src.buildcraft.GenericPipe"), TankRemapper(), EngineRemapper(), FillerRemapper(), RefineryRemapper(), ACTRemapper(), AssemblyTableRemapper()]

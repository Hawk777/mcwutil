#!/bin/env python3

import copy
import xml.etree.ElementTree

import remapper

import buildcraft

# Iron posts are not fully handled.
# In pre-1.4 Railcraft, these are represented as block.structure with a tile entity holding the colour.
# In newer Railcraft, these are represented as block.post for unpainted or block.post.metal with a metadata value holding the colour, eliminating the TE.
# This script doesn’t bother converting, so your metal posts will all turn unpainted.

def convert_liquid_to_tank(compound, old_type_key, old_qty_key, map_info):
	liquid_id = 0;
	liquid_qty = 0;
	# Read liquid from the old structure.
	liquid_int = remapper.find_named_child(compound, old_type_key)
	if liquid_int is not None:
		liquid_id = remapper.get_number_from_number(liquid_int, ("short", "int"))
	tank_int = remapper.find_named_child(compound, old_qty_key)
	if tank_int is not None:
		liquid_qty = remapper.get_number_from_number(tank_int, "int")
	# Read liquid from the new structure.
	tanks_list = remapper.find_named_child(compound, "tanks")
	if tanks_list is not None:
		assert tanks_list.tag == "list"
		if len(tanks_list) != 0:
			assert tanks_list.get("subtype") == "10"
			for tank_compound in tanks_list:
				tank_byte = remapper.find_named_child(tank_compound, "tank")
				if remapper.get_number_from_number(tank_byte, "byte") == 0:
					id_short = remapper.find_named_child(tank_compound, "Id")
					if id_short is not None:
						liquid_id = remapper.get_number_from_number(id_short, "short")
					amount_int = remapper.find_named_child(tank_compound, "Amount")
					if amount_int is not None:
						liquid_qty = remapper.get_number_from_number(amount_int, "int")
	# Delete both liquid structures.
	for name in (old_type_key, old_qty_key, "tanks"):
		named = remapper.find_named(compound, name)
		if named is not None:
			compound.remove(named)
	# If there is liquid, remap it and create the new structure.
	if liquid_id != 0:
		liquid_id = map_info.item_map[liquid_id]
		tank_compound = xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(compound, "named", {"name": "tanks"}), "list", {"subtype": "10"}), "compound")
		xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(tank_compound, "named", {"name": "tank"}), "byte", {"value": "0"})
		xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(tank_compound, "named", {"name": "Id"}), "short", {"value": str(liquid_id)})
		xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(tank_compound, "named", {"name": "Amount"}), "int", {"value": str(liquid_qty)})


# slot_mapping is a list.
# The index of an element in the list is the index of a slot in the old-style "Items" list.
# The element itself is a pair of (name, slot) where name is the name of the compound to contain the item and slot is the slot within the compound.
def shuffle_inventory(compound, slot_mapping, map_info):
	# We will gather the items.
	items = [None] * len(slot_mapping)

	# First try gathering items from the old "Items" list.
	items_list = remapper.find_named_child(compound, "Items")
	if items_list is not None:
		assert items_list.tag == "list"
		assert len(items_list) == 0 or items_list.get("subtype") == "10"
		for item_compound in items_list:
			slot_byte = remapper.find_named_child(item_compound, "Slot")
			if slot_byte is not None:
				slot = remapper.get_number_from_number(slot_byte, "byte")
				if 0 <= slot < len(items):
					items[slot] = item_compound

	# Second try gathering items in the new named lists.
	for slot_name, slot_number in slot_mapping:
		items_list = remapper.find_named_child(compound, slot_name)
		if items_list is not None:
			assert items_list.tag == "list"
			assert len(items_list) == 0 or items_list.get("subtype") == "10"
			for item_compound in items_list:
				slot_byte = remapper.find_named_child(item_compound, "Slot")
				if slot_byte is not None:
					slot = remapper.get_number_from_number(slot_byte, "byte")
					if slot == slot_number:
						items[slot_number] = item_compound

	# Remap the items.
	for item in items:
		if item is not None:
			remapper.remap_item_compound(item, map_info)

	# Delete existing inventory structures.
	for slot_name in set([x for x, y in slot_mapping]) | {"Items"}:
		named = remapper.find_named(compound, slot_name)
		if named is not None:
			compound.remove(named)

	# Create the new-style inventory structures.
	for i in range(0, len(items)):
		slot_name = slot_mapping[i][0]
		slot_number = slot_mapping[i][1]
		item = items[i]
		if item is not None:
			slot_byte = remapper.find_named_child(item, "Slot")
			if slot_byte is None:
				slot_byte = xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(item_compound, "named", {"name": "Slot"}), "byte")
			slot_byte.set("value", str(slot_number))
			inv_list = remapper.find_named_child(compound, slot_name)
			if inv_list is None:
				inv_list = xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(compound, "named", {"name": slot_name}), "list", {"subtype": "10"})
			inv_list.append(item)


# Some Railcraft structures used to have an inventory called "Items", which has now been renamed to something else (often "invStructure", but not always).
# This remapper reads in the inventory layout from either structure and generates an output with both "Items" and the new name in it, after remapping the items in the inventory.
# Occasionally leaving the old "Items" tree around causes weirdness (rolling machine), so optionally allow it to be removed.
class InventoryRemapperAndRenamer(remapper.TERemapper):
	def __init__(self, te_id, new_inventory_name="invStructure", delete_items=False):
		remapper.TERemapper.__init__(self, te_id)
		self.__new_inventory_name = new_inventory_name
		self.__delete_items = delete_items

	def remap_te(self, te_compound, map_info):
		inv_names = (self.__new_inventory_name, "Items")
		inv_lists = [remapper.find_named_child(te_compound, x) for x in inv_names]
		items = []
		for inv_list in inv_lists:
			if inv_list is not None:
				assert inv_list.tag == "list"
				if len(inv_list) != 0:
					assert inv_list.get("subtype") == "10"
					for item_compound in inv_list:
						remapper.remap_item_compound(item_compound, map_info)
						items.append(item_compound)
					break
		for i in range(0, len(inv_names)):
			if inv_lists[i] is None:
				inv_lists[i] = xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(te_compound, "named", {"name": inv_names[i]}), "list")
			inv_lists[i].clear()
			inv_lists[i].set("subtype", "10")
			for item_compound in items:
				inv_lists[i].append(copy.deepcopy(item_compound))
		if self.__delete_items:
			items_named = remapper.find_named(te_compound, "Items")
			if items_named is not None:
				te_compound.remove(items_named)


# In old Railcraft, the coke oven had keys "liquidId" and "liquidQty" for its liquid.
# In new Railcraft, it has a "tanks" list with a compound per tank, with "Id" and "Amount" elements, and a "tank" key set to byte zero.
# We input whichever one we can find, preferring the new format.
# We output both formats after remapping the liquid.
class CokeOvenLiquidRemapper(remapper.TERemapper):
	def __init__(self):
		remapper.TERemapper.__init__(self, "RCCokeOvenTile")

	def remap_te(self, te_compound, map_info):
		convert_liquid_to_tank(te_compound, "liquidId", "liquidQty", map_info)


# Some entities have been renamed.
# This renames them.
class MinecartRenamer(remapper.EntityRemapper):
	def __init__(self, old_name, new_name):
		remapper.EntityRemapper.__init__(self, old_name)
		self.__new_name = new_name

	def remap_entity(self, entity_compound, map_info):
		id_string = remapper.find_named_child(entity_compound, "id")
		id_string.set("value", self.__new_name)


# Tank carts used to have a "Liquid" key for the liquid type and a "Tank" key for the amount; now they have a "tanks" list with a compound containing "tank"=0, "Id", and "Amount".
# They also used to have a three-slot Items list for input, output, and filter slots; they now have a two-slot invBucket list for buckets and a one-slot invFilter list for the filter.
class TankCartRemapper(remapper.EntityRemapper):
	def __init__(self):
		remapper.EntityRemapper.__init__(self, "Railcraft.cart.tank")

	def remap_entity(self, entity_compound, map_info):
		convert_liquid_to_tank(entity_compound, "Liquid", "Tank", map_info)
		shuffle_inventory(entity_compound, [("invFilter", 0), ("invBucket", 0), ("invBucket", 1)], map_info)


# Energy loaders and unloaders need a tag adjusting.
class EnergyLoaderRemapper(remapper.TERemapper):
	def __init__(self, id):
		remapper.TERemapper.__init__(self, id)

	def remap_te(self, te_compound, map_info):
		items_list = remapper.find_named_child(te_compound, "Items")
		if items_list is not None:
			assert items_list.tag == "list"
			assert len(items_list) == 0 or items_list.get("subtype") == "10"
			for item in items_list:
				remapper.remap_item_compound(item, map_info)
		facing_short = remapper.find_named_child(te_compound, "facing")
		direction_byte = remapper.find_named_child(te_compound, "direction")
		if facing_short is not None and direction_byte is None:
			facing = remapper.get_number_from_number(facing_short, "short")
			direction_byte = xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(te_compound, "named", {"name": "direction"}), "byte", {"value": str(facing)})
			te_compound.remove(remapper.find_named(te_compound, "facing"))


def create_all_remappers():
	ret = []
	# Handle multiblock structures.
	ret += [InventoryRemapperAndRenamer(id) for id in ("RCCokeOvenTile", "RCBlastFurnaceTile", "RCRockCrusherTile")]
	ret.append(InventoryRemapperAndRenamer("RCRollingMachineTile", "Crafting", True))
	ret.append(CokeOvenLiquidRemapper())
	# Handle minecarts being renamed.
	ret += [MinecartRenamer(old, new) for (old, new) in (("entity.cart.basic", "Railcraft.cart.basic"), ("entity.cart.furnace", "Railcraft.cart.furnace"), ("entity.cart.chest", "Railcraft.cart.chest"), ("Tankcart", "Railcraft.cart.tank"), ("EnergyCart", "Railcraft.cart.energy"), ("Workcart", "Railcraft.cart.work"), ("Anchorcart", "Railcraft.cart.anchor"), ("TNTcart", "Railcraft.cart.tnt"))]
	# Handle various carts needing their contents remapped.
	ret += [remapper.SimpleItemContainerEntityRemapper(x) for x in ("Railcraft.cart.chest", "Railcraft.cart.energy")]
	# Handle tank carts representing their contents differently and their contents needing remapping.
	ret.append(TankCartRemapper())
	# Handle energy loaders and unloaders needing their contents remapped and some other tags changed.
	ret.append(EnergyLoaderRemapper("RCLoaderTileEnergy"))
	ret.append(EnergyLoaderRemapper("RCUnloaderTileEnergy"))
	return ret

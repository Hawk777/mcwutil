#!/bin/env python3

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
	if remapper.find_named_child(compound, "tanks") is None:
		# No new structure, so convert 1.2.5 to 1.4.7.
		# Read liquid from the old structure.
		liquid_int = remapper.find_named_child(compound, old_type_key)
		if liquid_int is not None:
			liquid_id = remapper.get_number_from_number(liquid_int, ("short", "int"))
		tank_int = remapper.find_named_child(compound, old_qty_key)
		if tank_int is not None:
			liquid_qty = remapper.get_number_from_number(tank_int, "int")
		if liquid_id != 0:
			# Create new structure.
			tank_compound = xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(compound, "named", {"name": "tanks"}), "list", {"subtype": "10"}), "compound")
			xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(tank_compound, "named", {"name": "tank"}), "byte", {"value": "0"})
			xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(tank_compound, "named", {"name": "Id"}), "short", {"value": str(liquid_id)})
			xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(tank_compound, "named", {"name": "Amount"}), "int", {"value": str(liquid_qty)})
			# Delete old structure.
			for name in (old_type_key, old_qty_key):
				named = remapper.find_named(compound, name)
				if named is not None:
					compound.remove(named)
	# If there is a 1.4.7-era tanks list, remap its IDs.
	tanks_list = remapper.find_named_child(compound, "tanks")
	if tanks_list is not None:
		assert tanks_list.tag == "list"
		if len(tanks_list) != 0:
			assert tanks_list.get("subtype") == "10"
			for tank_compound in tanks_list:
				id_short = remapper.find_named_child(tank_compound, "Id")
				if id_short is not None:
					id_short.set("value", str(map_info.item_map[remapper.get_number_from_number(id_short, "short")]))


# slot_mapping is a list.
# The index of an element in the list is the index of a slot in the old-style "Items" list.
# The element itself is a pair of (name, slot) where name is the name of the compound to contain the item and slot is the slot within the compound.
def shuffle_inventory(compound, slot_mapping, map_info):
	# We will gather the items.
	items = [None] * len(slot_mapping)

	# First try gathering items from the old "Items" list, if the new stuff doesn’t exist.
	new_exists = False
	for elt in slot_mapping:
		if remapper.find_named(compound, elt[0]) is not None:
			new_exists = True
	if not new_exists:
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
# This remapper reads in the inventory layout from either structure and generates an output with the new name in it, after remapping the items in the inventory.
class InventoryRemapperAndRenamer(remapper.TERemapper):
	def __init__(self, te_id, new_inventory_name="invStructure"):
		remapper.TERemapper.__init__(self, te_id)
		self.__new_inventory_name = new_inventory_name

	def remap_te(self, te_compound, map_info):
		inv_names = (self.__new_inventory_name, "Items")
		inv_lists = [remapper.find_named_child(te_compound, x) for x in inv_names]
		items = None
		for inv_list in inv_lists:
			if inv_list is not None and items is None:
				items = []
				assert inv_list.tag == "list"
				if len(inv_list) != 0:
					assert inv_list.get("subtype") == "10"
					for item_compound in inv_list:
						remapper.remap_item_compound(item_compound, map_info)
						items.append(item_compound)
					break
		for name in inv_names:
			inv_named = remapper.find_named(te_compound, name)
			if inv_named is not None:
				te_compound.remove(inv_named)
		inv_named = xml.etree.ElementTree.SubElement(te_compound, "named", {"name": self.__new_inventory_name})
		inv_list = xml.etree.ElementTree.SubElement(inv_named, "list", {"subtype": "10"})
		for item in items:
			inv_list.append(item)


# In Railcraft for 1.2.5, the coke oven had keys "liquidId" and "liquidQty" for its liquid.
# In Railcraft for 1.4.7, it had a "tanks" list with a compound per tank, with "Id" and "Amount" elements, and a "tank" key set to byte zero.
# In Railcraft for 1.6.4, it has a "tanks" list using liquid names.
class LiquidRemapper(remapper.TERemapper):
	def __init__(self, id):
		remapper.TERemapper.__init__(self, id)

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
		remapper.EntityRemapper.__init__(self, "Railcraft.railcraft.cart.tank")

	def remap_entity(self, entity_compound, map_info):
		convert_liquid_to_tank(entity_compound, "Liquid", "Tank", map_info)
		# These went from a single big Items list in 1.2.5 to an invFilter/invBucket group in 1.4.7.
		shuffle_inventory(entity_compound, [("invFilter", 0), ("invBucket", 0), ("invBucket", 1)], map_info)
		# Then invBucket went back to Items in 1.6.4, leaving invFilter separate!
		invBucket_named = remapper.find_named(entity_compound, "invBucket")
		if invBucket_named is not None:
			invBucket_named.set("name", "Items")


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


# Random stuff changes in track once in a while.
class TrackRemapper(remapper.TERemapper):
	def __init__(self):
		remapper.TERemapper.__init__(self, "RailcraftTrackTile")

	def remap_te(self, te_compound, map_info):
		trackId_int = remapper.find_named_child(te_compound, "trackId")
		if trackId_int is not None:
			# In Railcraft 6.17.0.0, coupler track was ID 22 and decoupler track was ID 23.
			# In Railcraft 8.4.0.0, both are ID 22 and there is a "decouple" byte which is 0 or 1.
			if trackId_int.get("value") == "23":
				trackId_int.set("value", "22")
				xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(te_compound, "named", {"name": "decouple"}), "byte", {"value": "1"})


def create_all_remappers():
	ret = []
	# Handle multiblock structures.
	ret += [InventoryRemapperAndRenamer(id) for id in ("RCCokeOvenTile", "RCBlastFurnaceTile", "RCRockCrusherTile", "RCSteamOvenTile", "RCWaterTankTile")]
	ret.append(InventoryRemapperAndRenamer("RCRollingMachineTile", "Crafting"))
	ret += [InventoryRemapperAndRenamer(id, "inv") for id in ("RCIronTankWallTile", "RCIronTankGaugeTile", "RCIronTankValveTile", "RCBoilerFireboxLiquidTile", "RCBoilerFireboxSolidTile")]
	ret.append(LiquidRemapper("RCCokeOvenTile"))
	ret += [LiquidRemapper(id) for id in ("RCIronTankWallTile", "RCIronTankGaugeTile", "RCIronTankValveTile", "RCWaterTankTile")]
	# Handle minecarts being renamed.
	ret += [MinecartRenamer(old, new) for (old, new) in (("entity.cart.basic", "Railcraft.cart.basic"), ("entity.cart.furnace", "Railcraft.cart.furnace"), ("entity.cart.chest", "railcraft.railcraft.cart.chest"), ("Railcraft.cart.chest", "Railcraft.railcraft.cart.chest"), ("Tankcart", "Railcraft.railcraft.cart.tank"), ("Railcraft.cart.tank", "Railcraft.railcraft.cart.tank"), ("EnergyCart", "Railcraft.cart.energy"), ("Workcart", "Railcraft.cart.work"), ("Anchorcart", "Railcraft.cart.anchor"), ("TNTcart", "Railcraft.cart.tnt"), ("Railcraft.cart.track.relayer", "Railcraft.railcraft.cart.track.relayer"), ("Railcraft.cart.undercutter", "Railcraft.railcraft.cart.undercutter"))]
	# Handle various carts needing their contents remapped.
	ret += [remapper.SimpleItemContainerEntityRemapper(x) for x in ("Railcraft.railcraft.cart.chest", "Railcraft.railcraft.cart.tank", "Railcraft.railcraft.cart.energy", "Railcraft.railcraft.cart.track.relayer", "Railcraft.railcraft.cart.undercutter")]
	ret.append(remapper.SimpleItemContainerEntityRemapper("Railcraft.railcraft.cart.track.relayer", "patternInv"))
	ret.append(remapper.SimpleItemContainerEntityRemapper("Railcraft.railcraft.cart.undercutter", "patternInv"))
	# Handle tank carts representing their contents differently and their contents needing remapping.
	ret.append(TankCartRemapper())
	# Handle energy loaders and unloaders needing their contents remapped and some other tags changed.
	ret.append(EnergyLoaderRemapper("RCLoaderTileEnergy"))
	ret.append(EnergyLoaderRemapper("RCUnloaderTileEnergy"))
	# Handle various crud changing in track tile entities.
	ret.append(TrackRemapper())
	return ret

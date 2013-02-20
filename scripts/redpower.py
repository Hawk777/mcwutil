#!/bin/env python3

import remapper

# This class ensures that ID remapping is applied to not only the ordinary Items array but also the Buffer array, which contains backstuffs from an attached tube network.
class TubeMachineRemapper(remapper.SimpleItemContainerTERemapper):
	def __init__(self, id):
		remapper.SimpleItemContainerTERemapper.__init__(self, id)

	def remap_te(self, te_compound, map_info):
		remapper.SimpleItemContainerTERemapper.remap_te(self, te_compound, map_info)
		buffer_list = remapper.find_named_child(te_compound, "Buffer")
		if buffer_list is not None:
			if buffer_list.tag == "list" and buffer_list.get("subtype") == "10":
				for item_compound in buffer_list:
					remapper.remap_item_compound(item_compound, map_info)


# After 1.2.5 and before 1.4.6, the project table was enhanced to use plans.
# This added a new slot to the inventory, which was inserted at position 9 in the NBT.
# Therefore, all slots 9 and greater must be incremented to maintain proper position in the bulk storage area.
# This is not included in the default set of RedPower remappers because there is no way to detect if it has already been done or not.
# Therefore, running it arbitrarily would be bad as it would shift the bulk storage area on every run!
class ProjectTablePlanSlotShifter(remapper.TERemapper):
	def __init__(self):
		remapper.TERemapper.__init__(self, "RPAdvBench")

	def remap_te(self, te_compound, map_info):
		items_list = remapper.find_named_child(te_compound, "Items")
		if items_list is not None:
			assert items_list.tag == "list"
			assert len(items_list) == 0 or items_list.get("subtype") == "10"
			for item in items_list:
				slot_byte = remapper.find_named_child(item, "Slot")
				if slot_byte is not None:
					slot = remapper.get_number_from_number(slot_byte, "byte")
					if slot >= 9:
						slot_byte.set("value", str(slot + 1))


def create_all_remappers():
	ret = []
	ret += [TubeMachineRemapper(x) for x in ("RPDeploy", "RPAssemble", "RPAFurnace", "RPItemDet", "RPEject", "RPFilter", "RPBatBox", "RPBFurnace", "RPRetrieve", "RPBuffer", "RPRelay", "RPSorter", "RPAdvBench", "RPBAFurnace", "RPRegulate", "RPConDDrv")]
	ret += [remapper.SimpleItemContainerTERemapper(x) for x in ("RPTube", "RPMTube", "RPAccel")]
	return ret

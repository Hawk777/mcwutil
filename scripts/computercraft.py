#!/bin/env python3

import remapper

class DiskDriveRemapper(remapper.TERemapper):
	def __init__(self):
		remapper.TERemapper.__init__(self, "diskdrive")

	def remap_te(self, te_compound, map_info):
		item_named = remapper.find_named(te_compound, "item")
		if item_named is not None:
			item_compound = item_named[0]
			assert item_compound.tag == "compound"
			remapper.remap_item_compound(item_compound, map_info)

class TurtleRemapper(remapper.SimpleItemContainerTERemapper):
	def __init__(self):
		remapper.SimpleItemContainerTERemapper.__init__(self, "turtle")

def create_all_remappers():
	return [DiskDriveRemapper(), TurtleRemapper()]

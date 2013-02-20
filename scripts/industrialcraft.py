#!/bin/env python3

import remapper

class CropnalyzerRemapper(remapper.Remapper):
	def __init__(self):
		remapper.Remapper.__init__(self)

	def remap_item(self, item_compound, map_info):
		# There isn’t actually a way to figure out whether an item is really a cropnalyzer.
		# However, a cropnalyzer has a "tag" compound containing an "Items" list, so just look for that.
		# Nobody other than IC2 seems to be crazy enough to have a compound called "tag" lying around.
		tag_named = remapper.find_named(item_compound, "tag")
		if tag_named is not None:
			tag_compound = tag_named[0]
			if tag_compound.tag == "compound":
				items_named = remapper.find_named(tag_compound, "Items")
				if items_named is not None:
					items_list = items_named[0]
					if items_list.tag == "list" and items_list.get("subtype") == "10":
						for inner_item_compound in items_list:
							remapper.remap_item_compound(inner_item_compound, map_info)

def create_all_remappers():
	ret = [remapper.SimpleItemContainerTERemapper(x) for x in ("Crop-Matron", "BatBox", "MFE", "MFS", "Electrolyzer", "Miner", "Pump", "Generator", "Geothermal Generator", "Nuclear Reactor", "Solar Panel", "Water Mill", "Wind Mill", "Canning Machine", "Compressor", "Electric Furnace", "Extractor", "Induction Furnace", "Iron Furnace", "Macerator", "Mass Fabricator", "Recycler", "Personal Safe", "Terraformer", "Trade-O-Mat")]
	ret.append(CropnalyzerRemapper())
	return ret

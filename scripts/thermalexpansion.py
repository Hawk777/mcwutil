#!/bin/env python3

import xml.etree.ElementTree

import remapper

import buildcraft


class MachineRemapper147164(remapper.SimpleItemContainerTERemapper):
	def __init__(self, oldid, newid):
		remapper.SimpleItemContainerTERemapper.__init__(self, oldid, "Inventory")
		self._newid = newid

	def remap_te(self, te_compound, map_info):
		# Machines’ tile entity IDs have changed.
		id_string = remapper.find_named_child(te_compound, "id")
		assert id_string.tag == "string"
		id_string.set("value", self._newid)
		# Some keys in the compound have changed name but structure.
		for old, new in (("inventory", "Inventory"), ("side.array", "SideCache"), ("side.facing", "Facing"), ("flag.active", "Active")):
			named = remapper.find_named(te_compound, old)
			if named is not None:
				named.set("name", new)
		# In 1.4.7, each inventory item has a “slot” key.
		# In 1.6.4, it is called “Slot”.
		inventory_list = remapper.find_named_child(te_compound, "Inventory")
		if inventory_list is not None:
			assert inventory_list.tag == "list"
			assert len(inventory_list) == 0 or inventory_list.get("subtype") == "10"
			for item_compound in inventory_list:
				assert item_compound.tag == "compound"
				slot_named = remapper.find_named(item_compound, "slot")
				if slot_named is not None:
					slot_named.set("name", "Slot")
		# In 1.4.7, redstone control is red.state and red.disable.
		# In 1.6.4, redstone control is RS/Disable and RS/Setting.
		rs_compound = remapper.find_named_child(te_compound, "RS")
		if rs_compound is None:
			rs_compound = xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(te_compound, "named", {"name": "RS"}), "compound")
		for old, new in (("red.disable", "Disable"), ("red.state", "Setting")):
			old_named = remapper.find_named(te_compound, old)
			if old_named is not None:
				te_compound.remove(old_named)
				old_named.set("name", new)
				rs_compound.append(old_named)
		# Remap the inventory.
		remapper.SimpleItemContainerTERemapper.remap_te(self, te_compound, map_info)


class EnergyConduitRemapper147164(remapper.TERemapper):
	def __init__(self):
		remapper.TERemapper.__init__(self, "thermalexpansion.transport.ConduitEnergy")

	def remap_te(self, te_compound, map_info):
		# This tile entity is now implemented as a multipart TE with the conduit inside the multipart.
		coords = [remapper.find_named(te_compound, name) for name in ("z", "y", "x")]
		assert None not in coords
		side_barray = remapper.find_named_child(te_compound, "side.array")
		assert side_barray.tag == "barray"
		while len(te_compound) > 0:
			del te_compound[0]
		xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(te_compound, "named", {"name": "id"}), "string", {"value": "savedMultipart"})
		mp_compound = xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(te_compound, "named", {"name": "parts"}), "list", {"subtype": "10"}), "compound")
		xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(mp_compound, "named", {"name": "id"}), "string", {"value": "ConduitEnergy0"})
		rs_compound = xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(mp_compound, "named", {"name": "RS"}), "compound")
		for key, value in (("Disable", "0"), ("Setting", "1"), ("Powered", "0")):
			xml.etree.ElementTree.SubElement(xml.etree.ElementTree.SubElement(rs_compound, "named", {"name": key}), "byte", {"value": value})
		for key, type, value in (("Tracker", "byte", "0"), ("Energy", "int", "0"), ("SideType", "barray", side_barray.text), ("hasServo", "byte", "0"), ("Mode", "byte", "0"), ("SubType", "byte", "0"), ("SideMode", "barray", "010101010101")):
			mp_child_named = xml.etree.ElementTree.SubElement(mp_compound, "named", {"name": key})
			if type == "barray":
				xml.etree.ElementTree.SubElement(mp_child_named, "barray").text = value
			else:
				xml.etree.ElementTree.SubElement(mp_child_named, type, {"value": value})
		for coord in coords:
			te_compound.append(coord)


def create_all_remappers():
	ret = []
	# Handle machine inventories, in the 1.4.7 naming convention.
	ret += [MachineRemapper147164("thermalexpansion.factory.{}".format(machine), "cofh.thermalexpansion.{}".format(machine)) for machine in ("RockGen", "Furnace", "Smelter", "Pulverizer", "IceGen", "WaterGen", "Crucible", "Sawmill")]
	ret.append(MachineRemapper147164("thermalexpansion.factory.Filler", "cofh.thermalexpansion.Transposer"))
	ret.append(EnergyConduitRemapper147164())
	return ret

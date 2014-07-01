#!/bin/env python3

import array
import binascii
import gzip
import os.path
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree
import zlib

class ConfigFile(object):
	def load(base_dir, file_info):
		config_filename = os.path.join(base_dir, file_info[1])
		config_type = file_info[0]
		print("Loading config file at {} of type {}…".format(config_filename, config_type))
		if config_type == "ini":
			return ConfigFileIni(config_filename)
		elif config_type == "hier-noprefix":
			return ConfigFileHierNoPrefix(config_filename)
		elif config_type == "hier-prefix":
			return ConfigFileHierPrefix(config_filename)


	def auto_blocks(self):
		return None

	def auto_items(self):
		return None


class ConfigFileIni(ConfigFile):
	def __init__(self, filename):
		ConfigFile.__init__(self)
		self.__values = {}
		with open(filename, mode="rt") as fp:
			for line in fp:
				line = line.strip()
				if line != "" and not line.startswith("#"):
					parts = line.split("=")
					if len(parts) == 2:
						key = parts[0].strip()
						try:
							value = int(parts[1].strip())
							self.__values[key] = value
						except ValueError:
							pass
					else:
						print("Malformed INI line (too many equals signs): {}".format(line))
						sys.exit(1)

	def get_block(self, name):
		return self.__values.get(name)

	def get_item(self, name):
		return self.__values.get(name)


class ConfigFileHierNoPrefix(ConfigFile):
	def __init__(self, filename):
		ConfigFile.__init__(self)
		self.__values = {}
		self.blockSection = None
		self.itemSection = None
		current_path = []
		with open(filename, mode="rt") as fp:
			in_angle_brackets = False
			for line in fp:
				line = line.strip()
				if line != "" and not line.startswith("#"):
					if in_angle_brackets:
						if line.endswith(">"):
							in_angle_brackets = False
					else:
						if line.endswith("{"):
							section = line[0:-1].strip()
							if len(current_path) == 0 and (section == "block" or section == "blocks"):
								if self.blockSection is None:
									self.blockSection = section
								else:
									print("Malformed hierarchical config file (multiple block sections)!")
									sys.exit(1)
							if len(current_path) == 0 and (section == "item" or section == "items"):
								if self.itemSection is None:
									self.itemSection = section
								else:
									print("Malformed hierarchical config file (multiple item sections)!")
									sys.exit(1)
							current_path.append(section)
						elif line == "}":
							current_path.pop()
						else:
							parts = line.split("=")
							if len(parts) == 2:
								key = self.fix_key(parts[0].strip())
								if key is not None:
									key = "/".join(current_path) + "/" + key
									try:
										value = int(parts[1].strip())
										self.__values[key] = value
									except ValueError:
										pass
							elif len(parts) == 1 and line.endswith("<"):
								in_angle_brackets = True
							else:
								print("Malformed hierarchical config file (too many equals signs): {}".format(line))
								sys.exit(1)

	def fix_key(self, key):
		return key

	def auto_blocks(self):
		if self.blockSection is not None:
			return [x[len(self.blockSection) + 1:] for x in self.__values if x.startswith(self.blockSection + "/")]
		else:
			return []

	def auto_items(self):
		if self.itemSection is not None:
			return [x[len(self.itemSection) + 1:] for x in self.__values if x.startswith(self.itemSection + "/")]
		else:
			return []

	def get_block(self, name):
		if self.blockSection is not None:
			return self.__values.get(self.blockSection + "/" + name)
		else:
			return None

	def get_item(self, name):
		if self.itemSection is not None:
			return self.__values.get(self.itemSection + "/" + name)
		else:
			return None


class ConfigFileHierPrefix(ConfigFileHierNoPrefix):
	def __init__(self, filename):
		ConfigFileHierNoPrefix.__init__(self, filename)

	def fix_key(self, key):
		if key[0:2] == "I:":
			return key[2:]
		else:
			return None


class MapInfo(object):
	def __init__(self, input_base_dir, output_base_dir, vanilla_block_ranges, vanilla_item_ranges, mod_info):
		# Save locals.
		self.vanilla_block_ranges = vanilla_block_ranges
		self.vanilla_item_ranges = vanilla_item_ranges
		self.mod_info = mod_info
		self.block_map = {}
		self.item_map = {}

		# Initialize the mods.
		for mod_name, mod in mod_info.items():
			# Display progress.
			print("===== GENERATING MAPPING RULES FOR MOD {} =====".format(mod_name))
			# Add missing keys.
			if "block" not in mod:
				mod["block"] = {}
			if "item" not in mod:
				mod["item"] = {}
			# Load the config files.
			old_config = ConfigFile.load(input_base_dir, mod["files"]["old"])
			new_config = ConfigFile.load(output_base_dir, mod["files"]["new"])
			# Build the maps.
			print("Building block map…")
			self.__build_mod_map(self.block_map, old_config, new_config, mod, False)
			print("Building item map…")
			self.__build_mod_map(self.item_map, old_config, new_config, mod, True)
			print()

		# Initialize the vanilla blocks and items.
		print("Generating vanilla mappings… ", end="")
		try:
			num_blocks = 0
			num_items = 0
			for r in vanilla_block_ranges:
				for block in range(r[0], r[1] + 1):
					self.block_map[block] = block
					num_blocks += 1
			for r in vanilla_item_ranges:
				for item in range(r[0], r[1] + 1):
					self.item_map[item] = item
					num_items += 1
			print("{} block(s), {} item(s).".format(num_blocks, num_items), end="")
		finally:
			print()

		# Add all the blocks into the item map, since (to a sufficiently good approximation) all blocks are also items.
		for k, v in self.block_map.items():
			self.item_map[k] = v
		print("Added {} blocks to the item map.".format(len(self.block_map)))

	def __build_mod_map(self, output_map, old_config, new_config, mod, do_items):
		if not do_items:
			# Blocks
			automatic = old_config.auto_blocks()
			configured = mod["block"].keys()
			configured_with_damage = {}
		else:
			# Items
			automatic = old_config.auto_items()
			configured = [k for k in mod["item"].keys() if isinstance(k, str)]
			configured_with_damage = {}
			for k in mod["item"].keys():
				if isinstance(k, tuple):
					item_name = k[0]
					item_damage = k[1]
					damage_list = configured_with_damage.setdefault(item_name, [])
					damage_list.append(item_damage)
			for damage_values in configured_with_damage.values():
				damage_values.sort()

		if automatic is None:
			# No automatic elements; we must explicitly map exactly each configured element and nothing else.
			print("Using only manually-configured elements.")
			keys_to_map = configured
		else:
			# Automatic elements present; we must map as many as possible of the automatic elements, and the configured elements will possibly provide mapping details.
			print("Using automatic elements.")
			keys_to_map = automatic
			# Even if we are doing automatic, we must specifically include any manually-configured mappings whose sources are integers, as they will necessarily not be automatically found.
			for key in mod["item"] if do_items else mod["block"]:
				if isinstance(key, int):
					keys_to_map.append(key)

		for key_to_map in keys_to_map:
			try:
				print("Mapping {}… ".format(key_to_map), end="")
				# See if this is an item with damage-value-specific configuration directives.
				damage_values = configured_with_damage.get(key_to_map)
				if damage_values is not None:
					print("per-damage-value config directive(s) found; mapping items…", end="")
					damage_map = {}
					for damage in damage_values:
						target_name_and_damage = mod["item"][(key_to_map, damage)]
						source_id = old_config.get_item(key_to_map) + 256
						if isinstance(target_name_and_damage[0], int):
							target_id = target_name_and_damage[0]
						else:
							target_id = new_config.get_item(target_name_and_damage[0])
							if target_id is not None:
								target_id += 256
							else:
								target_id = new_config.get_block(target_name_and_damage[0])
						damage_map[damage] = (target_id, target_name_and_damage[1])
						print(" {}".format(damage), end="")
					output_map[source_id] = damage_map
					print("… {} damage value(s) mapped.".format(len(damage_values)), end="")
				else:
					# Try to find a configuration directive that applies to this key.
					config_directive = self.__find_configuration_for_key(key_to_map, configured)
					print("best matching config directive is {}… ".format(config_directive), end="")
					if config_directive is None:
						target_name = key_to_map
					elif isinstance(config_directive, int):
						target_name = mod["item" if do_items else "block"][config_directive]
					else:
						target_name = mod["item" if do_items else "block"][config_directive] + key_to_map[len(config_directive):]
					print("target name is {}… ".format(target_name), end="")
					if do_items:
						source_id = key_to_map if isinstance(key_to_map, int) else (old_config.get_item(key_to_map) + 256)
						if isinstance(target_name, int):
							target_id = target_name
						else:
							target_id = new_config.get_item(target_name)
							if target_id is not None:
								target_id += 256
							else:
								target_id = new_config.get_block(target_name)
					else:
						source_id = key_to_map if isinstance(key_to_map, int) else old_config.get_block(key_to_map)
						target_id = target_name if isinstance(target_name, int) else new_config.get_block(target_name)
					if target_id is not None:
						output_map[source_id] = target_id
						print("{} → {}… ".format(source_id, target_id), end="")
						if do_items:
							print("all damage values mapped.", end="")
						else:
							output_map[source_id] = target_id
							print("block mapped.", end="")
					else:
						print("missing from new config file → no mapping created → assuming absent from world.", end="")
			finally:
				print()

	def __find_configuration_for_key(self, key, configured):
		if isinstance(key, int):
			return key
		parts = key.split(".")
		for length in range(len(parts), 0, -1):
			prefix = ".".join(parts[0:length])
			if prefix in configured:
				return prefix
		return None


def find_named(parent, name):
	for child in parent:
		assert child.tag == "named"
		if child.get("name") == name:
			return child
	return None


def find_named_child(parent, name):
	named = find_named(parent, name)
	if named is not None:
		return named[0]
	else:
		return None


def get_number_from_number(number, legal_types):
	if isinstance(legal_types, str):
		legal_types = [legal_types]
	assert number.tag in legal_types
	return int(number.get("value"))


def get_number_from_named(named, legal_types):
	child = named[0]
	return get_number_from_number(child, legal_types)


def find_tile_entities_by_id(chunk, id):
	# Tile entities are located at minecraft-nbt/named[name=""]/compound/named[name="Level"]/compound/named[name="TileEntities"]/list[subtype="10"]/compound (one per TE).
	minecraft_nbt = chunk.getroot()
	root_compound = find_named_child(minecraft_nbt, "")
	level_compound = find_named_child(root_compound, "Level")
	tileentities_list = find_named_child(level_compound, "TileEntities")
	assert tileentities_list.tag == "list"
	assert len(tileentities_list) == 0 or tileentities_list.get("subtype") == "10"
	ret = []
	for te_compound in tileentities_list:
		id_string = find_named_child(te_compound, "id")
		assert id_string.tag == "string"
		if id_string.get("value") == id:
			ret.append(te_compound)
	return ret


def find_entities_by_id(chunk, id):
	# Entities are located at minecraft-nbt/named[name=""]/compound/named[name="Level"]/compound/named[name="Entities"]/list[subtype="10"]/compound (one per TE).
	minecraft_nbt = chunk.getroot()
	root_compound = find_named_child(minecraft_nbt, "")
	level_compound = find_named_child(root_compound, "Level")
	entities_list = find_named_child(level_compound, "Entities")
	assert entities_list.tag == "list"
	assert len(entities_list) == 0 or entities_list.get("subtype") == "10"
	ret = []
	for entity_compound in entities_list:
		id_string = find_named_child(entity_compound, "id")
		assert id_string.tag == "string"
		if id_string.get("value") == id:
			ret.append(entity_compound)
	return ret


def remap_item_compound(item_compound, map_info):
	# Grab the ID and, if present, damage value.
	id_short = find_named_child(item_compound, "id")
	if id_short is not None:
		id = get_number_from_number(id_short, "short")
		damage_short = find_named_child(item_compound, "Damage")
		if damage_short is not None:
			damage = get_number_from_number(damage_short, "short")
		else:
			damage = None
		# Look up the mapping.
		target = map_info.item_map.get(id)
		if target is None:
			print("Item {} has no mapping!".format(id))
			sys.exit(1)
		elif isinstance(target, dict):
			if damage is None:
				print("Item {} has no damage value but mapping is damage-specific!".format(id))
				sys.exit(1)
			if damage not in target:
				print("Item {} has damage value {} not present in mapping!".format(id, damage))
				sys.exit(1)
			target_elt = target[damage]
			id = target_elt[0]
			damage = target_elt[1]
		else:
			id = target
		# Modify the elements.
		id_short.set("value", str(id))
		if damage is not None:
			damage_short.set("value", str(damage))
		# Let any other remappers do things to the item (e.g. nested containers).
		for remapper in map_info.remappers:
			remapper.remap_item(item_compound, map_info)


class Remapper(object):
	def __init__(self):
		pass

	def remap_chunk(self, chunk, map_info):
		pass

	def remap_item(self, item_compound, map_info):
		pass

	def remap_player(self, player, map_info):
		pass

	def remap_liquid(self, liquid, map_info):
		return liquid


class EntityRemapper(Remapper):
	def __init__(self, entity_id):
		Remapper.__init__(self)
		self.__entity_id = entity_id

	def remap_chunk(self, chunk, map_info):
		# Get all the relevant entities.
		for entity_compound in find_entities_by_id(chunk, self.__entity_id):
			self.remap_entity(entity_compound, map_info)

	def remap_entity(self, entity_compound, map_info):
		pass


class TERemapper(Remapper):
	def __init__(self, te_id):
		Remapper.__init__(self)
		self.__te_id = te_id
	
	def remap_chunk(self, chunk, map_info):
		# Get all the relevant tile entities.
		for te_compound in find_tile_entities_by_id(chunk, self.__te_id):
			self.remap_te(te_compound, map_info)

	def remap_te(self, te_compound, map_info):
		pass


class WorldBlockRemapper(Remapper):
	def __init__(self):
		Remapper.__init__(self)
		self.__whitespace_clear_map = str.maketrans("", "", "\n\r\t ")

	def remap_chunk(self, chunk, map_info):
		# There are a set of slices located at minecraft-nbt/named[name=""]/compound/named[name="Level"]/compound/named[name="Sections"]/list[subtype="10"]/compound (one per slice).
		# Iterate them.
		minecraft_nbt = chunk.getroot()
		root_compound = find_named_child(minecraft_nbt, "")
		level_compound = find_named_child(root_compound, "Level")
		sections_list = find_named_child(level_compound, "Sections")
		assert sections_list.tag == "list"
		assert len(sections_list) == 0 or sections_list.get("subtype") == "10"
		for section_compound in sections_list:
			# Found a section!
			assert section_compound.tag == "compound"
			# Haul out the Blocks and Add elements.
			blocks_named_elt = find_named(section_compound, "Blocks")
			add_named_elt = find_named(section_compound, "Add")
			blocks_elt = blocks_named_elt[0]
			add_elt = add_named_elt[0] if add_named_elt is not None else None
			assert blocks_elt.tag == "barray"
			assert add_elt is None or add_elt.tag == "barray"
			# Build a blocks array.
			if add_elt is None:
				blocks = array.array("H", list(bytes.fromhex(blocks_elt.text.translate(self.__whitespace_clear_map))))
				assert len(blocks) == 16 * 16 * 16
			else:
				blocks = array.array("H")
				blocks_bytes = bytes.fromhex(blocks_elt.text.translate(self.__whitespace_clear_map))
				add_bytes = bytes.fromhex(add_elt.text.translate(self.__whitespace_clear_map))
				for i in range(0, 16 * 16 * 16, 2):
					# Blocks is one byte per block.
					# Add is one nybble per block.
					# Add is stored little-endian; that is, the least significant nybble in a byte is part of the earlier block.
					lsb = blocks_bytes[i]
					msn = add_bytes[i // 2] & 0x0F
					blocks.append((msn << 8) | lsb)
					lsb = blocks_bytes[i + 1]
					msn = add_bytes[i // 2] >> 4
					blocks.append((msn << 8) | lsb)
				assert len(blocks) == 16 * 16 * 16
			# Perform the remapping.
			any_high = False
			for i in range(0, 16 * 16 * 16):
				old_block_id = blocks[i]
				new_block_id = map_info.block_map.get(old_block_id)
				if new_block_id is not None:
					blocks[i] = new_block_id
					if new_block_id > 255:
						any_high = True
				else:
					print("\nUnhandled block ID {}!".format(old_block_id))
					sys.exit(1)
			# Delete the old elements.
			section_compound.remove(blocks_named_elt)
			if add_named_elt is not None:
				section_compound.remove(add_named_elt)
			blocks_named_elt = None
			blocks_elt = None
			add_named_elt = None
			add_elt = None
			# Create the new elements.
			blocks_named_elt = xml.etree.ElementTree.SubElement(section_compound, "named", {"name": "Blocks"})
			blocks_elt = xml.etree.ElementTree.SubElement(blocks_named_elt, "barray")
			blocks_elt.text = str(binascii.hexlify(bytes([x & 0xFF for x in blocks])), "ASCII").upper()
			if any_high:
				lst = []
				for i in range(0, 16 * 16 * 16, 2):
					lst.append((blocks[i] >> 8) | ((blocks[i + 1] >> 8) << 4))
				add_named_elt = xml.etree.ElementTree.SubElement(section_compound, "named", {"name": "Add"})
				add_elt = xml.etree.ElementTree.SubElement(add_named_elt, "barray")
				add_elt.text = str(binascii.hexlify(bytes(lst)), "ASCII").upper()


class SimpleItemContainerTERemapper(TERemapper):
	def __init__(self, te_id, items_list_name="Items"):
		TERemapper.__init__(self, te_id)
		self.__items_list_name = items_list_name
	
	def remap_te(self, te_compound, map_info):
		# Go through the items.
		items_list = find_named_child(te_compound, self.__items_list_name)
		if items_list is not None:
			assert items_list.tag == "list"
			assert len(items_list) == 0 or items_list.get("subtype") == "10"
			for item_compound in items_list:
				remap_item_compound(item_compound, map_info)


class SimpleItemContainerEntityRemapper(EntityRemapper):
	def __init__(self, te_id, items_list_name="Items"):
		EntityRemapper.__init__(self, te_id)
		self.__items_list_name = items_list_name

	def remap_entity(self, entity_compound, map_info):
		# Go through the items.
		items_list = find_named_child(entity_compound, self.__items_list_name)
		if items_list is not None:
			assert items_list.tag == "list"
			assert len(items_list) == 0 or items_list.get("subtype") == "10"
			for item_compound in items_list:
				remap_item_compound(item_compound, map_info)


class CauldronRemapper(SimpleItemContainerTERemapper):
	def __init__(self):
		SimpleItemContainerTERemapper.__init__(self, "Cauldron")


class ChestRemapper(SimpleItemContainerTERemapper):
	def __init__(self):
		SimpleItemContainerTERemapper.__init__(self, "Chest")


class FurnaceRemapper(SimpleItemContainerTERemapper):
	def __init__(self):
		SimpleItemContainerTERemapper.__init__(self, "Furnace")


class HopperRemapper(SimpleItemContainerTERemapper):
	def __init__(self):
		SimpleItemContainerTERemapper.__init__(self, "Hopper")


class TrapRemapper(SimpleItemContainerTERemapper):
	def __init__(self):
		SimpleItemContainerTERemapper.__init__(self, "Trap")


class LooseItemRemapper(EntityRemapper):
	def __init__(self):
		EntityRemapper.__init__(self, "Item")

	def remap_entity(self, entity_compound, map_info):
		# Find the item structure and remap it.
		item_compound = find_named_child(entity_compound, "Item")
		remap_item_compound(item_compound, map_info)


class LegacyChestCartRemapper(SimpleItemContainerEntityRemapper):
	def __init__(self):
		EntityRemapper.__init__(self, "Minecart")


class NewChestCartRemapper(SimpleItemContainerEntityRemapper):
	def __init__(self):
		EntityRemapper.__init__(self, "MinecartChest")


class FallingSandRemapper(EntityRemapper):
	def __init__(self):
		EntityRemapper.__init__(self, "FallingSand")

	def remap_entity(self, entity_compound, map_info):
		id = None
		tile_named = find_named(entity_compound, "Tile")
		if tile_named is not None:
			id = get_number_from_named(tile_named, "byte")
		tileid_named = find_named(entity_compound, "TileID")
		if tileid_named is not None:
			id = get_number_from_named(tileid_named, "short")
		if id is not None:
			new_id = map_info.block_map.get(id)
			if new_id is None:
				print("No remap for block {} found in falling sand entity!".format(id))
				sys.exit(1)
			if tile_named is not None:
				entity_compound.remove(tile_named)
			if tileid_named is not None:
				entity_compound.remove(tileid_named)
			tileid_named = xml.etree.ElementTree.SubElement(entity_compound, "named", {"name": "TileID"})
			tileid_short = xml.etree.ElementTree.SubElement(tileid_named, "short")
			tileid_short.set("value", str(new_id))


class ItemFrameRemapper(EntityRemapper):
	def __init__(self):
		EntityRemapper.__init__(self, "ItemFrame")

	def remap_entity(self, entity_compound, map_info):
		# Remap the item.
		item_compound = find_named_child(entity_compound, "Item")
		if item_compound is not None:
			remap_item_compound(item_compound, map_info)


class PlayerInventoryRemapper(Remapper):
	def __init__(self):
		Remapper.__init__(self)

	def remap_player(self, player_compound, map_info):
		inventory_list = find_named_child(player_compound, "Inventory")
		assert inventory_list.tag == "list"
		assert len(inventory_list) == 0 or inventory_list.get("subtype") == "10"
		for item_compound in inventory_list:
			remap_item_compound(item_compound, map_info)


def run(mcwutil_path, vanilla_block_ranges, vanilla_item_ranges, mod_info, remappers):
	# Parse command-line arguments.
	if len(sys.argv) != 5:
		print("Usage:")
		print("{} <inbasedir> <inworlddir> <outbasedir> <outworlddir>".format(sys.argv[0]))
		sys.exit(1)
	input_base_dir = sys.argv[1]
	input_world_dir = os.path.join(input_base_dir, sys.argv[2])
	output_base_dir = sys.argv[3]
	output_world_dir = os.path.join(output_base_dir, sys.argv[4])

	# Ensure input and output directories exist.
	if not os.path.isdir(input_world_dir):
		print("Input world directory {} is not an existing directory!".format(input_world_dir))
		sys.exit(1)
	try:
		os.mkdir(output_world_dir)
	except OSError:
		pass

	# Assemble a MapInfo object.
	print("===========================")
	print("CONSTRUCTING MAPPING TABLES")
	print("===========================")
	print()
	map_info = MapInfo(input_base_dir, output_base_dir, vanilla_block_ranges, vanilla_item_ranges, mod_info)

	# Add the world block, player inventory, and vanilla container remappers.
	remappers.append(WorldBlockRemapper())
	remappers.append(CauldronRemapper())
	remappers.append(ChestRemapper())
	remappers.append(FurnaceRemapper())
	remappers.append(HopperRemapper())
	remappers.append(TrapRemapper())
	remappers.append(LooseItemRemapper())
	remappers.append(LegacyChestCartRemapper())
	remappers.append(NewChestCartRemapper())
	remappers.append(FallingSandRemapper())
	remappers.append(ItemFrameRemapper())
	remappers.append(PlayerInventoryRemapper())
	map_info.remappers = remappers

	# Get a temporary working directory.
	with tempfile.TemporaryDirectory() as work_dir:
		# Print progress.
		print()
		print()
		print()
		print("=================")
		print("PROCESSING CHUNKS")
		print("=================")
		print()

		# Find all the dimensions in the input directory.
		dimensions = ["region"]
		for f in os.listdir(input_world_dir):
			if f.startswith("DIM"):
				region_dir = os.path.join(f, "region")
				if os.path.isdir(region_dir):
					dimensions.append(region_dir)

		# Compile a regexp to match region filenames.
		region_file_re = re.compile(r"^r\.-?[0-9]+\.-?[0-9]+\.mca$")
		chunk_file_re = re.compile(r"^chunk-([0-9]{4})\.nbt\.zlib$")

		# Process the dimensions.
		for dimension in dimensions:
			print("===== PROCESSING DIMENSION {} =====".format(dimension))
			try:
				# Create the dimension directory, in case it doesn’t yet exist.
				dim_dir = os.path.join(output_world_dir, dimension)
				if not os.path.isdir(dim_dir):
					os.makedirs(os.path.join(output_world_dir, dimension), exist_ok=True)

				# Iterate the regions.
				for region in os.listdir(os.path.join(input_world_dir, dimension)):
					if region_file_re.match(region):
						try:
							# Print progress.
							print("Processing region {}…".format(region), end="")
							try:
								# Shell out to mcwutil region-unpack.
								subprocess.check_call([mcwutil_path, "region-unpack", os.path.join(input_world_dir, dimension, region), work_dir])

								# Scan for chunks.
								chunks = []
								for f in os.listdir(work_dir):
									match = chunk_file_re.match(f)
									if match:
										chunks.append(match.group(1))
								chunks.sort()

								# Process the chunks.
								done = 0
								for chunk in chunks:
									# Print progress.
									print("\rProcessing region {}… chunk {} ({}/{})…".format(region, chunk, done + 1, len(chunks)), end="")

									# We could shell out to mcwutil zlib-decompress, but doing it in-process is faster than forking.
									with open(os.path.join(work_dir, "chunk-" + chunk + ".nbt.zlib"), "rb") as fp:
										compressed_data = fp.read()
									plain_data = zlib.decompress(compressed_data)
									with open(os.path.join(work_dir, "chunk.nbt"), "wb") as fp:
										fp.write(plain_data)

									# Shell out to mcwutil nbt-to-xml.
									subprocess.check_call([mcwutil_path, "nbt-to-xml", os.path.join(work_dir, "chunk.nbt"), os.path.join(work_dir, "chunk.xml")])

									# Process the XML file.
									etree = xml.etree.ElementTree.parse(os.path.join(work_dir, "chunk.xml"))
									for remapper in remappers:
										remapper.remap_chunk(etree, map_info)
									etree.write(os.path.join(work_dir, "chunk.xml"), encoding="UTF-8")

									# Shell out to mcwutil nbt-from-xml.
									subprocess.check_call([mcwutil_path, "nbt-from-xml", os.path.join(work_dir, "chunk.xml"), os.path.join(work_dir, "chunk.nbt")])

									# We could shell out to mcwutil zlib-compress, but doing it in-process is faster than forking.
									with open(os.path.join(work_dir, "chunk.nbt"), "rb") as fp:
										plain_data = fp.read()
									compressed_data = zlib.compress(plain_data, 9)
									with open(os.path.join(work_dir, "chunk-" + chunk + ".nbt.zlib"), "wb") as fp:
										fp.write(compressed_data)

									# Update progress.
									done += 1

								# Shell out to mcwutil region-pack.
								subprocess.check_call([mcwutil_path, "region-pack", work_dir, os.path.join(output_world_dir, dimension, region)])
							finally:
								print()
						finally:
							# Delete all temporary files.
							for work_file in os.listdir(work_dir):
								os.unlink(os.path.join(work_dir, work_file))
			finally:
				print()

		# Process the players in the players directory, if any.
		input_players_dir = os.path.join(input_world_dir, "players")
		output_players_dir = os.path.join(output_world_dir, "players")
		if os.path.isdir(input_players_dir):
			# Print progress.
			print()
			print()
			print()
			print("=====================")
			print("PROCESSING MP PLAYERS")
			print("=====================")
			print()

			os.makedirs(output_players_dir, exist_ok=True)
			for player_filename in os.listdir(input_players_dir):
				# Print progress.
				print("===== PROCESSING {} =====".format(player_filename))
				# Decompress the GZipped data and store it in the work directory.
				with open(os.path.join(input_players_dir, player_filename), "rb") as fp:
					compressed_data = fp.read()
				plain_data = gzip.decompress(compressed_data)
				with open(os.path.join(work_dir, "player.nbt"), "wb") as fp:
					fp.write(plain_data)

				# Shell out to mcwutil nbt-to-xml.
				subprocess.check_call([mcwutil_path, "nbt-to-xml", os.path.join(work_dir, "player.nbt"), os.path.join(work_dir, "player.xml")])

				# Process the XML file.
				# The MP player is located at minecraft-nbt/named[name=""]/compound.
				etree = xml.etree.ElementTree.parse(os.path.join(work_dir, "player.xml"))
				minecraft_nbt = etree.getroot()
				root_compound = find_named_child(minecraft_nbt, "")
				for remapper in remappers:
					remapper.remap_player(root_compound, map_info)
				etree.write(os.path.join(work_dir, "player.xml"), encoding="UTF-8")

				# Shell out to mcwutil nbt-from-xml.
				subprocess.check_call([mcwutil_path, "nbt-from-xml", os.path.join(work_dir, "player.xml"), os.path.join(work_dir, "player.nbt")])

				# Compress the data with GZip and store it in the output directory.
				with open(os.path.join(work_dir, "player.nbt"), "rb") as fp:
					plain_data = fp.read()
				compressed_data = gzip.compress(plain_data, 9)
				with open(os.path.join(output_players_dir, player_filename), "wb") as fp:
					fp.write(compressed_data)

		# Print progress.
		print()
		print()
		print()
		print("====================")
		print("PROCESSING SP PLAYER")
		print("====================")
		print()

		# Process the level.dat file.
		# Decompress the GZipped data and store it in the work directory.
		with open(os.path.join(input_world_dir, "level.dat"), "rb") as fp:
			compressed_data = fp.read()
		plain_data = gzip.decompress(compressed_data)
		with open(os.path.join(work_dir, "level.nbt"), "wb") as fp:
			fp.write(plain_data)

		# Shell out to mcwutil nbt-to-xml.
		subprocess.check_call([mcwutil_path, "nbt-to-xml", os.path.join(work_dir, "level.nbt"), os.path.join(work_dir, "level.xml")])

		# Process the XML file.
		# The SP player is located at minecraft-nbt/named[name=""]/compound/named[name="Data"]/compound/named[name="Player"]/compound.
		etree = xml.etree.ElementTree.parse(os.path.join(work_dir, "level.xml"))
		minecraft_nbt = etree.getroot()
		root_compound = find_named_child(minecraft_nbt, "")
		data_compound = find_named_child(root_compound, "Data")
		player_compound = find_named_child(data_compound, "Player")
		if player_compound is not None:
			print("===== PROCESSING THE PLAYER =====")
			for remapper in remappers:
				remapper.remap_player(player_compound, map_info)
		etree.write(os.path.join(work_dir, "player.xml"), encoding="UTF-8")

		# Shell out to mcwutil nbt-from-xml.
		subprocess.check_call([mcwutil_path, "nbt-from-xml", os.path.join(work_dir, "level.xml"), os.path.join(work_dir, "level.nbt")])

		# Compress the data with GZip and store it in the output directory.
		with open(os.path.join(work_dir, "level.nbt"), "rb") as fp:
			plain_data = fp.read()
		compressed_data = gzip.compress(plain_data, 9)
		with open(os.path.join(output_world_dir, "level.dat"), "wb") as fp:
			fp.write(compressed_data)

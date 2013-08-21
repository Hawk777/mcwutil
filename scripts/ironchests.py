#!/bin/env python3

import remapper

def create_all_remappers():
	mappers = [remapper.SimpleItemContainerTERemapper(x) for x in ("COPPER", "IRON", "SILVER", "GOLD", "DIAMOND", "CRYSTAL")]
	mappers += [remapper.SimpleItemContainerTERemapper("IronChest." + x) for x in ("COPPER", "IRON", "SILVER", "GOLD", "DIAMOND", "CRYSTAL")]
	return mappers

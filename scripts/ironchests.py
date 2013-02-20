#!/bin/env python3

import remapper

def create_all_remappers():
	return [remapper.SimpleItemContainerTERemapper(x) for x in ("COPPER", "IRON", "SILVER", "GOLD", "DIAMOND", "CRYSTAL")]

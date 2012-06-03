#ifndef NBT_TAGS_H
#define NBT_TAGS_H

namespace NBT {
	enum Tag {
		TAG_END,
		TAG_BYTE,
		TAG_SHORT,
		TAG_INT,
		TAG_LONG,
		TAG_FLOAT,
		TAG_DOUBLE,
		TAG_BYTE_ARRAY,
		TAG_STRING,
		TAG_LIST,
		TAG_COMPOUND,
		TAG_INT_ARRAY,
	};
}

#endif


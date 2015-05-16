// ogg_tag.cpp
//					$Id: //adrian/choonz/choonz_library/main/ogg_tag.cpp#2 $
//					$Change: 64 $

#include "ogg_tag.h"
#include "defs.h"

OggTag::OggTag (const std::string& s) : MetaTag (s)
{
}

void OggTag::readTag ()
{
    m_data.resize (Choonz::TAG_MAX);
}

bool OggTag::writeTag (const std::vector <std::string>&)
{
	return false;
}

// flac_tag.cpp
//					$Id: //adrian/choonz/choonz_library/main/flac_tag.cpp#2 $
//					$Change: 64 $

#include "flac_tag.h"
#include "defs.h"

FlacTag::FlacTag (const std::string& s) : MetaTag (s)
{
}

void FlacTag::readTag ()
{
	m_data.resize (Choonz::TAG_MAX);
}

bool FlacTag::writeTag (const std::vector <std::string>&)
{
	return false;
}


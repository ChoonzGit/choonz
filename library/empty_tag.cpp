// $Id: //adrian/choonz/choonz_library/main/empty_tag.cpp#2 $
// $Change: 64 $

#include "empty_tag.h"
#include "defs.h"

EmptyTag::EmptyTag (const std::string& s) : MetaTag (s)
{
	m_data.resize (Choonz::TAG_MAX);
}

void EmptyTag::readTag ()
{
	m_empty = true;
}

bool EmptyTag::writeTag (const std::vector <std::string>& data)
{
	return true; // sod all we can do anyway
}

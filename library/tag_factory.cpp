// tag_factory.cpp
// $Id: //adrian/choonz/choonz_library/main/tag_factory.cpp#2 $
// $Change: 64 $
// Create a TagLib::File depending on file extension

#include "../library/tag_factory.h"
#include "../library/mp3_tag.h"
#include "../library/flac_tag.h"
#include "../library/wav_tag.h"
#include "../library/ogg_tag.h"
#include "../library/empty_tag.h"
#include <boost/algorithm/string/predicate.hpp>

TagFactory::TagFactory ()
{
}

//Return a TagLib::File based on file extension
MetaTag * TagFactory::getTag (const std::string& filename) const
{
	MetaTag *	tag (0);
	
	if (boost::algorithm::iends_with (filename, ".mp3"))
	{
		tag = new Mp3Tag (filename);
	}
	else if (boost::algorithm::iends_with (filename, ".wav"))
	{
		tag = new WavTag (filename);
	}
	else if (boost::algorithm::iends_with (filename, ".flac"))
	{
		tag = new FlacTag (filename);
	}
	else if (boost::algorithm::iends_with (filename, ".ogg"))
	{
		tag = new OggTag (filename);
	}
	else
	{
		tag = new EmptyTag (filename);
	}

	return tag;
}

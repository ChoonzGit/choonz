// mp3_tag.cpp
//					$Id: //adrian/choonz/choonz_library/main/mp3_tag.cpp#2 $
//					$Change: 64 $

#include "mp3_tag.h"
#include "defs.h"
#include <taglib/tag.h>
#include <taglib/id3v2tag.h>
#include <boost/lexical_cast.hpp>

Mp3Tag::Mp3Tag (const std::string& s) : MetaTag (s), m_tag_file (0)
{
}

void Mp3Tag::readTag ()
{
	m_tag_file = new TagLib::MPEG::File (m_filename.c_str (), true);

	m_data.resize (Choonz::TAG_MAX);

	set_tag (Choonz::TAG_TITLE, m_tag_file -> tag () -> title ().toCString ());
	set_tag (Choonz::TAG_ARTIST, m_tag_file -> tag () -> artist ().toCString ());
	set_tag (Choonz::TAG_ALBUM, m_tag_file -> tag () -> album ().toCString ());
	set_tag (Choonz::TAG_GENRE, m_tag_file -> tag () -> genre ().toCString ());
	set_tag (Choonz::TAG_YEAR, boost::lexical_cast <std::string> (m_tag_file -> tag () -> year ()));
	set_tag (Choonz::TAG_TRACK, boost::lexical_cast <std::string> (m_tag_file -> tag () -> track ()));
	set_tag (Choonz::TAG_COMMENT, m_tag_file -> tag () -> comment ().toCString ());

	TagLib::MPEG::Properties * prop = reinterpret_cast <TagLib::MPEG::Properties *> (m_tag_file -> audioProperties ());

	if (prop)
	{
		set_tag (Choonz::TAG_LENGTH, boost::lexical_cast <std::string> (prop -> length ()));
		set_tag (Choonz::TAG_BITRATE, boost::lexical_cast <std::string> (prop -> bitrate ()));
		set_tag (Choonz::TAG_SAMPLERATE, boost::lexical_cast <std::string> (prop -> sampleRate ()));
		set_tag (Choonz::TAG_CHANNELS, boost::lexical_cast <std::string> (prop -> channels ()));
	}

	m_empty = m_tag_file -> tag () -> isEmpty ();
}

bool Mp3Tag::writeTag (const std::vector <std::string>& data)
{
	m_tag_file = new TagLib::MPEG::File (m_filename.c_str (), false);

	TagLib::ID3v2::Tag * tag = m_tag_file -> ID3v2Tag (true);

	tag -> setTitle (data [Choonz::SAVE_TITLE]);
	tag -> setArtist (data [Choonz::SAVE_ARTIST]);
	tag -> setAlbum (data [Choonz::SAVE_ALBUM]);
	tag -> setGenre (data [Choonz::SAVE_GENRE]);
	tag -> setComment (data [Choonz::SAVE_COMMENT]);
	tag -> setYear (boost::lexical_cast <unsigned> (data [Choonz::SAVE_YEAR]));
	tag -> setTrack (boost::lexical_cast <unsigned> (data [Choonz::SAVE_TRACK]));

	return m_tag_file -> save ();
}

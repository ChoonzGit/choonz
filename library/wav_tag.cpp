// $Id: //adrian/choonz/choonz_library/main/wav_tag.cpp#2 $
// $Change: 64 $

#include "wav_tag.h"
#include "defs.h"
#include <taglib/tag.h>
#include <taglib/wavproperties.h>
#include <boost/lexical_cast.hpp>

WavTag::WavTag (const std::string& s) : MetaTag (s)
{
}

// WAV files don't have tags as such, but they do have properties we're interested in.
void WavTag::readTag ()
{
	m_tag_file = new TagLib::RIFF::WAV::File (m_filename.c_str (), true);

	m_data.resize (Choonz::TAG_MAX);

	TagLib::RIFF::WAV::Properties * prop = reinterpret_cast <TagLib::RIFF::WAV::Properties *> (m_tag_file -> audioProperties ());

	if (prop)
	{
		m_data [Choonz::TAG_LENGTH] = boost::lexical_cast <std::string> (prop -> length ());
		m_data [Choonz::TAG_BITRATE] = boost::lexical_cast <std::string> (prop -> bitrate ());
		m_data [Choonz::TAG_SAMPLERATE] = boost::lexical_cast <std::string> (prop -> sampleRate ());
		m_data [Choonz::TAG_CHANNELS] = boost::lexical_cast <std::string> (prop -> channels ());
	}

	m_empty = m_tag_file -> tag () -> isEmpty ();
}

bool WavTag::writeTag (const std::vector <std::string>&)
{
	return false;
}


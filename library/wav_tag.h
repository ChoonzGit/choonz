// $Id: //adrian/choonz/choonz_library/main/wav_tag.h#2 $
// $Change: 64 $

#ifndef WAV_TAG_H_DEFINED
#define WAV_TAG_H_DEFINED

#include "meta_tag.h"
#include <taglib/wavfile.h>

class WavTag : public MetaTag
{
public:
	WavTag (const std::string&);
	~WavTag () {}
	
	void 	readTag ();
	bool	writeTag (const std::vector <std::string>&);

private:
	TagLib::RIFF::WAV::File *	m_tag_file;
};

#endif

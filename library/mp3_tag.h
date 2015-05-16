//mp3_tag.h
//				$Id: //adrian/choonz/choonz_library/main/mp3_tag.h#2 $
//				$Change: 64 $

#ifndef MP3_TAG_H_DEFINED
#define MP3_TAG_H_DEFINED

#include "meta_tag.h"
#include <string>
#include <taglib/mpegfile.h>

class Mp3Tag : public MetaTag
{
public:
	Mp3Tag (const std::string&);
	~Mp3Tag () { delete m_tag_file; }

	void 	readTag ();
	bool	writeTag (const std::vector <std::string>&);

private:
	TagLib::MPEG::File *	m_tag_file;
};

#endif

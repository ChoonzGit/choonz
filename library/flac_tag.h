// flac_tag.h
//				$Id: //adrian/choonz/choonz_library/main/flac_tag.h#2 $
//				$Change: 64 $

#ifndef FLAC_TAG_H_DEFINED
#define FLAC_TAG_H_DEFINED

#include "meta_tag.h"

class FlacTag : public MetaTag
{
public:
	FlacTag (const std::string&);
	~FlacTag () {}
	
	void 	readTag ();
	bool	writeTag (const std::vector <std::string>&);
};

#endif

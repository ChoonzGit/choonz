// ogg_tag.h
//				$Id: //adrian/choonz/choonz_library/main/ogg_tag.h#2 $
//				$Change: 64 $

#ifndef OGG_TAG_H_DEFINED
#define OGG_TAG_H_DEFINED

#include "meta_tag.h"

class OggTag : public MetaTag
{
public:
	OggTag (const std::string&);
	
	void 	readTag ();
	bool	writeTag (const std::vector <std::string>&);
};

#endif

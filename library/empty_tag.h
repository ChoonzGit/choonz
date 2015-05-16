// $Id: //adrian/choonz/choonz_library/main/empty_tag.h#2 $
// $Change: 64 $

#ifndef EMPTY_TAG_H_DEFINED
#define EMPTY_TAG_H_DEFINED

#include "meta_tag.h"
#include <string>

class EmptyTag : public MetaTag
{
public:
	EmptyTag (const std::string&);
	~EmptyTag () { }
	
	void 	readTag ();
	bool	writeTag (const std::vector <std::string>&);
};

#endif

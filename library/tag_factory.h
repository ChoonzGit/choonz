// tag_factory.h
// $Id: //adrian/choonz/choonz_library/main/tag_factory.h#2 $
// $Change: 64 $
// Create a meta_tag object based on file extension

#ifndef TAG_FACTORY_H_DEFINED
#define TAG_FACTORY_H_DEFINED

#include "meta_tag.h"


class TagFactory
{
public:
	TagFactory ();
	MetaTag * getTag (const std::string&) const;
};

#endif

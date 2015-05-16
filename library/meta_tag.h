// meta_tag.h
// $Id: //adrian/choonz/choonz_library/main/meta_tag.h#2 $
// $Change: 64 $
// A base class for various tag readers. Although taglib has a simpler API, we do it a more complicated
// way so that derived classes can create the correct TagLib::File and use its API directly.

#ifndef TAG_LIBRARY_H_DEFINED
#define TAG_LIBRARY_H_DEFINED

#include <string>
#include <vector>

class MetaTag
{
public:
	MetaTag ();
	virtual ~MetaTag ();

	virtual void readTag () = 0;
	virtual bool writeTag (const std::vector <std::string>&) = 0;

	unsigned size () const;
	unsigned size (unsigned);
	void set_tag (unsigned, const std::string&);
	const std::string& operator [] (unsigned) const;
	MetaTag& operator = (const std::vector <std::string>&);
	void clear ();

protected:
	MetaTag (const std::string&);

	std::vector <std::string>	m_data;
	bool						m_empty; //True if no data
	std::string					m_filename;
};

#endif

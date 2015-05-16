// meta_tag.cpp

#include "meta_tag.h"

MetaTag::MetaTag () : m_empty (true)
{
}

MetaTag::MetaTag (const std::string& s) : m_filename (s)
{
}

MetaTag::~MetaTag ()
{
}

unsigned MetaTag::size () const
{
	return m_data.size ();
}

unsigned MetaTag::size (unsigned new_size)
{
	unsigned		old_size (m_data.size ());

	m_data.resize (new_size);

	return old_size;
}

void MetaTag::set_tag (unsigned index, const std::string& data)
{
	m_data [index] = data;
}

const std::string& MetaTag::operator [] (unsigned index) const
{
	return m_data [index];
}

MetaTag& MetaTag::operator = (const std::vector <std::string>& fields)
{
	m_data = fields;

	m_empty = m_data.size() > 0;

	return * this;
}

void MetaTag::clear ()
{
	for (std::vector <std::string>::iterator i = m_data.begin (); i != m_data.end (); ++i)
		(* i).clear ();
}

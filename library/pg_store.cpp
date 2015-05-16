// $Id: //adrian/choonz/choonz_library/main/pg_store.cpp#3 $

#include "pg_store.h"
#include "meta_tag.h"
#include "defs.h"
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/tokenizer.hpp>
#include <iostream> //Debug
const char * PgStore::m_field_name [12] = { "title", "artist", "album", "genre", "year", "track", "length", "bitrate", "samplerate", "channels", "comment" };

PgStore::PgStore (const std::string& host) :
    m_db (0),
    m_result (0),
    m_host (host)
{
	initialise ();
}

PgStore::~PgStore ()
{
	if (m_db)
	{
		PQfinish (m_db);
	}
}

bool PgStore::readLibrary ()
{
	return false;
}

unsigned PgStore::addDirectory (const std::string& directory, unsigned parent)
{
	unsigned			new_id (0);

	if (isOk())
	{
		PGresult *			result (0);
		std::stringstream	ss;

		ss << "SELECT add_directory FROM add_directory ('" << pg_escape (directory) << "'," << parent << ")";

		result = PQexec (m_db, ss.str ().c_str ());

		if (PQntuples (result) == 1)
		{
			new_id = boost::lexical_cast <unsigned> (PQgetvalue (result, 0, 0));
		}
		else
			std::cerr << "ERROR: failed to add directory: " << directory << "\n";

		PQclear (result);
	}

	return new_id;
}

// The vector of fields is in the order of m_field_array
bool PgStore::addTrack (const std::string& track_id, const std::string& filename, const MetaTag& fields, const std::vector <unsigned>& parts, unsigned parent)
{
	bool				success(false);

	if	(isOk())
	{
		success = saveTrackAsText(track_id, filename, fields, parts, parent) ? true : saveTrackAsByte(track_id, filename, fields, parts, parent);
	}

	if (!success)
		std::cerr << "ERROR: failed to add track: " << filename << "\n";

	return success;
}

bool PgStore::initialise ()
{
	if (!isOk())
	{
		std::string		s ("dbname=choonz user=postgres host=");

		s += m_host;

		m_db = PQconnectdb (s.c_str ());

		if (PQstatus (m_db) != CONNECTION_OK)
		{
			PQfinish (m_db);
			m_db = 0;
		}
	}

	return isOk();
}

std::string PgStore::getTrack (const std::string& track_hash)
{
	std::stringstream		track;

	if (isOk())
	{
		PGresult *				result (0);
		std::stringstream		query;

		query << "SELECT filename, parts FROM track WHERE hash = '" << track_hash << "'";

		result = PQexec (m_db, query.str ().c_str ());

		if (PQntuples (result) == 1)
		{
			get_parts (PQgetvalue (result, 0, 1));

			for (unsigned x = 0; x < m_parts.size (); ++x)
				track << m_parts [x] << "/";

			track << PQgetvalue (result, 0, 0);
		}

		PQclear (result);
	}

	return track.str ();
}

std::string PgStore::getTrack (std::size_t track_hash)
{
	std::stringstream		track;

	if (isOk())
	{
		PGresult *				result (0);
		std::stringstream		query;

		query << "SELECT filename, parts FROM track WHERE hash = '" << track_hash << "'";

		result = PQexec (m_db, query.str ().c_str ());

		if (PQntuples (result) == 1)
		{
			get_parts (PQgetvalue (result, 0, 1));

			for (unsigned x = 0; x < m_parts.size (); ++x)
				track << m_parts [x] << "/";

			track << PQgetvalue (result, 0, 0);
		}

		PQclear (result);
	}

	return track.str ();
}

bool PgStore::trackExists (const std::string& track_id)
{
	bool				exists(false);
	if (isOk())
	{
		std::stringstream	ss;

		ss << "SELECT track_exists('" << track_id << "')";

		PGresult * result (PQexec(m_db, ss.str().c_str()));

		exists = PQntuples(result) ? * PQgetvalue(result, 0, 0) == 't' : false;
		PQclear(result);
	}

	return exists;
}

unsigned PgStore::trackCount ()
{
	unsigned			tracks (0);

	if (isOk())
	{
		PGresult *			result (PQexec (m_db, "SELECT track_count FROM track_count()"));

		if (PQntuples (result) == 1)
		{
			tracks = boost::lexical_cast <unsigned> (PQgetvalue (result, 0, 0));
		}

		PQclear (result);
	}

	return tracks;
}

// A better way to do this?
// 02 Jan 2012 Now deprecated after db schema changes, to be removed
/*
bool PgStore::next_tag (std::string& hash, MetaTag& tag_data)
{
	bool	got_track (false);

	if (isOk())
	{
		if (m_result == 0)
		{
			m_result = PQexec (m_db, "SELECT hash, tag FROM track");
			m_current_row = 0;
		}

		if (m_result && m_current_row < PQntuples (m_result))
		{
			std::stringstream		track;
			std::string				tag;

			hash = PQgetvalue (m_result, m_current_row, 0);
			tag = PQgetvalue (m_result, m_current_row, 1);

			set_tag (tag, tag_data);

			++m_current_row;

			got_track = true;
		}
		else
		{
			m_current_row = 0;
			PQclear (m_result);
			m_result = 0;
		}
	}

	return got_track;
}
*/
// Deprecated due to database schema update, due to be removed
/*
bool PgStore::get_tag (const std::string& hash, MetaTag& tag_data)
{
	bool				success (false);

	if (isOk())
	{
		PGresult *			result (0);
		std::stringstream	ss;

		ss << "SELECT tag from track WHERE hash = '" << hash << "'";

		result = PQexec (m_db, ss.str ().c_str ());

		if (PQntuples (result) == 1)
		{
			std::string		tag_string (PQgetvalue (result, 0, 0));

			set_tag (tag_string, tag_data);

			success = true;
		}

		PQclear (result);
	}

	return success;
}
*/
bool PgStore::renameTrack (const std::string& old_hash, const std::string& new_hash, const std::string& new_name)
{
	bool				success (false);
	PGresult *			result (0);
	std::stringstream	ss;

	ss << "UPDATE track SET hash = '" << new_hash << "', filename = '" << pg_escape (new_name) << "' WHERE hash = '" << old_hash << "'";

	result = PQexec (m_db, ss.str ().c_str ());

	success = (boost::lexical_cast <bool> (PQcmdTuples (result)) == 1);

	PQclear (result);

	return success;
}
/*
// This function assumes that \n in comments has already been replaced with <LF>
// Fields are: artist, album, genre, year, track num, comment.
// A couple of ways of doing this are possible, but we'll read the existing tag from the db,
// replace the editable fields, and write back to db. We also update the track file.
bool PgStore::update_tag (const std::string& track_hash, const MetaTag& fields)
{
	bool				success (false);
	PGresult *			result (0);
	std::stringstream	ss;

	ss << "UPDATE track SET tag = ARRAY[";

	for (unsigned x = 0; x < fields.size (); ++x)
	{
		ss << "'" << pg_escape (fields [x]) << "'"; // TODO: check fields is the right size
		if (x < fields.size () - 1)
			ss << ",";
	}

	ss << "] WHERE hash = '" << track_hash << "'";

	result = PQexec (m_db, ss.str ().c_str ());

	success = boost::lexical_cast <bool> (PQcmdTuples (result)) == 1;

	PQclear (result);

	return success;
}
*/

// The tag data now has its own table and the MetaTag struct is assumed to be properly populated
// A trigger adds an empty tag record when a new track is inserted
bool PgStore::updateTag (const std::string& track_hash, const MetaTag& fields)
{
    bool                success (false);
    PGresult *          result (0);
    const char *        columns[] = { "title", "artist", "album", "genre", "year", "track", "length", "bitrate", "samplerate", "channels", "comment" }; // These names must match those in the table, MetaTag assumed to be in this order
    std::stringstream   ss;

    ss << "UPDATE tag SET ";

    for (unsigned x = 0; x < fields.size (); ++x)
    {
        ss << columns [x] << " = '" << pg_escape(fields[x]) << "'";
        if (x < fields.size () - 1)
            ss << ",";
    }

    ss << " WHERE track_hash = '" << track_hash << "'";

    result = PQexec (m_db, ss.str ().c_str ());

    if (PQntuples(result) == 1)
    {
        success = boost::lexical_cast <bool> (PQcmdTuples (result)) == 1;
    }

    PQclear (result);

    return success;
}

void PgStore::empty()
{
	PQexec(m_db, "DELETE FROM track");
}

bool PgStore::getListIds(std::vector <std::string>& id_list)
{
	bool			success(false);

	if (isOk())
	{
		PGresult *		result(0);

		result = PQexec(m_db, "SELECT id FROM playlist");

		if (PQntuples(result))
		{
			for (int x = 0; x < PQntuples(result); ++x)
				id_list.push_back(PQgetvalue(result, x, 0));

			success = true;
		}

		PQclear(result);
	}

	return success;
}

bool PgStore::getList(const std::string& list_id, std::vector<std::string>& list)
{
	bool				success(false);

	if (isOk())
	{
		PGresult *			result(0);
		std::stringstream	ss;

		ss << "SELECT name, comment FROM playlist WHERE id = " << list_id;

		result = PQexec (m_db, ss.str ().c_str());

		if (PQntuples (result))
		{
			for (int x = 0; x < 2; ++x)
				list.push_back(PQgetvalue(result, 0, x));

			PQclear(result);

			ss.str("");

			ss << "SELECT hash, filename FROM playlist_files(" << list_id << ")";

			result = PQexec(m_db, ss.str().c_str());

			for (int row = 0; row < PQntuples(result); ++row)
			{
				ss.str("");

				ss << PQgetvalue(result, row, 0) << "|" << PQgetvalue(result, row, 1);
				list.push_back(ss.str());
			}

			success = true;
		}

		PQclear(result);
	}

	return success;
}

bool PgStore::openPlaylist(unsigned id, std::string& name, std::vector<std::string>& tracks)
{
	bool		        success(false);

	if (isOk())
	{
		PGresult *	        result(0);
		std::stringstream  ss;

		ss << "SELECT name, track_hash FROM playlist WHERE id = " << id;

		result = PQexec(m_db, ss.str().c_str());

    	if (PQntuples(result))
    	{
    	    std::string     track_list(PQgetvalue(result, 0, 1));

    	    name = PQgetvalue(result, 0, 0);
    	    boost::iter_split (tracks, track_list, boost::first_finder (","));

    	    success = true;
    	}
	}

	return success;
}

bool PgStore::savePlaylist(std::size_t& id, const std::string& title, const std::string& comment, std::vector<std::string>& tracks)
{
	bool				success(false);

	if (isOk())
	{
		PGresult *			result(0);
		std::stringstream	ss;
		std::size_t         list_id (0);

    	ss << "SELECT add_playlist FROM add_playlist ('" << id << "')";

    	result = PQexec (m_db, ss.str ().c_str ());

    	if (PQntuples (result) == 1)
    	{
    	    list_id = boost::lexical_cast <std::size_t> (PQgetvalue (result, 0, 0));
    	}

    	PQclear(result);

    	if (list_id)
    	{
    	    ss.str("");

    	    ss << "UPDATE playlist SET name = '" << pg_escape(title) << "', comment = '" << pg_escape(comment) << "', track_hash = ARRAY[";

    	    for (unsigned x = 0; x < tracks.size (); ++x)
    	    {
    	        ss << "'" << pg_escape (tracks [x]) << "'"; // TODO: check fields is the right size
    	        if (x < tracks.size () - 1)
    	            ss << ",";
    	    }

    	    ss << "] WHERE id = " << list_id;

    	    result = PQexec(m_db, ss.str().c_str());

    	    success = boost::lexical_cast <bool> (PQcmdTuples (result)) == 1;

    	    PQclear(result);
    	}

    	if (success) id = list_id;
	}

	return success;
}

// This version doesn't have a locations table: instead, any directory whose
// parent ID is 0 must be a location
bool PgStore::getLocations(std::vector<int>& ids)
{
	int			row(0);

	if (isOk())
	{
		PGresult *	result (PQexec(m_db, "SELECT id FROM directory WHERE parent_id = 0"));

		for (row = 0; row < PQntuples(result); ++row)
		{
			ids.push_back (boost::lexical_cast <int> (PQgetvalue(result, row, 0)));
		}

		PQclear(result);
	}

	return row > 0;
}

bool PgStore::getChildren(int location_id, std::vector<int>& ids)
{
	if (isOk())
	{
		std::stringstream	ss;

		ss << "SELECT children FROM directory WHERE id = " << location_id;
		PGresult *	result (PQexec(m_db, ss.str().c_str()));

		if (PQntuples(result) == 1) // Anything else is a failure
		{
			std::vector <std::string>	tokens;
			std::string					s(PQgetvalue(result, 0, 0));

			if (s.size())
			{
				s.erase(0, 1);
				s.erase(s.size() - 1, 1);

				boost::iter_split(tokens, s, boost::first_finder(","));

				for (unsigned x = 0; x < tokens.size(); ++x)
				{
					ids.push_back(boost::lexical_cast <int> (tokens [x]));
				}
			}
		}

		PQclear(result);
	}

	return ids.size() > 0;
}

bool PgStore::getLocationName (int location_id, std::string& location_name)
{
	bool				success(false);

	if (isOk())
	{
		std::stringstream	ss;

		ss << "SELECT name FROM directory_name WHERE id = (SELECT name_id FROM directory WHERE id = " << location_id << ")";

		PGresult *	result (PQexec(m_db, ss.str().c_str()));

		if (PQntuples(result) == 1)
		{
			location_name = PQgetvalue(result, 0, 0);
			success = true;
		}

		PQclear(result);
	}

	return success;
}

bool PgStore::getLocationTracks (int location_id, std::vector<std::string>& hashes, std::vector<std::string>& names, std::vector<std::string>& artists, std::vector<std::string>& years, std::vector<std::string>& lengths)
{
	if (isOk())
	{
		std::stringstream	ss;

//		ss << "SELECT hash, filename FROM track WHERE parent_id = " << location_id;
        ss << "SELECT track.hash, track.filename, tag.artist, tag.year, tag.length FROM track INNER JOIN tag ON (tag.track_hash = track.hash) WHERE parent_id = " << location_id;

		PGresult *	result (PQexec(m_db, ss.str().c_str()));

		for (int row = 0; row < PQntuples(result); ++row)
		{
			hashes.push_back(PQgetvalue(result, row, 0));
            names.push_back(PQgetvalue(result, row, 1));
            artists.push_back(PQgetvalue(result, row, 2));
            years.push_back(PQgetvalue(result, row, 3));
            lengths.push_back(PQgetvalue(result, row, 4));
		}

		PQclear (result);
	}

	return hashes.size() > 0;
}

unsigned PgStore::locations()
{
	std::vector<int>	ids;

	return getLocations(ids) ? ids.size() : 0;
}

// Add a track to an album if there is an artist and album name in the tag data
unsigned PgStore::addAlbumTrack (const std::string& hash, const MetaTag& tag)
{
    unsigned        user_id(0); // Not sure that returning a value makes sense
    std::string     artist(tag [Choonz::TAG_ARTIST]);
    std::string     album(tag [Choonz::TAG_ALBUM]);

    if ((artist.length() > 2) && (album.length() >> 2))
    {
        std::stringstream       ss;

        ss << "SELECT add_album_track('" << pg_escape(artist) << "','" << pg_escape(album) << "','" << hash << "')";

        PGresult * result (PQexec(m_db, ss.str().c_str()));

        if (PQntuples(result) == 1)
        {
            user_id = boost::lexical_cast<unsigned> (PQgetvalue(result, 0, 0));
        }

        PQclear(result);
    }

    return user_id;
}

int PgStore::getArtistList (std::vector<int>& artist_list, std::vector<std::string>& artist_name)
{
    int                 row;
    PGresult *          result (PQexec(m_db, "SELECT id,name FROM artist"));

    for (row = 0; row < PQntuples(result); ++row)
    {
        artist_list.push_back(boost::lexical_cast<int>(PQgetvalue(result, row, 0)));
        artist_name.push_back(PQgetvalue(result, row, 1));
    }

    PQclear(result);

    return row;
}

int PgStore::getAlbumList (std::vector<int>& album_id, std::vector<std::string>& album_name)
{
    int                 row;
    PGresult *          result (PQexec(m_db, "SELECT * FROM global_album_list()"));

    for (row = 0; row < PQntuples(result); ++row)
    {
        album_id.push_back(boost::lexical_cast<int>(PQgetvalue(result, row, 0)));
        album_name.push_back(PQgetvalue(result, row, 1));
    }

    PQclear(result);

    return row;
}

int PgStore::getAlbums (int artist_id, std::vector<int>& album_ids, std::vector<std::string>& album_names)
{
    int                 row;
    std::stringstream   ss;

    ss << "SELECT * from album_list(" << artist_id << ")";

    PGresult *          result (PQexec(m_db, ss.str().c_str()));

    for (row = 0; row < PQntuples(result); ++row)
    {
        album_ids.push_back(boost::lexical_cast<int>(PQgetvalue(result, row, 0)));
        album_names.push_back(PQgetvalue(result, row, 1));
    }

    PQclear(result);

    return row;
}

int PgStore::getTracks (int album_id, std::vector<std::string>& track_hash, std::vector<std::string>& track_name)
{
    int                 row;
    std::stringstream   ss;

    ss << "SELECT * from track_list(" << album_id << ")";

    PGresult *          result (PQexec(m_db, ss.str().c_str()));

    for (row = 0; row < PQntuples(result); ++row)
    {
        track_hash.push_back(PQgetvalue(result, row, 0));
        track_name.push_back(PQgetvalue(result, row, 1));
    }

    PQclear(result);

    return row;
}

int PgStore::getTrackAndArtist (int album_id, std::vector<std::string>& track_hash, std::vector<std::string>& track_name, std::vector<std::string>& artist_name, std::vector<std::string>& track_length)
{
    int                 row;
    std::stringstream   ss;

    ss << "SELECT track_hash, title, artist, length from track_artist_list(" << album_id << ")";

    PGresult *          result (PQexec(m_db, ss.str().c_str()));

    for (row = 0; row < PQntuples(result); ++row)
    {
        track_hash.push_back(PQgetvalue(result, row, 0));
        track_name.push_back(PQgetvalue(result, row, 1));
        artist_name.push_back(PQgetvalue(result, row, 2));
        track_length.push_back(PQgetvalue(result, row, 3));
    }

    PQclear(result);

    return row;
}

int PgStore::getNextTrackId () const
{
    int         id(0);
    PGresult *  result(PQexec(m_db, "SELECT nextval('track_hash_sequence')"));
    
    if (PQntuples(result))
    {
        id = boost::lexical_cast<int>(PQgetvalue(result, 0, 0));
    }

    return id;
}
//------------------------------------------------
void PgStore::getTrackList (std::set<std::string>& list) const
{
    const char * const query = "SELECT hash FROM track";

    PGresult *      result(PQexec(m_db, query));

    for (int row = 0; row < PQntuples(result); ++row)
    {
        list.insert(PQgetvalue(result, row, 0));
    }

    PQclear(result);
}

bool PgStore::addTrack (const std::string& hash, const std::string& song_id, const std::string& song_title, const std::string& artist_id, const std::string& artist_name, int score)
{
    bool                    success(false);
    std::stringstream       ss;

    ss << "SELECT add_echonest_track('" << hash << "','" << song_id << "','" << pg_escape(song_title) << "','" << artist_id << "','" << pg_escape(artist_name) << "'," << score << ")";

    PGresult *      result(PQexec(m_db, ss.str().c_str()));

    success = PQcmdTuples(result) > 0;

    PQclear(result);

    return success;
}

bool PgStore::getLastHash (std::string& hash)
{
    bool    success(false);

    PGresult *  result(PQexec(m_db, "SELECT track_hash FROM echo_last"));

    if (PQntuples(result))
    {
        hash = PQgetvalue(result, 0, 0);
        success = true;
    }

    PQclear (result);

    return success;
}

//------------------------------------------------

std::string PgStore::pg_escape (const std::string& input) const
{
	std::string		output (input);

	boost::algorithm::replace_all (output, "\n", "<LF>");

	char *			buffer (new char [(output.size () << 1) + 1]);

	PQescapeStringConn (m_db, buffer, output.c_str (), output.size (), 0);

	output = buffer;
	delete [] buffer;
	return output;
}

//Not happy about the casts in here
std::string PgStore::pg_escape_byte (const std::string& input) const
{
	std::string		output (input);
	std::size_t		buffer_size(0);

	boost::algorithm::replace_all (output, "\n", "<LF>");

	unsigned char * buffer (PQescapeByteaConn (m_db, reinterpret_cast<unsigned const char *> (output.c_str ()), output.size (), &buffer_size));

	output.assign(reinterpret_cast<const char *>(buffer));

	PQfreemem(buffer);

	return output;
}

// Get the path component IDs
// Maybe we can replace this with a PL/PgSQL function some day.
void PgStore::get_parts (const std::string& input)
{
	std::string					s (input);

	m_parts.clear ();

	if (s.size())
	{
		std::vector <std::string>	tokens;
		PGresult	*				result;
		std::stringstream			ss;

		s.erase (0, 1);
		s.erase (s.size () - 1, 1);

		boost::iter_split (tokens, s, boost::first_finder (","));

		for (unsigned x = 0; x < tokens.size (); ++x)
		{
			ss.str ("");
			ss << "SELECT name FROM directory_name WHERE id = (SELECT name_id FROM directory WHERE id = " << tokens [x] << ")";

			result = PQexec (m_db, ss.str ().c_str ());

			m_parts.push_back (PQgetvalue (result, 0, 0));
			PQclear (result);
		}
	}
}
/*
void PgStore::set_tag (const std::string& tag_string, MetaTag& tag_data)
{
	std::string		tag (tag_string);
	unsigned		m (Choonz::TAG_TITLE);

	if (tag_data.size () != Choonz::TAG_MAX) tag_data.size (Choonz::TAG_MAX);

	tag.erase (tag.find_first_of ('{'), 1);
	tag.erase (tag.find_last_of ('}'), 1);

	boost::tokenizer <boost::escaped_list_separator <char> >	tok (tag);

	for (boost::tokenizer <boost::escaped_list_separator <char> >::iterator i = tok.begin (); i != tok.end (); ++i)
	{
		tag_data.set_tag (m, * i);
		++m;
	}
}
*/
/*
// This function tries to save a track record as plain text, but may fail if there are UTF-8 issues
bool PgStore::saveTrackAsText (const std::string& track_id, const std::string& filename, const MetaTag& fields, const std::vector <unsigned>& parts, unsigned parent)
{
	bool				success(false);

	if	(isOk())
	{
		PGresult *			result (0);
		std::stringstream	ss;

		ss << "INSERT INTO track (hash, filename, tag, parts, parent_id) VALUES ('" << track_id << "','" << pg_escape (filename) << "',ARRAY[";

        if (fields.size())
        {
            for (unsigned x = 0; x < fields.size (); ++x)
            {
                ss << "'" << pg_escape (fields [x]) << "'";
                if (x < fields.size () - 1)
                    ss << ",";
            }
        }
        else
        {
            ss << "'Unknown'";
        }

		ss << "],ARRAY[";

		for (unsigned x = 0; x < parts.size (); ++x)
		{
			ss << parts [x];

			if (x < parts.size () - 1)
			ss << ",";
		}

		ss << "]," << parent << ")";

		result = PQexec (m_db, ss.str ().c_str ());

		success = (PQresultStatus(result) == PGRES_COMMAND_OK);

		PQclear (result);

        if (!success)
            std::cerr << "saveAsText error: " << ss.str() << "\n";
	}

	return success;
}
*/

// No longer save tag data in the track record. Can fail if there's UTF-8 issues in which case saveTrackAsByte should be called.
bool PgStore::saveTrackAsText (const std::string& track_id, const std::string& filename, const MetaTag& fields, const std::vector <unsigned>& parts, unsigned parent)
{
    bool                success(false);

    if  (isOk())
    {
        PGresult *          result (0);
        std::stringstream   ss;

        ss << "INSERT INTO track (hash, filename, parts, parent_id) VALUES ('" << track_id << "','" << pg_escape (filename) << "',ARRAY[";

        for (unsigned x = 0; x < parts.size (); ++x)
        {
            ss << parts [x];

            if (x < parts.size () - 1)
                ss << ",";
        }

        ss << "]," << parent << ")";

        result = PQexec (m_db, ss.str ().c_str ());

        success = (PQresultStatus(result) == PGRES_COMMAND_OK);

        PQclear (result);

        if (success)
        {
            updateTag(track_id, fields);
        }
        else
        {
            std::cerr << "saveAsText error: " << ss.str() << "\n";
        }
    }

    return success;
}

// This function is tried if the one above fails. It treats input text as arrays of bytes and escapes them (turns them into text strings)
bool PgStore::saveTrackAsByte (const std::string& track_id, const std::string& filename, const MetaTag& fields, const std::vector <unsigned>& parts, unsigned parent)
{
	bool				success(false);

	if	(isOk())
	{
		PGresult *			result (0);
		std::stringstream	ss;

		ss << "INSERT INTO track (hash, filename, parts, parent_id, escaped) VALUES ('" << track_id << "','" << pg_escape_byte (filename) << "',ARRAY[";

		for (unsigned x = 0; x < parts.size (); ++x)
		{
			ss << parts [x];

			if (x < parts.size () - 1)
			ss << ",";
		}

		ss << "]," << parent << ",'TRUE')";

		result = PQexec (m_db, ss.str ().c_str ());

		success = (PQresultStatus(result) == PGRES_COMMAND_OK);

		PQclear (result);

        if (success)
        {
            updateTag(track_id, fields);
        }
        else
        {
            std::cerr << "saveAsByte error: " << ss.str() << "\n";
        }
	}

	return success;
}

// $Id: //adrian/choonz/track_manager.cpp#3 $
// $Change: 40 $

#include "track_manager.h"
#include "../library/tag_factory.h"
#include "../library/pg_store.h"
#include "../library/defs.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include <memory>
#include <cstring>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/tokenizer.hpp>
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

const char * TrackManager::m_supported_extensions [] = { ".mp3", ".flac", ".ogg", ".wav", 0 };

TrackManager::TrackManager (const std::string& host) :
		Runnable (),
		m_db (0),
		m_host (host),
		m_status (statusReady)
{
}

TrackManager::~TrackManager ()
{
	delete m_db;
}

void TrackManager::run ()
{
	m_db = new PgStore (m_host);

	if (m_db -> isOk ())
	{
		do
		{
			if (m_locations_pending.size ())
			{

				m_status = statusSearch;
				status (0);

				std::vector <unsigned>		v;
				//std::string					location (m_locations_pending.front ());
				std::string  location("/home/media");

				m_locations_pending.pop ();

				std::cout << "Starting scan: " << location << std::endl;

				recursive_scan (location, location, v, 0);

                		std::cout << "Scan completed" << std::endl;

				m_status = statusReady;
				status (0);
			}

			sleep (2);
		} while (m_continue);
	}

	std::cout << "TrackManager done\n";
}

// Send the library in artist/album view

bool TrackManager::getAlbums (std::string& output)
{
    bool                        success(false);
    std::vector<int>            artist_id;
    std::vector<std::string>    artist_name;

//    m_message_id = m_net_helper.messageBegin (Choonz::ALBUMS, Choonz::SUCCESS);
    std::stringstream       ss;

 //   ss << "BEGIN " << Choonz::ALBUMS << " " << Choonz::SUCCESS << "\n";
 //   m_net_helper.sendImmediate(ss.str(), client);

    if (m_db->getArtistList(artist_id, artist_name))
    {
        for (unsigned x = 0; x < artist_id.size(); ++x)
        {
            m_stream << artist_name[x] << " {\n";
            sendAlbums(artist_id [x]);
            m_stream << "}\n";
        }

        output = m_stream.str();
        success = true;
    }

//    m_net_helper.sendImmediate("END\n", client);

    return success;
}

/*
// Send the library in chunks - see if it fixes missing albums e.g. Concerto Moon
void TrackManager::sendAlbumsToClient (std::size_t client)
{
    std::vector<int>            artist_id;
    std::vector<std::string>    artist_name;

    std::cout << "BEGIN " << Choonz::ALBUMS << " " << Choonz::SUCCESS << "\n";

    if (m_db->getArtistList(artist_id, artist_name))
    {
        for (unsigned x = 0; x < artist_id.size(); ++x)
        {
            std::cout << artist_name[x] << " {\n";
            sendAlbums(artist_id [x]);
            std::cout << "}\n";
        }
    }

    std::cout << "END\n";
}
*/

bool TrackManager::getArtists (std::string& output)
{
    bool                        success(false);
    /*
    std::vector<int>            album_id;
    std::vector<std::string>    album_name;

    std::stringstream       ss;

    ss << "BEGIN " << Choonz::ARTISTS << " " << Choonz::SUCCESS << "\n";
    m_net_helper.sendImmediate(ss.str(), client);

    if (m_db->getAlbumList(album_id, album_name))
    {
        for (unsigned x = 0; x < album_id.size(); ++x)
        {
            m_stream << album_name[x] << " {\n";
            sendAlbumTracks(album_id [x]);
            m_stream << "}\n";
            m_net_helper.sendImmediate(m_stream.str(), client);
            m_stream.str("");
        }
    }

    m_net_helper.sendImmediate("END\n", client);
    */

    output = m_stream.str();

    m_stream.str("");

    return success;
}

// Send the library in file view
bool TrackManager::getTracks (std::string& output)
{
    bool                        success(false);
	std::vector<int>			location_id;

//	m_message_id = m_net_helper.messageBegin (Choonz::LIBRARY, Choonz::SUCCESS);

	m_stream.str("Choonz {\n");

	if (m_db->getLocations(location_id))
	{
		for (unsigned location = 0; location < location_id.size(); ++location)
		{
			expandLocation(location_id [location]);
		}

        success = true;
	}

	m_stream << "}\n";

    output = m_stream.str();

    return success;
}

//move this code to the bottom when complete
void TrackManager::expandLocation (unsigned id)
{
	std::string			name;

	if (m_db->getLocationName(id, name))
	{
		std::vector<int>	children;

		m_stream << name << " {\n";

		expandTracks(id);

		if (m_db->getChildren(id, children))
		{
			for (unsigned child = 0; child < children.size(); ++ child)
			{
				expandLocation(children [child]);
			}
		}

		m_stream << "}\n";
	}
}

void TrackManager::expandTracks (unsigned id)
{
	std::vector<std::string>		hashes;
	std::vector<std::string>		names;
    std::vector<std::string>        artists;
    std::vector<std::string>        albums;
    std::vector<std::string>        lengths;

	if (m_db->getLocationTracks(id, hashes, names, artists, albums, lengths))
	{
		for (unsigned x = 0; x < hashes.size(); ++x)
		{
			m_stream << "<" << hashes[x] << ">|" << names [x] << "|" << artists[x] << "|" << albums[x] << "|" << lengths[x] << "\n";
		}
	}
}

// We probably don't need this function any more
bool TrackManager::getMetadata (std::string& output)
{
    bool                        success(false);
	std::string					track_id;
	TagFactory					tf;
	std::stringstream			strm;
	std::unique_ptr <MetaTag>		tag_data (tf.getTag (""));

//	m_message_id = m_net_helper.messageBegin (Choonz::TAG_LIST, Choonz::SUCCESS);
/*
	while (m_db -> next_tag (track_id, *tag_data))
	{
		strm.str ("");

		strm << track_id;

		for (unsigned index = Choonz::TAG_TITLE; index < Choonz::TAG_MAX; index ++)
			strm << "|" << tag_data -> operator [] (index);

		strm << "\n";
		m_net_helper.messageBody (m_message_id, strm.str ());

		tag_data -> clear ();
	}
*/
//	m_net_helper.messageSend (m_message_id, client);

    return success;
}

void TrackManager::rescan ()
{
}

void TrackManager::rescan (const std::string&)
{
}

// This can update filename and meta data in one step
// This function to be updated for new db schema (separate tag table)
bool TrackManager::modify (const std::string& data)
{
	bool							result (false);
	TagFactory						tf;
	std::vector <std::string>		tokens;
	std::string						sep ("|");

	boost::iter_split (tokens, data, boost::first_finder ("|"));

	std::string						old_hash (tokens.at (0));

	rename (tokens); // This supplies the filename as token.at (1) so we can create the correct tag

	std::unique_ptr <MetaTag>			mtag (tf.getTag (tokens.at (1)));

//	mtag -> size (Choonz::TAG_MAX); TODO: fix or remove

    /*
	if (m_db -> get_tag (tokens.at (0), * mtag))
	{
		apply_tag_changes (* mtag, std::vector <std::string> (tokens.begin () + 2, tokens.end ()));

		result = m_db -> update_tag (tokens.at (0), * mtag);

		if (result)
		{
			mtag -> writeTag (std::vector <std::string> (tokens.begin () + 2, tokens.end ()));
			m_net_helper.libraryModified (0, old_hash);
		}
	}
*/
	return result;
}

void TrackManager::addLocation (const std::string& location)
{
	m_locations_pending.push (location);
}

const std::string TrackManager::getTrackNamePath (const std::string& track_id) const
{
	return m_db -> getTrack(track_id);
}

void TrackManager::status (std::size_t client)
{
    /*
	std::stringstream			ss;

	m_message_id = m_net_helper.messageBegin (Choonz::STATUS, Choonz::INFORMATION);

	switch (m_status)
	{
		case statusReady:
			ss << "Ready.";
			break;
		case statusSearch:
			ss << "Scanning location " << m_locations_pending.front ();
			break;
		case statusRead:
			ss << "Reading library " << m_locations_pending.front ();
			break;
		case statusRescan:
			ss << "Rescanning.";
	}

	ss << "\n";

	ss << "Tracks: " << m_db -> track_count () << " Locations: " << m_db-> locations() << "\n";

	m_net_helper.messageBody (m_message_id, ss.str ());
	m_net_helper.messageSend (m_message_id, client);
    */
}

const std::vector <std::string> TrackManager::createPlaylist () const
{
	return std::vector <std::string> ();
}

void TrackManager::recursive_scan (const std::string& top_level, const std::string& sub_level, std::vector <unsigned>& v, unsigned parent_id)
{
	DIR *						dirp;

	if ((dirp = opendir (top_level.c_str ())))
	{
		struct dirent *				dent;
		std::vector <unsigned>		parts (v);
		unsigned					new_id (0);

                std::cout << "Opened: " << top_level << std::endl;

		new_id = m_db -> addDirectory(sub_level, parent_id);
		parts.push_back (new_id);

		while ((dent = readdir (dirp)))
		{
			if (dent -> d_name [0] != '.')
			{
				struct stat		st;

				stat (full_path (top_level, dent -> d_name).c_str (), &st);

				if (S_ISDIR (st.st_mode))
				{
					recursive_scan (full_path (top_level, dent -> d_name), dent -> d_name, parts, new_id);
				}
				else
				{
					for (const char * * ext = m_supported_extensions; * ext; ext ++)
					{
						if (boost::algorithm::iends_with (dent -> d_name, * ext))
						{
							std::string			track_hash (boost::lexical_cast <std::string> (boost::hash <std::string> () (full_path (top_level, dent -> d_name))));

							if (!m_db -> trackExists (track_hash))
							{
								TagFactory		tf;

								std::unique_ptr<MetaTag>	mtag (tf.getTag (full_path (top_level, dent -> d_name)));

								if (mtag.get ())
								{
									mtag -> readTag ();
								}
								else
									mtag -> size (Choonz::TAG_MAX);

								if (m_db->addTrack(track_hash, dent -> d_name, * mtag, parts, new_id))
                                    m_db->addAlbumTrack(track_hash, * mtag);
							}
							break;
						}
					}
				}
			}
		}

		closedir (dirp) ;
	}
        else
        {
             int   err(errno);
             std::cout << "ERROR opening " << top_level.c_str() << std::endl;
             std::cout << "Cause is " << strerror(err) << std::endl;
        }
}

std::string TrackManager::full_path (const std::string& root, const std::string& subdir) const
{
	std::string	result (root);

	result += "/" + subdir;

	return result;
}

// This code assumes the new name has a full path
// Track id is first item, new name is second, but may be empty
// This code does not cope with any path component changing
// It also places the existing name at params [1]
bool TrackManager::rename (std::vector <std::string>& params)
{
	bool						success (false);
    /*
	std::string					track_hash (params.at (0));
	boost::filesystem::path		old_name (m_db -> getTrack (params.at (0)));
	boost::filesystem::path		new_name (params.at (1));

	if (params.at (1).size ())
	{
		try
		{
			boost::filesystem::rename (old_name, new_name);
			track_hash = boost::lexical_cast <std::string> (boost::hash <std::string> () (params.at (1)));

			std::string		name_only (params.at (1).substr (params.at (1).find_last_of ('/') + 1));

			m_db -> renameTrack (params.at (0), track_hash, name_only); // FIXME: We shouldn't assume the db update succeeds
			m_net_helper.fileRename (params.at (0), track_hash, params.at (1));
			params [0] = track_hash;
		}
		catch (boost::filesystem::filesystem_error e)
		{
			std::cerr << "FileManager::rename exception: " << e.what () << "\n";
			m_net_helper.fileRename (track_hash, "", params.at (1));
			success = false;
		}
	}
	else
	{
		params [1] = m_db -> get_track (params.at (0)); // We need the track name so we can create a MetaTag for it
	}
*/
	return success;
}

void TrackManager::apply_tag_changes (MetaTag& m, const std::vector <std::string>& d) const
{
	m.set_tag (Choonz::TAG_TITLE, d.at (Choonz::SAVE_TITLE));
	m.set_tag (Choonz::TAG_ARTIST, d.at (Choonz::SAVE_ARTIST));
	m.set_tag (Choonz::TAG_ALBUM, d.at (Choonz::SAVE_ALBUM));
	m.set_tag (Choonz::TAG_GENRE, d.at (Choonz::SAVE_GENRE));
	m.set_tag (Choonz::TAG_YEAR, d.at (Choonz::SAVE_YEAR));
	m.set_tag (Choonz::TAG_TRACK, d.at (Choonz::SAVE_TRACK));
	m.set_tag (Choonz::TAG_COMMENT, d.at (Choonz::SAVE_COMMENT));
}

void TrackManager::sendAlbums(int artist_id)
{
    std::vector<int>            album_ids;
    std::vector<std::string>    album_names;

    if (m_db->getAlbums(artist_id, album_ids, album_names))
    {
        for (unsigned x = 0; x < album_ids.size(); ++x)
        {
            m_stream << album_names [x] << " {\n";

            sendAlbumTracks(album_ids [x]);

            m_stream << "}\n";
        }
    }
}

void TrackManager::sendAlbumTracks (int album_id)
{
    std::vector<std::string>    track_hash;
    std::vector<std::string>    track_name;
    std::vector<std::string>    artist_name;
    std::vector<std::string>    track_length;

    if (m_db->getTrackAndArtist(album_id, track_hash, track_name, artist_name, track_length))
    {
        for (unsigned x = 0; x < track_hash.size(); ++x)
        {
            m_stream << "<" << track_hash [x] << ">|" << track_name [x] << "|" << artist_name[x] << "|" << track_length[x] << "\n"; // The GUI uses '|' as a new column indicator
        }
    }
}

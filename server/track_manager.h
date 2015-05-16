// $Id: //adrian/choonz/track_manager.h#2 $
// $Change: 3 $

#ifndef TRACK_MANAGER_H_DEFINED
#define TRACK_MANAGER_H_DEFINED

#include "../library/runnable.h"
#include <string>
#include <vector>
#include <queue>
#include <sstream>

class PgStore;
class NetHelper;
class MetaTag;

class TrackManager : public Runnable
{
public:
	explicit TrackManager (const std::string&);
	~TrackManager ();
	void run ();
	bool isOk () const { return m_db != 0; }
	bool getTracks (std::string&);
    bool getAlbums (std::string&);
    bool getArtists (std::string&);
	bool getMetadata (std::string&);
//	bool writeTag (const std::string&, MetaTag&);
	void rescan (); //Perform a full rescan of all locations
	void rescan (const std::string&); // Only rescan the named directory
	bool modify (const std::string&); // Rename track and modify tag in a single function
	void addLocation (const std::string&);
	const std::string getTrackNamePath (const std::string&) const; // Get the full pathname of the track
	void status (std::size_t);
	const std::vector <std::string> createPlaylist () const;

private:
	enum						Status { statusReady, statusSearch, statusRead, statusRescan };
	static const char * 		m_supported_extensions [];
	PgStore *					m_db;
	std::string					m_host;
	std::size_t					m_message_id; //NetHelper message Id
	std::queue <std::string>	m_locations_pending;
	Status						m_status;
	std::stringstream			m_stream;
	void						recursive_scan (const std::string&, const std::string&, std::vector <unsigned>&, unsigned);
	std::string					full_path (const std::string&, const std::string&) const;
	bool						rename (std::vector <std::string>&);
	void						apply_tag_changes (MetaTag&, const std::vector <std::string>&) const; // Update the tag with user's changes
	void						expandLocation (unsigned);
	void						expandTracks (unsigned); // Output all tracks belonging to a location
    void                        sendAlbums (int); // Send all albums for a specific artist
    void                        sendAlbumTracks (int); // Send all tracks for a specific album

};

#endif

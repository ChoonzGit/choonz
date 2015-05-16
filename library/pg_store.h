// $Id: //adrian/choonz/choonz_library/main/pg_store.h#3 $
// PostgreSQL storage, uses choonz2.sql schema file.

#ifndef PG_STORE_H_DEFINED
#define PG_STORE_H_DEFINED

#include <libpq-fe.h>
#include <set>
#include <string>
#include <vector>

class MetaTag;

class PgStore
{
public:
    explicit PgStore (const std::string&);
    ~PgStore ();
    bool        readLibrary ();
    unsigned    addDirectory (const std::string&, unsigned);
    bool        addTrack (const std::string&, const std::string&, const MetaTag&, const std::vector <unsigned>&, unsigned);
    bool        initialise ();
    bool        isOk () const { return m_db != 0; }
    std::string getTrack (const std::string&);
    std::string getTrack (std::size_t);
    bool        trackExists (const std::string&);
    unsigned    trackCount ();
//  bool        next_tag (std::string&, MetaTag&);
//  bool        get_tag (const std::string&, MetaTag&);
    bool        renameTrack (const std::string&, const std::string&, const std::string&);
    bool        updateTag (const std::string&, const MetaTag&);
    void        empty();
    bool        getListIds(std::vector <std::string>&);
    bool        getList(const std::string&, std::vector <std::string>&);
    bool        openPlaylist(unsigned, std::string&, std::vector<std::string>&);
    bool        savePlaylist(std::size_t&, const std::string&, const std::string&, std::vector<std::string>&);
    bool        getLocations(std::vector<int>&); // We just want directory IDs whose parent_id is 0
    bool        getChildren(int, std::vector<int>&); // Get all the IDs of the children of a location
    bool        getLocationName(int, std::string&); // Get this location's named, based on ID
    bool        getLocationTracks(int, std::vector<std::string>&, std::vector<std::string>&, std::vector<std::string>&, std::vector<std::string>&, std::vector<std::string>&); // Get track hashes, filenames and meta tags in this location
    unsigned    locations(); //Need to make const - replace with a call to a SQL function
    unsigned    addAlbumTrack(const std::string&, const MetaTag&);
    int         getArtistList(std::vector<int>&, std::vector<std::string>&);
    int         getAlbumList(std::vector<int>&, std::vector<std::string>&);
    int         getAlbums(int, std::vector<int>&, std::vector<std::string>&);
    int         getTracks(int, std::vector<std::string>&, std::vector<std::string>&); // Get all track IDs for a given artist album ID.
    int         getTrackAndArtist(int, std::vector<std::string>&, std::vector<std::string>&, std::vector<std::string>&, std::vector<std::string>&); // Get track IDs, names, and artists for a given album ID
    int         getNextTrackId() const;
    // Echonest stuff
    void        getTrackList (std::set<std::string>&) const; // Get all the track ID in alphabetical order
    bool        addTrack (const std::string&, const std::string&, const std::string&, const std::string&, const std::string&, int);
    bool        getLastHash (std::string&); // Get the last hash successfully queried

private:
    static const char *         m_field_name [12];
    PGconn *                    m_db;
    PGresult *                  m_result; //Used to fetch all tracks & tags
    std::string                 m_host;
    std::vector <std::string>   m_parts;
    int                         m_current_row; // The current track to get
    std::string                 pg_escape (const std::string&) const;
    std::string                 pg_escape_byte (const std::string&) const;
    void                        get_parts (const std::string&);
//  void                        set_tag (const std::string&, MetaTag&);
    bool                        saveTrackAsText (const std::string&, const std::string&, const MetaTag&, const std::vector <unsigned>&, unsigned); // Save filenames and tags as plain text
    bool                        saveTrackAsByte (const std::string&, const std::string&, const MetaTag&, const std::vector <unsigned>&, unsigned); // Save filenames and tags as escaped bytea fields
};

#endif

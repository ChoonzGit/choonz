--
-- PostgreSQL database dump
--

SET statement_timeout = 0;
SET lock_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET client_min_messages = warning;

--
-- Name: plpgsql; Type: EXTENSION; Schema: -; Owner: 
--

CREATE EXTENSION IF NOT EXISTS plpgsql WITH SCHEMA pg_catalog;


--
-- Name: EXTENSION plpgsql; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION plpgsql IS 'PL/pgSQL procedural language';


SET search_path = public, pg_catalog;

--
-- Name: add_album_track(text, text, text); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION add_album_track(text, text, text) RETURNS integer
    LANGUAGE plpgsql
    AS $_$
DECLARE
    artist_name     ALIAS FOR $1;
    album_name      ALIAS FOR $2;
    track_id        ALIAS FOR $3;
    album_artist_id integer;
    album_global_id integer;
    artist_id       integer;
    temp_id         integer;
BEGIN
    SELECT id INTO artist_id FROM artist WHERE name = artist_name;

    IF NOT FOUND THEN
        SELECT INTO artist_id nextval('artist_id_sequence');
        INSERT INTO artist(id, name) VALUES (artist_id, artist_name);
    END IF;

    SELECT name_id INTO album_global_id FROM global_album WHERE name = album_name;

    IF NOT FOUND THEN
        SELECT INTO album_global_id nextval('global_album_id_sequence');
        INSERT INTO global_album (name_id, name) VALUES (album_global_id, album_name);
    END IF;

    SELECT id INTO album_artist_id FROM artist_album WHERE name_id = album_global_id AND owner_id = artist_id;

    IF NOT FOUND THEN
        SELECT INTO album_artist_id nextval('artist_album_id_sequence');
        INSERT INTO artist_album (id, owner_id, name_id) VALUES (album_artist_id, artist_id, album_global_id);
    END IF;

    SELECT id INTO temp_id FROM artist WHERE id = artist_id AND album_artist_id = ANY (album_list);

    IF NOT FOUND THEN
        UPDATE artist SET album_list = album_list || ARRAY [album_artist_id] WHERE id = artist_id;
    END IF;

    UPDATE artist_album SET track_hash = track_hash || ARRAY [track_id] WHERE id = album_artist_id;

    INSERT INTO album_track(name_id, artist_id, track_id) VALUES (album_global_id, artist_id, track_id);

    RETURN album_artist_id;
END;
$_$;


ALTER FUNCTION public.add_album_track(text, text, text) OWNER TO postgres;

--
-- Name: add_directory(text, integer); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION add_directory(text, integer) RETURNS integer
    LANGUAGE plpgsql
    AS $_$
DECLARE
	new_directory	ALIAS FOR $1;
	parent			ALIAS FOR $2;
	new_name_id		integer;
	new_id			integer;
BEGIN
	SELECT id INTO new_name_id FROM directory_name WHERE name = new_directory;

	IF NOT FOUND THEN
		SELECT INTO new_name_id nextval('directory_name_sequence');
		INSERT INTO directory_name(id, name) VALUES (new_name_id, new_directory);
	END IF;

	SELECT id INTO new_id FROM directory WHERE parent_id = parent AND name_id = new_name_id;

	IF NOT FOUND THEN
		SELECT INTO new_id nextval('directory_id_sequence');
		INSERT INTO directory (id, name_id, parent_id) VALUES (new_id, new_name_id, parent);
		UPDATE directory SET children = children || ARRAY [new_id] WHERE id = parent;
	END IF;

	RETURN new_id;
END;
$_$;


ALTER FUNCTION public.add_directory(text, integer) OWNER TO postgres;

--
-- Name: add_playlist(text); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION add_playlist(text) RETURNS integer
    LANGUAGE plpgsql
    AS $_$
DECLARE
	new_playlist	ALIAS FOR $1;
	new_id			integer;
BEGIN
	SELECT id INTO new_id FROM playlist WHERE name = new_playlist;

	IF NOT FOUND THEN
		SELECT INTO new_id nextval('playlist_id_sequence');
		INSERT INTO playlist (id, name) VALUES (new_id, new_playlist);
	END IF;

	RETURN new_id;
END;
$_$;


ALTER FUNCTION public.add_playlist(text) OWNER TO postgres;

--
-- Name: album_list(integer); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION album_list(integer) RETURNS TABLE(id integer, albumname text)
    LANGUAGE sql
    AS $_$
    SELECT artist_album.id, global_album.name FROM artist_album INNER JOIN global_album USING(name_id) WHERE artist_album.id IN (SELECT unnest(album_list) FROM artist WHERE owner_id = $1);
$_$;


ALTER FUNCTION public.album_list(integer) OWNER TO postgres;

--
-- Name: auto_add_tag(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION auto_add_tag() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
BEGIN
    IF (TG_OP = 'INSERT') THEN
        INSERT INTO tag (track_hash) VALUES (NEW.hash);
        RETURN NEW;
    END IF;
    RETURN NULL;
END;
$$;


ALTER FUNCTION public.auto_add_tag() OWNER TO postgres;

--
-- Name: global_album_list(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION global_album_list() RETURNS TABLE(id integer, albumname text)
    LANGUAGE sql
    AS $$
    SELECT name_id, name FROM global_album;
$$;


ALTER FUNCTION public.global_album_list() OWNER TO postgres;

--
-- Name: playlist_files(integer); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION playlist_files(integer) RETURNS TABLE(id text, filename text)
    LANGUAGE sql
    AS $_$
	SELECT hash, filename FROM track WHERE hash IN (SELECT unnest(track_hash) FROM playlist WHERE id = $1);
$_$;


ALTER FUNCTION public.playlist_files(integer) OWNER TO postgres;

--
-- Name: track_artist_list(integer); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION track_artist_list(integer) RETURNS TABLE(track_hash text, title text, artist text)
    LANGUAGE sql
    AS $_$
    SELECT track_hash, title, artist FROM tag WHERE track_hash IN (SELECT track_id FROM album_track WHERE name_id = $1);
$_$;


ALTER FUNCTION public.track_artist_list(integer) OWNER TO postgres;

--
-- Name: track_count(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION track_count() RETURNS integer
    LANGUAGE plpgsql
    AS $$
DECLARE
	result		integer;
BEGIN
	SELECT INTO result count(*) FROM track;
	RETURN result;
END;
$$;


ALTER FUNCTION public.track_count() OWNER TO postgres;

--
-- Name: track_exists(text); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION track_exists(text) RETURNS boolean
    LANGUAGE plpgsql
    AS $_$
DECLARE
	track_hash		ALIAS FOR $1;
	parent			integer;
BEGIN
	SELECT parent_id INTO parent FROM track WHERE hash = track_hash;
	RETURN FOUND;
END;
$_$;


ALTER FUNCTION public.track_exists(text) OWNER TO postgres;

--
-- Name: track_list(integer); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION track_list(integer) RETURNS TABLE(id text, title text)
    LANGUAGE sql
    AS $_$
    SELECT track_hash, title FROM tag WHERE track_hash IN (SELECT unnest (track_hash) FROM artist_album WHERE id = $1);
$_$;


ALTER FUNCTION public.track_list(integer) OWNER TO postgres;

--
-- Name: track_location(integer); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION track_location(integer) RETURNS TABLE(track_hash text, track_filename text, tag_artist text, tag_year text, tag_rate text, tag_length text)
    LANGUAGE sql
    AS $_$
    SELECT track.hash, track.filename, tag.artist, tag.year, tag.bitrate, tag.length FROM track INNER JOIN tag ON (tag.track_hash = track.hash) WHERE parent_id = $1;
$_$;


ALTER FUNCTION public.track_location(integer) OWNER TO postgres;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: album_track; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE album_track (
    name_id integer NOT NULL,
    artist_id integer,
    track_id text
);


ALTER TABLE public.album_track OWNER TO postgres;

--
-- Name: artist; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE artist (
    id integer NOT NULL,
    name text,
    echonest_id text,
    album_list integer[]
);


ALTER TABLE public.artist OWNER TO postgres;

--
-- Name: artist_album; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE artist_album (
    id integer NOT NULL,
    owner_id integer NOT NULL,
    name_id integer NOT NULL,
    track_hash text[]
);


ALTER TABLE public.artist_album OWNER TO postgres;

--
-- Name: artist_album_id_sequence; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE artist_album_id_sequence
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.artist_album_id_sequence OWNER TO postgres;

--
-- Name: artist_id_sequence; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE artist_id_sequence
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.artist_id_sequence OWNER TO postgres;

--
-- Name: directory; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE directory (
    id integer NOT NULL,
    name_id integer NOT NULL,
    parent_id integer NOT NULL,
    children integer[]
);


ALTER TABLE public.directory OWNER TO postgres;

--
-- Name: directory_id_sequence; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE directory_id_sequence
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.directory_id_sequence OWNER TO postgres;

--
-- Name: directory_name; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE directory_name (
    id integer NOT NULL,
    name text
);


ALTER TABLE public.directory_name OWNER TO postgres;

--
-- Name: directory_name_sequence; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE directory_name_sequence
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.directory_name_sequence OWNER TO postgres;

--
-- Name: global_album; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE global_album (
    name_id integer NOT NULL,
    name text
);


ALTER TABLE public.global_album OWNER TO postgres;

--
-- Name: global_album_id_sequence; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE global_album_id_sequence
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.global_album_id_sequence OWNER TO postgres;

--
-- Name: playlist; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE playlist (
    id integer NOT NULL,
    name text NOT NULL,
    comment text,
    track_hash text[]
);


ALTER TABLE public.playlist OWNER TO postgres;

--
-- Name: playlist_id_sequence; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE playlist_id_sequence
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.playlist_id_sequence OWNER TO postgres;

--
-- Name: tag; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE tag (
    track_hash text NOT NULL,
    title text,
    artist text,
    album text,
    genre text,
    year text,
    track text,
    length text,
    bitrate text,
    samplerate text,
    channels text,
    comment text
);


ALTER TABLE public.tag OWNER TO postgres;

--
-- Name: track; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE track (
    hash text NOT NULL,
    parent_id integer NOT NULL,
    escaped boolean DEFAULT false,
    filename text,
    echonest_id text,
    parts integer[]
);


ALTER TABLE public.track OWNER TO postgres;

--
-- Name: track_hash_sequence; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE track_hash_sequence
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.track_hash_sequence OWNER TO postgres;

--
-- Name: artist_album_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY artist_album
    ADD CONSTRAINT artist_album_pkey PRIMARY KEY (id);


--
-- Name: artist_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY artist
    ADD CONSTRAINT artist_pkey PRIMARY KEY (id);


--
-- Name: directory_name_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY directory_name
    ADD CONSTRAINT directory_name_pkey PRIMARY KEY (id);


--
-- Name: directory_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY directory
    ADD CONSTRAINT directory_pkey PRIMARY KEY (id);


--
-- Name: global_album_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY global_album
    ADD CONSTRAINT global_album_pkey PRIMARY KEY (name_id);


--
-- Name: playlist_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY playlist
    ADD CONSTRAINT playlist_pkey PRIMARY KEY (id);


--
-- Name: tag_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY tag
    ADD CONSTRAINT tag_pkey PRIMARY KEY (track_hash);


--
-- Name: track_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY track
    ADD CONSTRAINT track_pkey PRIMARY KEY (hash);


--
-- Name: album_global_idx; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX album_global_idx ON album_track USING btree (name_id);


--
-- Name: artist_album_name_id_idx; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX artist_album_name_id_idx ON artist_album USING btree (name_id);


--
-- Name: artist_album_owner_id_idx; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX artist_album_owner_id_idx ON artist_album USING btree (owner_id);


--
-- Name: artist_echonest_index; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX artist_echonest_index ON artist USING btree (echonest_id);


--
-- Name: artist_name_idx; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX artist_name_idx ON artist USING btree (name);


--
-- Name: directory_name_index; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX directory_name_index ON directory_name USING btree (name);


--
-- Name: directory_parent_id_index; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX directory_parent_id_index ON directory USING btree (parent_id, id);


--
-- Name: global_album_name_idx; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX global_album_name_idx ON global_album USING btree (name);


--
-- Name: track_echonest_index; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX track_echonest_index ON track USING btree (echonest_id);


--
-- Name: track_parent_index; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX track_parent_index ON track USING btree (parent_id);


--
-- Name: auto_tag; Type: TRIGGER; Schema: public; Owner: postgres
--

CREATE TRIGGER auto_tag AFTER INSERT ON track FOR EACH ROW EXECUTE PROCEDURE auto_add_tag();


--
-- Name: album_track_name_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY album_track
    ADD CONSTRAINT album_track_name_id_fkey FOREIGN KEY (name_id) REFERENCES global_album(name_id);


--
-- Name: artist_album_name_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY artist_album
    ADD CONSTRAINT artist_album_name_id_fkey FOREIGN KEY (name_id) REFERENCES global_album(name_id);


--
-- Name: artist_album_owner_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY artist_album
    ADD CONSTRAINT artist_album_owner_id_fkey FOREIGN KEY (owner_id) REFERENCES artist(id);


--
-- Name: directory_name_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY directory
    ADD CONSTRAINT directory_name_id_fkey FOREIGN KEY (name_id) REFERENCES directory_name(id);


--
-- Name: tag_track_hash_fkey; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY tag
    ADD CONSTRAINT tag_track_hash_fkey FOREIGN KEY (track_hash) REFERENCES track(hash);


--
-- Name: track_parent_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY track
    ADD CONSTRAINT track_parent_id_fkey FOREIGN KEY (parent_id) REFERENCES directory(id);


--
-- Name: public; Type: ACL; Schema: -; Owner: postgres
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM postgres;
GRANT ALL ON SCHEMA public TO postgres;
GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- PostgreSQL database dump complete
--


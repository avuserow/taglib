/***************************************************************************
    copyright            : (C) 2002 - 2008 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License version   *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 *   Alternatively, this file is available under the Mozilla Public        *
 *   License Version 1.1.  You may obtain a copy of the License at         *
 *   http://www.mozilla.org/MPL/                                           *
 ***************************************************************************/

#include "tagunion.h"

using namespace TagLib;

#define stringUnion(method)                                          \
  if(tag(0) && !tag(0)->method().isEmpty())                          \
    return tag(0)->method();                                         \
  if(tag(1) && !tag(1)->method().isEmpty())                          \
    return tag(1)->method();                                         \
  if(tag(2) && !tag(2)->method().isEmpty())                          \
    return tag(2)->method();                                         \
  return String::null                                                \

#define numberUnion(method)                                          \
  if(tag(0) && tag(0)->method() > 0)                                 \
    return tag(0)->method();                                         \
  if(tag(1) && tag(1)->method() > 0)                                 \
    return tag(1)->method();                                         \
  if(tag(2) && tag(2)->method() > 0)                                 \
    return tag(2)->method();                                         \
  return 0

#define setUnion(method, value)                                      \
  if(tag(0))                                                         \
    tag(0)->set##method(value);                                      \
  if(tag(1))                                                         \
    tag(1)->set##method(value);                                      \
  if(tag(2))                                                         \
    tag(2)->set##method(value);                                      \

class TagUnion::TagUnionPrivate
{
public:
  TagUnionPrivate() : tags(3, static_cast<Tag *>(0))
  {

  }

  ~TagUnionPrivate()
  {
    delete tags[0];
    delete tags[1];
    delete tags[2];
  }

  std::vector<Tag *> tags;
};

TagUnion::TagUnion(Tag *first, Tag *second, Tag *third)
{
  d = new TagUnionPrivate;

  d->tags[0] = first;
  d->tags[1] = second;
  d->tags[2] = third;
}

TagUnion::~TagUnion()
{
  delete d;
}

Tag *TagUnion::operator[](int index) const
{
  return tag(index);
}

Tag *TagUnion::tag(int index) const
{
  return d->tags[index];
}

void TagUnion::set(int index, Tag *tag)
{
  delete d->tags[index];
  d->tags[index] = tag;
}

#pragma mark -

String TagUnion::title() const
{
  stringUnion(title);
}

String TagUnion::artist() const
{
  stringUnion(artist);
}

String TagUnion::album() const
{
  stringUnion(album);
}

String TagUnion::comment() const
{
  stringUnion(comment);
}

String TagUnion::genre() const
{
  stringUnion(genre);
}

String TagUnion::lyrics() const
{
	stringUnion(lyrics);
}

TagLib::uint TagUnion::year() const
{
  numberUnion(year);
}

TagLib::uint TagUnion::track() const
{
  numberUnion(track);
}

String TagUnion::albumArtist() const
{
	stringUnion(albumArtist);
}

String TagUnion::grouping() const
{
	stringUnion(grouping);
}

String 
TagUnion::composer() const
{
	stringUnion(composer);
}

TagLib::uint TagUnion::totalTracks() const
{
	numberUnion(totalTracks);
}

TagLib::uint TagUnion::cdNr() const
{
	numberUnion(cdNr);
}

TagLib::uint TagUnion::totalCDs() const
{
	numberUnion(totalCDs);
}

TagLib::uint 
TagUnion::bpm() const
{
	numberUnion(bpm);
}



#pragma mark -

void TagUnion::setTitle(const String &s)
{
  setUnion(Title, s);
}

void TagUnion::setArtist(const String &s)
{
  setUnion(Artist, s);
}

void TagUnion::setAlbum(const String &s)
{
  setUnion(Album, s);
}

void TagUnion::setComment(const String &s)
{
  setUnion(Comment, s);
}

void TagUnion::setGenre(const String &s)
{
  setUnion(Genre, s);
}

int TagUnion::setLyrics(const String &s)
{
  setUnion(Lyrics, s);
  return 1;
}

void TagUnion::setYear(TagLib::uint i)
{
  setUnion(Year, i);
}

void TagUnion::setTrack(TagLib::uint i)
{
  setUnion(Track, i);
}

int TagUnion::setAlbumArtist(const String &s)
{
	setUnion(AlbumArtist, s);
	return 1;
}

int  TagUnion::setGrouping(const String &s)
{
	setUnion(Grouping, s);
	return 1;
}

int  TagUnion::setComposer(const String &s)
{
	setUnion(Composer, s);
	return 1;
}

int  TagUnion::setTotalTracks(TagLib::uint i)
{
	setUnion(TotalTracks, i);
	return 1;
}

int  TagUnion::setCDNr(TagLib::uint i)
{
	setUnion(CDNr, i);
	return 1;
}

int  TagUnion::setTotalCDs(TagLib::uint i)
{
	setUnion(TotalCDs, i);
	return 1;
}

int  TagUnion::setBPM(TagLib::uint i)
{
	setUnion(BPM, i);
	return 1;
}


#pragma mark -

bool TagUnion::isEmpty() const
{
  if(d->tags[0] && !d->tags[0]->isEmpty())
    return false;
  if(d->tags[1] && !d->tags[1]->isEmpty())
    return false;
  if(d->tags[2] && !d->tags[2]->isEmpty())
    return false;

  return true;
}


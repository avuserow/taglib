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

#include <tfile.h>
#include <tdebug.h>

#include "id3v2tag.h"
#include "id3v2header.h"
#include "id3v2extendedheader.h"
#include "id3v2footer.h"
#include "id3v2synchdata.h"

#include "id3v1genres.h"

#include "frames/textidentificationframe.h"
#include "frames/commentsframe.h"
#include "frames/unsynchronizedlyricsframe.h"

using namespace TagLib;
using namespace ID3v2;

class ID3v2::Tag::TagPrivate
{
public:
  TagPrivate() : file(0), tagOffset(-1), extendedHeader(0), footer(0), paddingSize(0)
  {
    frameList.setAutoDelete(true);
  }
  ~TagPrivate()
  {
    delete extendedHeader;
    delete footer;
  }

  File *file;
  long tagOffset;
  const FrameFactory *factory;

  Header header;
  ExtendedHeader *extendedHeader;
  Footer *footer;

  int paddingSize;

  FrameListMap frameListMap;
  FrameList frameList;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

ID3v2::Tag::Tag() : TagLib::Tag()
{
  d = new TagPrivate;
  d->factory = FrameFactory::instance();
}

ID3v2::Tag::Tag(File *file, long tagOffset, const FrameFactory *factory) :
  TagLib::Tag()
{
  d = new TagPrivate;

  d->file = file;
  d->tagOffset = tagOffset;
  d->factory = factory;

  read();
}

ID3v2::Tag::~Tag()
{
  delete d;
}

#pragma mark -

String ID3v2::Tag::title() const
{
  if(!d->frameListMap["TIT2"].isEmpty())
    return d->frameListMap["TIT2"].front()->toString();
  return String::null;
}

String ID3v2::Tag::artist() const
{
  if(!d->frameListMap["TPE1"].isEmpty())
    return d->frameListMap["TPE1"].front()->toString();
  return String::null;
}

String ID3v2::Tag::album() const
{
  if(!d->frameListMap["TALB"].isEmpty())
    return d->frameListMap["TALB"].front()->toString();
  return String::null;
}

String ID3v2::Tag::comment() const
{
	const FrameList &comments = d->frameListMap["COMM"];
	
	if(comments.isEmpty())
		return String::null;
	
	
	//  for(FrameList::ConstIterator it = comments.begin(); it != comments.end(); ++it)
	//  {
	//    CommentsFrame *frame = dynamic_cast<CommentsFrame *>(*it);
	//
	//    if(frame && frame->description().isEmpty())
	//      return (*it)->toString();
	//  }
	//allways use the last frame  (like iTunes does)
	//but ignore the iTunes special flags
	
	FrameList::ConstIterator it = comments.end();
	--it;
	while (it != comments.end()){
		CommentsFrame *frame = dynamic_cast<CommentsFrame *>(*it);
		if (!(frame->description() == String("iTunNORM"))
			&& !(frame->description() == String("iTunSMPB"))
			&& !(frame->description() == String("iTunPGAP"))
			&& !(frame->description() == String("iTunes_CDDB_1"))
			&& !(frame->description() == String("iTunes_CDDB_IDs"))
			&& !(frame->description() == String("iTunes_CDDB_TrackNumber"))) {
			return (*it)->toString();
		}
		--it; //revers enumerate
	}
	return String::null;
}

String ID3v2::Tag::genre() const
{
  // TODO: In the next major version (TagLib 2.0) a list of multiple genres
  // should be separated by " / " instead of " ".  For the moment to keep
  // the behavior the same as released versions it is being left with " ".

  if(d->frameListMap["TCON"].isEmpty() ||
     !dynamic_cast<TextIdentificationFrame *>(d->frameListMap["TCON"].front()))
  {
    return String::null;
  }

  // ID3v2.4 lists genres as the fields in its frames field list.  If the field
  // is simply a number it can be assumed that it is an ID3v1 genre number.
  // Here was assume that if an ID3v1 string is present that it should be
  // appended to the genre string.  Multiple fields will be appended as the
  // string is built.

  TextIdentificationFrame *f = static_cast<TextIdentificationFrame *>(
    d->frameListMap["TCON"].front());

  StringList fields = f->fieldList();

  StringList genres;

  for(StringList::Iterator it = fields.begin(); it != fields.end(); ++it) {

    if((*it).isEmpty())
      continue;

    bool ok;
    int number = (*it).toInt(&ok);
    if(ok && number >= 0 && number <= 255) {
      *it = ID3v1::genre(number);
    }

    if(std::find(genres.begin(), genres.end(), *it) == genres.end())
      genres.append(*it);
  }

  return genres.toString();
}

String ID3v2::Tag::lyrics() const
{
	if (!d->frameListMap["USLT"].isEmpty()) {
		return d->frameListMap["USLT"].front()->toString();
	}
	return String::null;
}

TagLib::uint ID3v2::Tag::year() const
{
  if(!d->frameListMap["TDRC"].isEmpty())
    return d->frameListMap["TDRC"].front()->toString().substr(0, 4).toInt();
  return 0;
}

TagLib::uint 
ID3v2::Tag::track() const
{
  if(!d->frameListMap["TRCK"].isEmpty()) {
	String trackNrAndTotalTracks(d->frameListMap["TRCK"].front()->toString());
	
	int seperator = trackNrAndTotalTracks.rfind(String("/"));
	if (seperator == -1) {
		return  d->frameListMap["TRCK"].front()->toString().toInt();
	} else {
		String trackNr = trackNrAndTotalTracks.substr(0,seperator);
		return trackNr.toInt();
	}
  }
  return 0;
}

String
ID3v2::Tag::albumArtist() const
{
	if(!d->frameListMap["TPE2"].isEmpty())
		return d->frameListMap["TPE2"].front()->toString();
	return String::null;
}

String 
ID3v2::Tag::grouping() const
{
	if(!d->frameListMap["TIT1"].isEmpty())
		return d->frameListMap["TIT1"].front()->toString();
	return String::null;
}

String 
ID3v2::Tag::composer() const
{
	if(!d->frameListMap["TCOM"].isEmpty())
		return d->frameListMap["TCOM"].front()->toString();
	return String::null;
}

TagLib::uint 
ID3v2::Tag::totalTracks() const
{
	if(!d->frameListMap["TRCK"].isEmpty()) {
		String trackNrAndTotalTracks(d->frameListMap["TRCK"].front()->toString());
		int seperator = trackNrAndTotalTracks.rfind(String("/"));
		if (seperator == -1) {
			return  0;
		} else {
			String tracks = trackNrAndTotalTracks.substr(seperator+1);
			return tracks.toInt();
		}
	}
	return 0;
}

TagLib::uint 
ID3v2::Tag::cdNr() const
{
	if(!d->frameListMap["TPOS"].isEmpty()) {
		String cdNrAndTotalCDs(d->frameListMap["TPOS"].front()->toString());
		
		int seperator = cdNrAndTotalCDs.rfind(String("/"));
		if (seperator == -1) {
			return  d->frameListMap["TPOS"].front()->toString().toInt();
		} else {
			String cdNr = cdNrAndTotalCDs.substr(0,seperator);
			return cdNr.toInt();
		}
	}
	return 0;
}

TagLib::uint 
ID3v2::Tag::totalCDs() const
{
	if(!d->frameListMap["TPOS"].isEmpty()) {
		String cdNrAndTotalCDs(d->frameListMap["TPOS"].front()->toString());
		
		int seperator = cdNrAndTotalCDs.rfind(String("/"));
		if (seperator == -1) {
			return  0;
		} else {
			String totalCDs = cdNrAndTotalCDs.substr(seperator+1);
			return totalCDs.toInt();
		}
	}
	return 0;
}

TagLib::uint 
ID3v2::Tag::bpm() const
{
	if(!d->frameListMap["TBPM"].isEmpty())
		return d->frameListMap["TBPM"].front()->toString().toInt();
	return 0;
}


bool 
ID3v2::Tag::compilation() const
{
	if(!d->frameListMap["TCMP"].isEmpty())
		return d->frameListMap["TCMP"].front()->toString().toInt();
	return 0;
}

bool 
ID3v2::Tag::podcast() const
{
	if(!d->frameListMap["PCST"].isEmpty())
		return 1;
	return 0;
}

bool 
ID3v2::Tag::itunesu() const
{
	if(!d->frameListMap["ITNU"].isEmpty())
		return 1;
	return 0;
}


#pragma mark -

void ID3v2::Tag::setTitle(const String &s)
{
  setTextFrame("TIT2", s);
}

void ID3v2::Tag::setArtist(const String &s)
{
  setTextFrame("TPE1", s);
}

void ID3v2::Tag::setAlbum(const String &s)
{
  setTextFrame("TALB", s);
}

void ID3v2::Tag::setComment(const String &s)
{
	if(s.isEmpty()) {
		removeFrames("COMM");
		return;
	}
	//finda non iTunes specific frame to put the comment
	for(FrameList::ConstIterator it = d->frameListMap["COMM"].begin(); it != d->frameListMap["COMM"].end(); ++it) {
		CommentsFrame *frame = dynamic_cast<CommentsFrame *>(*it);
		if (!(frame->description() == String("iTunNORM"))
			&& !(frame->description() == String("iTunSMPB"))
			&& !(frame->description() == String("iTunPGAP"))
			&& !(frame->description() == String("iTunes_CDDB_1"))
			&& !(frame->description() == String("iTunes_CDDB_IDs"))
			&& !(frame->description() == String("iTunes_CDDB_TrackNumber"))) {
			frame->setText(s);
			return;
		}
	}
	// if no not iTunes specific frame was found, add a new one
	CommentsFrame *f = new CommentsFrame(d->factory->defaultTextEncoding());
	addFrame(f);
	f->setText(s);
}

int ID3v2::Tag::setLyrics(const String &s)
{  
  if(s.isEmpty()) {
    removeFrames("USLT");
    return 0;
  }

  if(!d->frameListMap["USLT"].isEmpty()) {
    d->frameListMap["USLT"].front()->setText(s);
  }
  else {
    UnsynchronizedLyricsFrame *f = new UnsynchronizedLyricsFrame(d->factory->defaultTextEncoding());
    addFrame(f);
    f->setText(s);
  }
  return 0;
}

void ID3v2::Tag::setGenre(const String &s)
{
  if(s.isEmpty()) {
    removeFrames("TCON");
    return;
  }

  // iTunes can't handle correctly encoded ID3v2.4 numerical genres.  Just use
  // strings until iTunes sucks less.

#ifdef NO_ITUNES_HACKS

  int index = ID3v1::genreIndex(s);

  if(index != 255)
    setTextFrame("TCON", String::number(index));
  else
    setTextFrame("TCON", s);

#else

  setTextFrame("TCON", s);

#endif
}

void ID3v2::Tag::setYear(TagLib::uint i)
{
  if(i <= 0) {
    removeFrames("TDRC");
    return;
  }
  setTextFrame("TDRC", String::number(i));
}

void ID3v2::Tag::setTrack(TagLib::uint i)
{
  if(i <= 0 && totalTracks() <= 0) {
    removeFrames("TRCK");
    return;
  }
	setTextFrame("TRCK", String::number(i) += String('/') += String::number(totalTracks()));
}

int
ID3v2::Tag::setAlbumArtist(const String &s)
{
	setTextFrame("TPE2", s);
	return 0;
}
int 
ID3v2::Tag::setGrouping(const String &s)
{
	setTextFrame("TIT1", s);
	return 0;
}

int 
ID3v2::Tag::setComposer(const String &s)
{
	setTextFrame("TCOM", s);
	return 0;
}

int 
ID3v2::Tag::setTotalTracks(TagLib::uint i)
{
	if(i <= 0 && track() <= 0) {
		removeFrames("TRCK");
		return 0;
	}
	setTextFrame("TRCK", String::number(track()) += String('/') += String::number(i));
	return 0;
}

int 
ID3v2::Tag::setCDNr(TagLib::uint i){
	if(i <= 0 && totalCDs() <= 0) {
		removeFrames("TPOS");
		return 0;
	}
	setTextFrame("TPOS", String::number(i) += String('/') += String::number(totalCDs()));
	return 0;
}

int 
ID3v2::Tag::setTotalCDs(TagLib::uint i)
{
	if(i <= 0 && cdNr() <= 0) {
		removeFrames("TPOS");
		return 0;
	}
	setTextFrame("TPOS", String::number(cdNr()) += String('/') += String::number(i));
	return 0;
}

int 
ID3v2::Tag::setBPM(TagLib::uint i)
{
	if(i <= 0) {
		removeFrames("TBPM");
		return 0;
	}
	setTextFrame("TBPM", String::number(i));
	return 0;
}


int
ID3v2::Tag::setCompilation(bool compilation)
{
	if (!compilation) {
		removeFrames("TCMP");
		return 0;
	}
	setTextFrame("TCMP", String::number(compilation));
	return 0;
}


#pragma mark -

bool ID3v2::Tag::isEmpty() const
{
  return d->frameList.isEmpty();
}

Header *ID3v2::Tag::header() const
{
  return &(d->header);
}

ExtendedHeader *ID3v2::Tag::extendedHeader() const
{
  return d->extendedHeader;
}

Footer *ID3v2::Tag::footer() const
{
  return d->footer;
}

const FrameListMap &ID3v2::Tag::frameListMap() const
{
  return d->frameListMap;
}

const FrameList &ID3v2::Tag::frameList() const
{
  return d->frameList;
}

const FrameList &ID3v2::Tag::frameList(const ByteVector &frameID) const
{
  return d->frameListMap[frameID];
}

void ID3v2::Tag::addFrame(Frame *frame)
{
  d->frameList.append(frame);
  d->frameListMap[frame->frameID()].append(frame);
}

void ID3v2::Tag::removeFrame(Frame *frame, bool del)
{
  // remove the frame from the frame list
  FrameList::Iterator it = d->frameList.find(frame);
  d->frameList.erase(it);

  // ...and from the frame list map
  it = d->frameListMap[frame->frameID()].find(frame);
  d->frameListMap[frame->frameID()].erase(it);

  // ...and delete as desired
  if(del)
    delete frame;
}

void ID3v2::Tag::removeFrames(const ByteVector &id)
{
    FrameList l = d->frameListMap[id];
    for(FrameList::Iterator it = l.begin(); it != l.end(); ++it)
      removeFrame(*it, true);
}

ByteVector ID3v2::Tag::render() const
{
  // We need to render the "tag data" first so that we have to correct size to
  // render in the tag's header.  The "tag data" -- everything that is included
  // in ID3v2::Header::tagSize() -- includes the extended header, frames and
  // padding, but does not include the tag's header or footer.

  ByteVector tagData;

  // TODO: Render the extended header.

  // Loop through the frames rendering them and adding them to the tagData.

  for(FrameList::Iterator it = d->frameList.begin(); it != d->frameList.end(); it++) {
    if((*it)->header()->frameID().size() != 4) {
      debug("A frame of unsupported or unknown type \'"
          + String((*it)->header()->frameID()) + "\' has been discarded");
      continue;
    }
    if(!(*it)->header()->tagAlterPreservation())
      tagData.append((*it)->render());
  }

  // Compute the amount of padding, and append that to tagData.

  TagLib::uint paddingSize = 0;
  TagLib::uint originalSize = d->header.tagSize();

  if(tagData.size() < originalSize)
    paddingSize = originalSize - tagData.size();
  else
    paddingSize = 1024;

  tagData.append(ByteVector(paddingSize, char(0)));

  // Set the tag size.
  d->header.setTagSize(tagData.size());

  // TODO: This should eventually include d->footer->render().
  return d->header.render() + tagData;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void ID3v2::Tag::read()
{
  if(d->file && d->file->isOpen()) {

    d->file->seek(d->tagOffset);
    d->header.setData(d->file->readBlock(Header::size()));

    // if the tag size is 0, then this is an invalid tag (tags must contain at
    // least one frame)

    if(d->header.tagSize() == 0)
      return;

    parse(d->file->readBlock(d->header.tagSize()));
  }
}

void ID3v2::Tag::parse(const ByteVector &origData)
{
  ByteVector data = origData;

  if(d->header.unsynchronisation() && d->header.majorVersion() <= 3)
    data = SynchData::decode(data);

  TagLib::uint frameDataPosition = 0;
  TagLib::uint frameDataLength = data.size();

  // check for extended header

  if(d->header.extendedHeader()) {
    if(!d->extendedHeader)
      d->extendedHeader = new ExtendedHeader;
    d->extendedHeader->setData(data);
    if(d->extendedHeader->size() <= data.size()) {
      frameDataPosition += d->extendedHeader->size();
      frameDataLength -= d->extendedHeader->size();
    }
  }

  // check for footer -- we don't actually need to parse it, as it *must*
  // contain the same data as the header, but we do need to account for its
  // size.

  if(d->header.footerPresent() && Footer::size() <= frameDataLength)
    frameDataLength -= Footer::size();

  // parse frames

  // Make sure that there is at least enough room in the remaining frame data for
  // a frame header.

  while(frameDataPosition < frameDataLength - Frame::headerSize(d->header.majorVersion())) {

    // If the next data is position is 0, assume that we've hit the padding
    // portion of the frame data.

    if(data.at(frameDataPosition) == 0) {
      if(d->header.footerPresent())
        debug("Padding *and* a footer found.  This is not allowed by the spec.");

      d->paddingSize = frameDataLength - frameDataPosition;
      return;
    }

    Frame *frame = d->factory->createFrame(data.mid(frameDataPosition),
                                           &d->header);

    if(!frame)
      return;

    // Checks to make sure that frame parsed correctly.

    if(frame->size() <= 0) {
      delete frame;
      return;
    }

    frameDataPosition += frame->size() + Frame::headerSize(d->header.majorVersion());
    addFrame(frame);
  }
}

void ID3v2::Tag::setTextFrame(const ByteVector &id, const String &value)
{
  if(value.isEmpty()) {
    removeFrames(id);
    return;
  }

  if(!d->frameListMap[id].isEmpty())
    d->frameListMap[id].front()->setText(value);
  else {
    const String::Type encoding = d->factory->defaultTextEncoding();
    TextIdentificationFrame *f = new TextIdentificationFrame(id, encoding);
    addFrame(f);
    f->setText(value);
  }
}

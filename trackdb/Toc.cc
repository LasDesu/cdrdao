/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998, 1999 Andreas Mueller <mueller@daneb.ping.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * $Log: Toc.cc,v $
 * Revision 1.2  2000/06/10 14:44:47  andreasm
 * Tracks that are shorter than 4 seconds do not lead to a fatal error anymore.
 * The user has the opportunity to record such tracks now.
 *
 * Revision 1.1.1.1  2000/02/05 01:32:54  llanero
 * Uploaded cdrdao 1.1.3 with pre10 patch applied.
 *
 * Revision 1.11  1999/04/05 11:03:01  mueller
 * Added CD-TEXT support.
 *
 * Revision 1.10  1999/04/02 20:36:21  mueller
 * Created implementation class that contains all mutual member data.
 *
 * Revision 1.9  1999/03/27 19:52:26  mueller
 * Added data track support.
 *
 * Revision 1.8  1999/01/10 15:10:13  mueller
 * Added functions 'appendTrack()' and 'appendAudioData()'.
 *
 * Revision 1.7  1998/11/15 12:19:13  mueller
 * Added several functions for manipulating track/index marks.
 *
 * Revision 1.6  1998/10/24 14:28:16  mueller
 * Fixed bug in 'readSamples()'.
 *
 * Revision 1.5  1998/10/03 14:32:31  mueller
 * Applied patch from Bjoern Fischer <bfischer@Techfak.Uni-Bielefeld.DE>.
 *
 * Revision 1.4  1998/09/22 19:17:19  mueller
 * Added seeking to and reading of samples for GUI.
 *
 * Revision 1.3  1998/09/06 12:00:26  mueller
 * Used 'message' function for messages.
 *
 */

static char rcsid[] = "$Id: Toc.cc,v 1.2 2000/06/10 14:44:47 andreasm Exp $";

#include <config.h>

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <fstream.h>

#include "Toc.h"
#include "util.h"
#include "TrackDataList.h"
#include "CdTextItem.h"

extern Toc *parseToc(FILE *fp, const char *filename);
extern Toc *parseCue(FILE *fp, const char *filename);

Toc::Toc() : length_(0)
{
  tocType_ = CD_DA;
  nofTracks_ = 0;
  tracks_ = lastTrack_ = NULL;

  catalogValid_ = 0;
}

Toc::~Toc()
{
  TrackEntry *run = tracks_;
  TrackEntry *next;

  while (run != NULL) {
    next = run->next;
    delete run->track;
    delete run;
    run = next;
  }
}

      
// appends track
// return: 0: OK
//         1: first track contains pregap
int Toc::append(const Track *t)
{
  if (tracks_ == NULL) {
    tracks_ = lastTrack_ = new TrackEntry;
  }
  else {
    lastTrack_->next = new TrackEntry;
    lastTrack_->next->pred = lastTrack_;
    lastTrack_ = lastTrack_->next;
  }

  lastTrack_->track = new Track(*t);
  nofTracks_ += 1;

  update();
  return 0;
}

void Toc::update()
{
  TrackEntry *run;
  long length = 0; // length of disc in blocks
  long tlength; // length of single track in blocks
  int tnum;

  for (run = tracks_, tnum = 1; run != NULL; run = run->next, tnum++) {
    tlength = run->track->length().lba();

    run->absStart = Msf(length);
    run->start = Msf(length + run->track->start().lba());
    run->end = Msf(length + tlength);
    run->trackNr = tnum;

    length += tlength;
  }

  length_ = Msf(length);
}



Toc *Toc::read(const char *filename)
{
  FILE *fp;
  Toc *ret;
  const char *p;

  if ((fp = fopen(filename, "r")) == NULL) {
    message(-2, "Cannot open toc file '%s' for reading: %s",
	    filename, strerror(errno));
    return NULL;
  }

  if ((p = strrchr(filename, '.')) != NULL && strcasecmp(p, ".cue") == 0)
    ret = parseCue(fp, filename);
  else
    ret = parseToc(fp, filename);

  fclose(fp);

  /*
  if (ret != NULL)
    ret->print(cout);
    */

  return ret;
}

// Writes toc to file with given name.
// Return: 0: OK
//         1: error occured

int Toc::write(const char *filename) const
{
  assert(filename != NULL);
  assert(*filename != 0);

  ofstream out(filename);

  if (!out) {
    message(-2, "Cannot open file \"%s\" for writing: %s", filename,
	    strerror(errno));
    return 1;
  }

  print(out);

  return 0;
}

int Toc::check() const
{
  TrackEntry *t;
  int trackNr;
  int ret = 0;

  for (t = tracks_, trackNr = 1; t != NULL; t = t->next, trackNr++) {
    ret |= t->track->check(trackNr);
  }

  return ret;
}

// Sets catalog number. 's' must be a string of 13 digits.
// return: 0: OK
//         1: illegal catalog string
int Toc::catalog(const char *s)
{
  int i;

  if (s == NULL) {
    catalogValid_ = 0;
    return 0;
  }

  if (strlen(s) != 13) {
    return 1;
  }

  for (i = 0; i < 13; i++) {
    if (!isdigit(s[i]))
      return 1;
  }

  for (i = 0; i < 13; i++)
    catalog_[i] = s[i] - '0';

  catalogValid_ = 1;

  return 0;
}


const char *Toc::catalog() const
{
  static char buf[14];
  int i;

  if (!catalogValid_)
    return NULL;

  for (i = 0; i < 13; i++)
    buf[i] = catalog_[i] + '0';

  buf[13] = 0;

  return buf;
}

// writes contents in TOC file syntax
void Toc::print(ostream &out) const
{
  int i;
  TrackEntry *t;

  out << tocType2String(tocType()) << "\n\n";

  if (catalogValid()) {
    out << "CATALOG \"";
    for (i = 0; i < 13; i++) {
      out << (char)(catalog(i) + '0');
    }
    out << "\"" << endl;
  }

  cdtext_.print(0, out);

  for (t = tracks_, i = 1; t != NULL; t = t->next, i++) {
    out << "\n// Track " << i << "\n";
    t->track->print(out);
    out << endl;
  }
}

// find track entry that contains given sample number
// return: found track entry or 'NULL' if sample is out of range
Toc::TrackEntry *Toc::findTrack(unsigned long sample) const
{
  TrackEntry *run;

  for (run = tracks_; run != NULL; run = run->next) {
    if (sample < run->end.samples())
      return run;
  }

  return NULL;
}

// find track with given number
// return: found track entry or 'NULL' if 'trackNr' is out of range
Toc::TrackEntry *Toc::findTrackByNumber(int trackNr) const
{
  TrackEntry *run;

  for (run = tracks_; run != NULL; run = run->next) {
    if (run->trackNr == trackNr)
      return run;
  }

  return NULL;
}

Track *Toc::getTrack(int trackNr)
{
  TrackEntry *ent = findTrackByNumber(trackNr);

  if (ent != NULL)
    return ent->track;
  else
    return NULL;
}

// Moves specified track/index position to given LBA if possible.
// return: 0: OK
//         1: Cannot move pre-gap of first track
//         2: specified track/index position not found in toc
//         3: 'lba' is an illegal position for track/index mark
//         4: resulting track length would be short than 4 seconds
//         5: cannot cross track/index boundaries
//         6: cannot modify data track
int Toc::moveTrackMarker(int trackNr, int indexNr, long lba)
{
  TrackEntry *act;

  if (trackNr == 1 && indexNr == 0)
    return 1;

  if ((act = findTrackByNumber(trackNr)) == NULL) {
    return 2;
  }

  if (indexNr <= 1 && act->track->type() != TrackData::AUDIO)
    return 6;

  if ((indexNr == 0 || (indexNr == 1 && act->track->start().lba() == 0)) &&
      act->pred != NULL && act->pred->track->type() != TrackData::AUDIO)
    return 6;
  

  if (indexNr == 0 && act->track->start().lba() == 0)
    return 2;

  if (indexNr > 1 && indexNr - 2 >= act->track->nofIndices())
    return 2;

  if (lba < 0 || lba >= length().lba())
    return 3;

  if ((indexNr == 1 && (act->track->start().lba() > 0 || trackNr == 1)) ||
      indexNr > 1) {
    // change track/index position within track
    if (indexNr == 1) {
      if (lba > act->end.lba() - 4 * 75)
	return 4;

      if (lba <= act->absStart.lba() && trackNr > 1)
	return 3;
    }
    else {
      if (lba - act->absStart.lba() <= 0)
	return 3;
    }

    switch (act->track->moveIndex(indexNr, lba - act->absStart.lba())) {
    case 1:
      return 4;
    case 2:
      return 5;
    }
  }
  else {
    // move track start position, we need to shift audio data from
    // on track to the other

    // 'act->pred' is always non NULL in this case because track 1 is 
    // exhaustively handled above

    TrackDataList *dataList; // audio data that is removed from one track

    if (lba < act->absStart.lba()) {
      // move to the left

      if (lba < act->pred->start.lba() + 4 * 75)
	return 4;
      
      dataList =
	act->pred->track->removeToEnd(Msf(lba - act->pred->absStart.lba()).samples());
      act->track->prependTrackData(dataList);
      delete dataList;

      // adjust start of track
      act->track->start(Msf(act->start.lba() - lba));

      if (indexNr == 1)
	act->track->moveIndex(1, 0); // remove intermediate pre-gap
    }
    else if (lba > act->absStart.lba()) {
      // move to the right
      
      if (indexNr == 1) {
	// introduce an intermediate pre-gap that adjusts the index 
	// increments
	switch (act->track->moveIndex(1, lba - act->absStart.lba())) {
	case 1:
	  return 4;
	case 2:
	  return 5;
	}

	// remove intermediate pre-gap
	act->track->start(Msf(0));
      }
      else {
	// adjust pre-gap
	if (lba >= act->start.lba())
	  return 5;

	act->track->start(Msf(act->start.lba() - lba));
      }

      dataList =
	act->track->removeFromStart(Msf(lba - act->absStart.lba()).samples());
      act->pred->track->appendTrackData(dataList);
      delete dataList;

    }
  }

  update();
  checkConsistency();

  return 0;
}

void Toc::remove(TrackEntry *ent)
{
  if (ent->pred != NULL)
    ent->pred->next = ent->next;
  else
    tracks_ = ent->next;

  if (ent->next != NULL)
    ent->next->pred = ent->pred;
  else
    lastTrack_ = ent->pred;

  ent->pred = ent->next = NULL;
  ent->track = NULL;
  delete ent;
}

// Removes specified track marker.
// return: 0: OK
//         1: cannot remove first track
//         2: specified track/index position not found in toc
//         3: cannot modify a data track
int Toc::removeTrackMarker(int trackNr, int indexNr)
{
  TrackEntry *act;

  if (trackNr == 1 && indexNr == 1)
    return 1;

  if ((act = findTrackByNumber(trackNr)) == NULL) {
    return 2;
  }

  if (act->track->type() != TrackData::AUDIO && indexNr <= 1)
    return 3;

  if (act->pred != NULL && act->pred->track->type() != TrackData::AUDIO &&
      indexNr <= 1)
    return 3;

  if (trackNr == 1 && indexNr == 0) {
    // pre-gap of first track

    if (act->start.lba() == 0)
      return 2; // no pre-gap

    act->track->start(Msf(0));
  }
  else if (indexNr > 1) {
    // index increment
    if (act->track->removeIndex(indexNr - 2) != 0)
      return 2;
  }
  else if (indexNr == 0) {
    // remove pre-gap, audio data of pre-gap is appended to previous track
    unsigned long len = act->track->start().samples();

    if (len == 0)
      return 2; // track has no pre-gap

    act->track->start(Msf(0));
    TrackDataList *dataList =  act->track->removeFromStart(len);
    act->pred->track->appendTrackData(dataList);
    delete dataList;
  }
  else {
    // index == 1, remove track completely
    
    act->pred->track->appendTrackData(act->track);

    Track *store = act->track;
    remove(act);
    delete store;

    nofTracks_--;
  }

  update();
  checkConsistency();

  return 0;
}

// Adds index increment at given LBA.
// return: 0: OK
//         1: LBA out of range
//         2: cannot add index at this position
//         3: more than 98 index increments
int Toc::addIndexMarker(long lba)
{
  TrackEntry *act = findTrack(Msf(lba).samples());

  if (act == NULL)
    return 1;

  if (lba <= act->start.lba())
    return 2;

  switch (act->track->addIndex(Msf(lba - act->start.lba()))) {
  case 1:
    return 3;
  case 2:
    return 2;
  }

  return 0;
}

// Adds a track marker at given LBA.
// return: 0: OK
//         1: LBA out of range
//         2: cannot add track at this position
//         3: resulting track would be shorter than 4 seconds
//         4: previous track would be short than 4 seconds
//         5: cannot modify a data track
int Toc::addTrackMarker(long lba)
{
  TrackEntry *act = findTrack(Msf(lba).samples());

  if (act == NULL)
    return 1;

  if (act->track->type() != TrackData::AUDIO)
    return 5;

  if (lba <= act->start.lba())
    return 2;
  
  if (lba - act->start.lba() < 4 * 75)
    return 4;

  if (act->end.lba() - lba < 4 * 75)
    return 3;


  TrackDataList *dataList =
    act->track->removeToEnd(Msf(lba - act->absStart.lba()).samples());

  Track *t = new Track(act->track->type());
  t->appendTrackData(dataList);
  delete dataList;

  TrackEntry *ent = new TrackEntry;
  ent->track = t;

  ent->next = act->next;
  if (ent->next != NULL)
    ent->next->pred = ent;

  ent->pred = act;
  act->next = ent;
  
  if (act == lastTrack_)
    lastTrack_ = ent;

  nofTracks_++;

  update();
  checkConsistency();

  return 0;
}


// Adds pre-gap add given LBA.
// return: 0: OK
//         1: LBA out of range
//         2: cannot add pre-gap at this point
//         3: actual track would be shorter than 4 seconds
//         4: cannot modify a data track
int Toc::addPregap(long lba)
{
  TrackEntry *act = findTrack(Msf(lba).samples());

  if (act == NULL)
    return 1;

  if (act->track->type() != TrackData::AUDIO) 
    return 4;
  
  if (act->next == NULL)
    return 2; // no next track where we could add pre-gap

  if (act->next->track->type() != TrackData::AUDIO) 
    return 4;
  
  if (lba <= act->start.lba())
    return 2;

  if (act->next->track->start().lba() != 0)
    return 2; // track has already a pre-gap

  if (lba - act->start.lba() < 4 * 75)
    return 3;

  TrackDataList *dataList =
    act->track->removeToEnd(Msf(lba - act->absStart.lba()).samples());
  act->next->track->prependTrackData(dataList);
  delete dataList;

  act->next->track->start(Msf(act->next->start.lba() - lba));

  update();
  checkConsistency();

  return 0;
}


void Toc::checkConsistency()
{
  TrackEntry *run, *last = NULL;
  long cnt = 0;

  for (run = tracks_; run != NULL; last = run, run = run->next) {
    cnt++;
    if (run->pred != last) 
      message(-3, "Toc::checkConsistency: wrong pred pointer.");

    run->track->checkConsistency();
  }

  if (last != lastTrack_)
    message(-3, "Toc::checkConsistency: wrong last pointer.");

  if (cnt != nofTracks_)
    message(-3, "Toc::checkConsistency: wrong sub track counter.");
}


// Appends a track with given audio data. 'start' is filled with
// first LBA of new track, 'end' is filled with last LBA + 1 of new track.
void Toc::appendTrack(const TrackDataList *list, long *start, long *end)
{
  Track t(TrackData::AUDIO);
  const TrackData *run;

  for (run = list->first(); run != NULL; run = list->next())
    t.append(SubTrack(SubTrack::DATA, *run));

  // ensure that track lasts at least 4 seconds
  unsigned long minTime = 4 * 75 * SAMPLES_PER_BLOCK; // 4 seconds
  unsigned long len = list->length();

  if (len < minTime)
    t.append(SubTrack(SubTrack::DATA,
		      TrackData(TrackData::AUDIO, minTime - len)));

  *start = length().lba();

  append(&t);
  checkConsistency();

  *end = length().lba();
}

// Appends given audio data to last track. If no track exists 'appendTrack'
// will be called. 'start' and 'end' is filled with position of modified
// region.
// Return: 0: OK
//         1: cannot modify a data track
int Toc::appendTrackData(const TrackDataList *list, long *start, long *end)
{
  const TrackData *run;

  if (lastTrack_ == NULL) {
    appendTrack(list, start, end);
    return 0;
  }

  if (lastTrack_->track->type() != TrackData::AUDIO)
    return 1;

  *start = length().lba();

  for (run = list->first(); run != NULL; run = list->next())
    lastTrack_->track->append(SubTrack(SubTrack::DATA, *run));
  
  update();
  checkConsistency();

  *end = length().lba();

  return 0;
}

// Removes specified range of samples from a single track.
// Return: 0: OK
//         1: samples range covers more than one track
//         2: cannot modify a data track
//         3: illegal 'start' position
int Toc::removeTrackData(unsigned long start, unsigned long end,
			 TrackDataList **list)
{
  TrackEntry *tent = findTrack(start);

  if (tent == NULL)
    return 3;

  if (tent->track->type() != TrackData::AUDIO)
    return 2;

  if (tent != findTrack(end))
    return 1;

  *list = tent->track->removeTrackData(start - tent->absStart.samples(),
				       end - tent->absStart.samples());

  update();
  checkConsistency();
  
  return 0;
}

// Inserts given track data at specified postion.
// Return: 0: OK
//         1: cannot modify a data track

int Toc::insertTrackData(unsigned long pos, const TrackDataList *list)
{
  TrackEntry *tent = findTrack(pos);

  if (tent != NULL && tent->track->type() != TrackData::AUDIO)
    return 1;
  
  if (tent != NULL) {
    tent->track->insertTrackData(pos - tent->absStart.samples(), list);

    update();
    checkConsistency();

    return 0;
  }
  else {
    long start, end;

    return appendTrackData(list, &start, &end);
  }

}


// Returns mode that should be used for lead-in. The mode of first track's
// first sub-track is used.
TrackData::Mode Toc::leadInMode() const
{
  const SubTrack *t;

  if (tracks_ == NULL || (t = tracks_->track->firstSubTrack()) == NULL) {
    // no track or track data available - return AUDIO in this case
    return TrackData::AUDIO;
  }

  return t->mode();
}

// Returns mode that should be used for lead-out. The mode of last track's
// last sub-track is used
TrackData::Mode Toc::leadOutMode() const
{
  const SubTrack *t;

  if (lastTrack_ == NULL || (t = lastTrack_->track->lastSubTrack()) == NULL) {
    // no track or track data available - return AUDIO in this case
    return TrackData::AUDIO;
  }

  return t->mode();
}

const char *Toc::tocType2String(TocType t)
{
  const char *ret = NULL;
  switch (t) {
  case CD_DA:
    ret = "CD_DA";
    break;
  case CD_ROM:
    ret = "CD_ROM";
    break;
  case CD_I:
    ret = "CD_I";
    break;
  case CD_ROM_XA:
    ret = "CD_ROM_XA";
    break;
  }

  return ret;
}

void Toc::addCdTextItem(int trackNr, CdTextItem *item)
{
  assert(trackNr >= 0 && trackNr <= 99);

  if (trackNr == 0) {
    cdtext_.add(item);
  }
  else {
    TrackEntry *track = findTrackByNumber(trackNr);

    if (track == NULL) {
      message(-3, "addCdTextItem: Track %d is not available.", trackNr);
      return;
    }

    track->track->addCdTextItem(item);
  }
}

void Toc::removeCdTextItem(int trackNr, CdTextItem::PackType type, int blockNr)
{
  assert(trackNr >= 0 && trackNr <= 99);

  if (trackNr == 0) {
    cdtext_.remove(type, blockNr);
  }
  else {
    TrackEntry *track = findTrackByNumber(trackNr);

    if (track == NULL) {
      message(-3, "addCdTextItem: Track %d is not available.", trackNr);
      return;
    }

    track->track->removeCdTextItem(type, blockNr);
  }
}

int Toc::existCdTextBlock(int blockNr) const
{
  if (cdtext_.existBlock(blockNr))
    return 1;

  TrackEntry *run;

  for (run = tracks_; run != NULL; run = run->next) {
    if (run->track->existCdTextBlock(blockNr))
      return 1;
  }

  return 0;
}

const CdTextItem *Toc::getCdTextItem(int trackNr, int blockNr,
				     CdTextItem::PackType type) const
{
  if (trackNr == 0) {
    return cdtext_.getPack(blockNr, type);
  }

  TrackEntry *track = findTrackByNumber(trackNr);

  if (track == NULL)
    return NULL;

  return track->track->getCdTextItem(blockNr, type);
}

void Toc::cdTextLanguage(int blockNr, int lang)
{
  cdtext_.language(blockNr, lang);

}


int Toc::cdTextLanguage(int blockNr) const
{
  return cdtext_.language(blockNr);
}

// Check the consistency of the CD-TEXT data.
// Return: 0: OK
//         1: at least one warning occured
//         2: at least one error occured
int Toc::checkCdTextData() const
{
  TrackEntry *trun;
  int err = 0;
  int l;
  int last;
  int titleCnt, performerCnt, songwriterCnt, composerCnt, arrangerCnt;
  int messageCnt, isrcCnt, genreCnt;
  int languageCnt = 0;

  genreCnt = 0;

  // Check if language numbers are continuously used starting at 0
  for (l = 0, last = -1; l <= 7; l++) {
    if (cdTextLanguage(l) >= 0) {
      languageCnt++;

      if (cdtext_.getPack(l, CdTextItem::CDTEXT_GENRE) != NULL)
	genreCnt++;

      if (l - 1 != last) {
	if (last == -1)
	  message(-2, "CD-TEXT: Language number %d: Language numbers must start at 0.", l);
	else
	  message(-2, "CD-TEXT: Language number %d: Language numbers are not continuously used.", l);

	if (err < 2)
	  err = 2;
      }
      
      last = l;
    }
  }

  if (genreCnt > 0 && genreCnt != languageCnt) {
    message(-1, "CD-TEXT: %s field not defined for all languages.",
	    CdTextItem::packType2String(1, CdTextItem::CDTEXT_GENRE));
    if (err < 1)
      err = 1;
  }


  for (l = 0, last = -1; l <= 7; l++) {
    if (cdTextLanguage(l) < 0)
      continue;

    titleCnt = (cdtext_.getPack(l, CdTextItem::CDTEXT_TITLE) != NULL) ? 1 : 0;
    performerCnt = (cdtext_.getPack(l, CdTextItem::CDTEXT_PERFORMER) != NULL) ? 1 : 0;
    songwriterCnt = (cdtext_.getPack(l, CdTextItem::CDTEXT_SONGWRITER) != NULL) ? 1 : 0;
    composerCnt = (cdtext_.getPack(l, CdTextItem::CDTEXT_COMPOSER) != NULL) ? 1 : 0;
    arrangerCnt = (cdtext_.getPack(l, CdTextItem::CDTEXT_ARRANGER) != NULL) ? 1 : 0;
    messageCnt = (cdtext_.getPack(l, CdTextItem::CDTEXT_MESSAGE) != NULL) ? 1 : 0;
    isrcCnt = 0;
    
    for (trun = tracks_; trun != NULL; trun = trun->next) {
      if (trun->track->getCdTextItem(l, CdTextItem::CDTEXT_TITLE) != NULL)
	titleCnt++;

      if (trun->track->getCdTextItem(l, CdTextItem::CDTEXT_PERFORMER) != NULL)
	performerCnt++;

      if (trun->track->getCdTextItem(l, CdTextItem::CDTEXT_SONGWRITER) != NULL)
	songwriterCnt++;

      if (trun->track->getCdTextItem(l, CdTextItem::CDTEXT_COMPOSER) != NULL)
	composerCnt++;

      if (trun->track->getCdTextItem(l, CdTextItem::CDTEXT_ARRANGER) != NULL)
	arrangerCnt++;

      if (trun->track->getCdTextItem(l, CdTextItem::CDTEXT_MESSAGE) != NULL)
	messageCnt++;

      if (trun->track->getCdTextItem(l, CdTextItem::CDTEXT_UPCEAN_ISRC) != NULL)
	isrcCnt++;
    }

    if (titleCnt > 0 && titleCnt != nofTracks_ + 1) {
      message(-2, "CD-TEXT: Language %d: %s field not defined for all tracks or disk.",
	      l, CdTextItem::packType2String(1, CdTextItem::CDTEXT_TITLE));
      if (err < 2)
	err = 2;
    }
    else if (titleCnt == 0) {
      message(-1, "CD-TEXT: Language %d: %s field is not defined.",
	      l, CdTextItem::packType2String(1, CdTextItem::CDTEXT_TITLE));
      if (err < 1)
	err = 1;
    }

    if (performerCnt > 0 && performerCnt != nofTracks_ + 1) {
      message(-2, "CD-TEXT: Language %d: %s field not defined for all tracks or disk.",
	      l, CdTextItem::packType2String(1, CdTextItem::CDTEXT_PERFORMER));
      if (err < 2)
	err = 2;
    }
    else if (performerCnt == 0) {
      message(-1, "CD-TEXT: Language %d: %s field is not defined.",
	      l, CdTextItem::packType2String(1, CdTextItem::CDTEXT_PERFORMER));
      if (err < 1)
	err = 1;
    }

    if (songwriterCnt > 0 && songwriterCnt != nofTracks_ + 1) {
      message(-2, "CD-TEXT: Language %d: %s field not defined for all tracks or disk.",
	      l, CdTextItem::packType2String(1, CdTextItem::CDTEXT_SONGWRITER));
      if (err < 2)
	err = 2;
    }

    if (composerCnt > 0 && composerCnt != nofTracks_ + 1) {
      message(-2, "CD-TEXT: Language %d: %s field not defined for all tracks or disk.",
	      l, CdTextItem::packType2String(1, CdTextItem::CDTEXT_COMPOSER));
      if (err < 2)
	err = 2;
    }

    if (arrangerCnt > 0 && arrangerCnt != nofTracks_ + 1) {
      message(-2, "CD-TEXT: Language %d: %s field not defined for all tracks or disk.",
	      l, CdTextItem::packType2String(1, CdTextItem::CDTEXT_ARRANGER));
      if (err < 2)
	err = 2;
    }

    if (messageCnt > 0 && messageCnt != nofTracks_ + 1) {
      message(-2, "CD-TEXT: Language %d: %s field not defined for all tracks or disk.",
	      l, CdTextItem::packType2String(1, CdTextItem::CDTEXT_MESSAGE));
      if (err < 2)
	err = 2;
    }

    if ((isrcCnt > 0 && isrcCnt != nofTracks_) ||
	(isrcCnt == 0 &&
	 cdtext_.getPack(l, CdTextItem::CDTEXT_UPCEAN_ISRC) != NULL)) {
      message(-2, "CD-TEXT: Language %d: %s field not defined for all tracks.",
	      l, CdTextItem::packType2String(1, CdTextItem::CDTEXT_UPCEAN_ISRC));
      if (err < 2)
	err = 2;
    }
  }

  return err;
}

// Class TrackIterator
TrackIterator::TrackIterator(const Toc *t)
{
  toc_ = t;
  iterator_ = NULL;
}

TrackIterator::~TrackIterator()
{
  toc_ = NULL;
  iterator_ = NULL;
}

const Track *TrackIterator::find(int trackNr, Msf &start, Msf &end)
{
  Track *t;

  iterator_ = toc_->findTrackByNumber(trackNr);

  if (iterator_ != NULL) {
    start = iterator_->start;
    end = iterator_->end;
    t = iterator_->track;
    iterator_ = iterator_->next;
    return t;
  }
   
  return NULL;
}
  
const Track *TrackIterator::find(unsigned long sample, Msf &start, Msf &end,
				 int *trackNr)
{
  Track *t;

  iterator_ = toc_->findTrack(sample);

  if (iterator_ != NULL) {
    start = iterator_->start;
    end = iterator_->end;
    *trackNr = iterator_->trackNr;
    t = iterator_->track;
    iterator_ = iterator_->next;
    return t;
  }
   
  return NULL;
}

const Track *TrackIterator::first(Msf &start, Msf &end)
{
  iterator_ = toc_->tracks_;

  return next(start, end);
}

const Track *TrackIterator::next(Msf &start, Msf &end)
{
  Track *t;

  if (iterator_ != NULL) {
    start = iterator_->start;
    end = iterator_->end;
    t = iterator_->track;
    iterator_ = iterator_->next;
    return t;
  }
  else {
    return NULL;
  }
}


// Class TocReader

TocReader::TocReader(const Toc *t) : reader(NULL)
{
  toc_ = t;
  
  readTrack_ = NULL;
  readPos_ = 0;
  readPosSample_ = 0;
  open_ = 0;
}

TocReader::~TocReader ()
{
  if (open_) {
    closeData();
  }

  toc_ = NULL;
  readTrack_ = NULL;
}

void TocReader::init(const Toc *t)
{
  if (open_) {
    closeData();
  }

  reader.init(NULL);

  toc_ = t;
  readTrack_ = NULL;
}

int TocReader::openData()
{
  int ret = 0;

  assert(open_ == 0);
  assert(toc_ != NULL);

  readTrack_ = toc_->tracks_;
  readPos_ = 0;
  readPosSample_ = 0;

  reader.init(readTrack_->track);

  if (readTrack_ != NULL) {
    ret = reader.openData();
  }

  open_ = 1;

  return ret;
}

void TocReader::closeData()
{
  if (open_ != 0) {
    reader.closeData();

    readTrack_ = NULL;
    readPos_ = 0;
    open_ = 0;
    readPosSample_ = 0;
  }
}

long TocReader::readData(long lba, char *buf, long len)
{
  long n;
  long nread = 0;

  assert(open_ != 0);
  
  if (readPos_ + len > toc_->length_.lba()) {
    if ((len = toc_->length_.lba() - readPos_) <= 0) {
      return 0;
    }
  }

  do {
    n = reader.readData(0, lba, buf + (nread * AUDIO_BLOCK_LEN), len);

    if (n < 0) {
      return -1;
    }

    lba += n;

    if (n != len) {
      // skip to next track
      readTrack_ = readTrack_->next;

      assert(readTrack_ != NULL);

      reader.init(readTrack_->track);
      if (reader.openData() != 0) {
	return -1;
      }
    }
    
    nread += n;
    len -= n;
  } while (len > 0);
  
  readPos_ += nread;
 
  return nread;
}

// seeks to specified sample (absolute position)
// return: 0: OK
//         10: sample position out of range
//         return codes from 'Track::openData()'
int TocReader::seekSample(unsigned long sample)
{
  int ret;

  assert(open_ != 0);

  // find track that contains 'sample'
  Toc::TrackEntry *tr = toc_->findTrack(sample);

  if (tr == NULL)
    return 10;

  // open track if necessary
  if (tr != readTrack_) {
    readTrack_ = tr;
    reader.init(readTrack_->track);

    if ((ret = reader.openData() != 0))
      return ret;
  }

  assert(sample >= readTrack_->absStart.samples());

  unsigned long offset = sample - readTrack_->absStart.samples();

  // seek in track
  if ((ret = reader.seekSample(offset)) != 0)
    return ret;

  readPosSample_ = sample;

  return 0;
}

long TocReader::readSamples(Sample *buf, long len)
{
  long n;
  long nread = 0;

  assert(open_ != 0);
  
  if (readPosSample_ + (unsigned long)len > toc_->length_.samples()) {
    if ((len = toc_->length_.samples() - readPosSample_) <= 0)
      return 0;
  }

  do {
    n = reader.readSamples(buf + nread , len);

    if (n < 0)
      return -1;

    if (n != len) {
      // skip to next track
      readTrack_ = readTrack_->next;
      reader.init(readTrack_->track);

      assert(readTrack_ != NULL);

      if (reader.openData() != 0) {
	return -1;
      }
    }
    
    nread += n;
    len -= n;
  } while (len > 0);
  
  readPosSample_ += nread;
 
  return nread;
}

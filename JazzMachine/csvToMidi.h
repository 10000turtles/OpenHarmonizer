#include "MidiCsv/midio.h"
#include "getopt.h"
#include "midiToCsv.h"
#include "midifile.h"
#include "midio.h"
#include "types.h"
#include "unistd.h"
#include "version.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <string>
#include <vector>
#define COMMENT_DELIMITERS "#;"

#define FALSE 0
#define TRUE 1

#define ELEMENTS(array) (sizeof(array) / sizeof((array)[0]))

/*  Codes for control items.  */

typedef enum
{
  Header,
  Start_track,
  End_of_file
} controlMessages;

/*  The following table lists all possible item codes which may
    appear in a CSV-encoded MIDI file.	These should be listed
    in order of frequency of occurrence since the list is
    searched linearly.	*/

struct mitem
{
  char* name;
  int   icode;
};

#define EVENT 0
#define META 0x100
#define MARKER 0x200

#define Event(x) ((x) | EVENT)
#define Meta(x) ((x) | META)
#define Marker(x) ((x) | MARKER)

static struct mitem mitems[] = {
    {"Note_on_c", Event(NoteOn)},
    {"Note_off_c", Event(NoteOff)},
    {"Pitch_bend_c", Event(PitchBend)},
    {"Control_c", Event(ControlChange)},
    {"Program_c", Event(ProgramChange)},
    {"Poly_aftertouch_c", Event(PolyphonicKeyPressure)},
    {"Channel_aftertouch_c", Event(ChannelPressure)},

    {"System_exclusive", Event(SystemExclusive)},
    {"System_exclusive_packet", Event(SystemExclusivePacket)},

    {"Sequence_number", Meta(SequenceNumberMetaEvent)},
    {"Text_t", Meta(TextMetaEvent)},
    {"Copyright_t", Meta(CopyrightMetaEvent)},
    {"Title_t", Meta(TrackTitleMetaEvent)},
    {"Instrument_name_t", Meta(TrackInstrumentNameMetaEvent)},
    {"Lyric_t", Meta(LyricMetaEvent)},
    {"Marker_t", Meta(MarkerMetaEvent)},
    {"Cue_point_t", Meta(CuePointMetaEvent)},
    {"Channel_prefix", Meta(ChannelPrefixMetaEvent)},
    {"MIDI_port", Meta(PortMetaEvent)},
    {"End_track", Meta(EndTrackMetaEvent)},
    {"Tempo", Meta(SetTempoMetaEvent)},
    {"SMPTE_offset", Meta(SMPTEOffsetMetaEvent)},
    {"Time_signature", Meta(TimeSignatureMetaEvent)},
    {"Key_signature", Meta(KeySignatureMetaEvent)},
    {"Sequencer_specific", Meta(SequencerSpecificMetaEvent)},
    {"Unknown_meta_event", Meta(0xFF)},

    {"Header", Marker(Header)},
    {"Start_track", Marker(Start_track)},
    {"End_of_file", Marker(End_of_file)},
};

static char* progname;        /* Program name string */
static int   verbose = FALSE; /* Debug output */
static int   zerotol = FALSE; /* Any warning terminates processing */
static int   errors  = 0;     /* Errors and warnings detected */
static char* s       = NULL;  /* Dynamically expandable CSV input buffer */

/*  OUTBYTE  --  Store byte in track buffer.  */

static byte  of[10];
static byte *trackbuf = NULL, *trackbufp;
static int   trackbufl;
static char* f    = NULL;
static int   flen = 0;

#define Warn(msg) \
  { \
    errors++; \
    fprintf(stderr, "%s: Error on line %d:\n    %s\n  %s.\n", PROG, lineno, s, msg); \
    if (zerotol) \
    { \
      exit(1); \
    } \
  }

static void outbyte(const byte c)
{
  long l = trackbufp - trackbuf;

  if (l >= trackbufl)
  {
    /*trackbuf =*/realloc(trackbuf, trackbufl += 16384);
    if (trackbuf == NULL)
    {
      fprintf(stderr, "%s: Unable to allocate memory to expand track buffer to %d bytes.\n", PROG, trackbufl);
      exit(2);
    }
    trackbufp = trackbuf + l;
  }
  *trackbufp++ = c;
}

/*  OUTEVENT  --  Output event, optimising repeats.  */

static long abstime, tabstime = 0;
static void outVarLen(const vlint value);
static int  optimiseStatus = TRUE, lastStatus = -1;

static void outevent(const byte c)
{
  outVarLen(abstime - tabstime);
  tabstime = abstime;
  if (!optimiseStatus || (c != lastStatus))
  {
    outbyte(c);
    lastStatus = c;
  }
}

/*  OUTMETA  --  Output file meta-event.  */

static void outmeta(const byte c)
{
  /* Running status may not be used by file meta-events,
       and meta-events cancel any running status. */
  lastStatus = -1;
  outevent(FileMetaEvent);
  outbyte(c);
}

/*  OUTSHORT  --  Output two-byte value to track buffer.  */

static void outshort(const short v)
{
  outbyte((byte)((v >> 8) & 0xFF));
  outbyte((byte)(v & 0xFF));
}

/*  OUTBYTES  --  Output a linear array of bytes.  */

static void outbytes(const byte* s, const int n)
{
  int i;

  outVarLen(n);
  for (i = 0; i < n; i++)
  {
    outbyte(*s++);
  }
}

/*  OUTVARLEN  --  Output variable length number.  */

static void outVarLen(const vlint v)
{
  vlint value = v;
  long  buffer;

  buffer = value & 0x7F;
  while ((value >>= 7) > 0)
  {
    buffer <<= 8;
    buffer |= 0x80;
    buffer += (value & 0x7F);
  }

  while (TRUE)
  {
    outbyte((byte)(buffer & 0xFF));
    if (buffer & 0x80)
    {
      buffer >>= 8;
    }
    else
    {
      break;
    }
  }
}

/*  XFIELDS  --  Parse one or more numeric fields.  Returns FALSE if
    	    	 all fields were scanned successfully, TRUE if an
		 error occurred.  The fbias argument gives the absolute
		 field number of the first field to be parsed; it is used
		 solely to identify fields in error messages.  */

#define MAX_NFIELDS 10

static int   lineno = 0;
static long  nfld[MAX_NFIELDS];
static char* csvline;

static int xfields(const int n, const int fbias)
{
  int i;

#ifndef ndebug
  if (n > MAX_NFIELDS)
  {
    fprintf(stderr, "%s: Internal error: nfields(%d) exceeds max of %d.\n",
            PROG, n, MAX_NFIELDS);
    abort();
  }
#endif
  for (i = 0; i < n; i++)
  {
    if (!CSVscanField(&f, &flen))
    {
      errors++;
      fprintf(stderr, "%s: Error on line %d:\n    %s\n  Missing field %d.\n",
              PROG, lineno, s, fbias + i);
      return TRUE;
    }
    if (sscanf(f, "%ld", &nfld[i]) != 1)
    {
      errors++;
      fprintf(stderr, "%s: Error on line %d:\n    %s\n  Invalid field %d.\n",
              PROG, lineno, s, fbias + i);
      return TRUE;
    }
  }
  return FALSE;
}

/*  XFIELDS  --  Parse one or more numeric fields.  This is a
    	    	 wrapper for xfields which specifies the default
		 starting field of 4 used by most events.  */

static int nfields(const int n)
{
  return xfields(n, 4);
}

/*  CHECKBYTES  --  Pre-parse a sequence of arguments representing
    	    	    a byte vector.  If an error is detected, return
		    TRUE.  Otherwise, reset the CSV parser to the
		    first byte of the sequence and return FALSE.  This
		    permits code which handles arbitrary length byte
		    vectors to recover from syntax errors and ignore
		    the line prior to the irreversible step of emitting
		    the event type and length. */

static int checkBytes(const int fieldno, const int length)
{
  int i;

  for (i = 0; i < length; i++)
  {
    if (xfields(1, i + fieldno))
    {
      if (zerotol)
      {
        exit(1);
      }
      return TRUE;
    }
  }

  /* The preliminary parsing pass passed.  (Got that?)  Now restart
       the CSV parser at the start of the line and ignore fields prior
       to the first byte of the data, leaving them ready to be processed
       by the actual translation code. */

  CSVscanInit(s);
  for (i = 0; i < (fieldno - 1); i++)
  {
    CSVscanField(&f, &flen);
  }
  return FALSE;
}

/*  GETCSVLINE  --  Get next line from CSV file.  Reads into a dynamically
    	    	    allocated buffer s which is expanded as required to
		    accommodate longer lines.  All standard end of line
		    sequences are handled transparently, and the line
		    is returned with the end line sequence stripped
		    with a C string terminator (zero byte) appended.  */

static int getCSVline(FILE* fp)
{
  static int sl = 0;
  int        c, ll = -1;

  if (s == NULL)
  {
    sl = 1024;
    s  = (char*)malloc(sl);
    if (s == NULL)
    {
      fprintf(stderr, "%s: Unable to allocate %d byte CSV input line buffer.\n",
              PROG, sl);
      exit(2);
    }
  }

  while ((c = getc(fp)) >= 0)
  {
    /*  Test for end of line sequence.  We accept either a carriage return or
	    line feed as an end of line delimiter, optionally followed by no more
	    than one of the other delimiter.  */

    if (c == '\n')
    {
      c = getc(fp);
      if (c != '\r')
      {
        ungetc(c, fp);
      }
      break;
    }
    if (c == '\r')
    {
      c = getc(fp);
      if (c != '\n')
      {
        ungetc(c, fp);
      }
      break;
    }

    /*  Increment line length, expand buffer if necessary, and store character
	    in buffer.  */

    ll++;
    if (ll >= (sl - 1))
    {
      sl += 1024;
      s = (char*)realloc(s, sl);
      if (s == NULL)
      {
        fprintf(stderr, "%s: Unable to expand CSV input line buffer to %d bytes.\n",
                PROG, sl);
        exit(2);
      }
    }
    s[ll] = c;
  }

  /*	If we got a line, append a C string terminator to it.  */

  if (ll >= 0)
  {
    s[ll + 1] = 0;
  }

  return (ll >= 0);
}

/*  CLAMP  --  Constrain a numeric value to be within a specified
    	       inclusive range.  If the value is outside the range
	       an error message is issued and the value is forced to
	       the closest limit of the range.  */

static void clamp(long* value, const long minval, const long maxval, const char* fieldname)
{
  if (((*value) < minval) || ((*value) > maxval))
  {
    char errm[256];

    sprintf(errm, "%s out of range.  Value (%ld) outside limits of %ld to %ld",
            fieldname, *value, minval, maxval);
    Warn(errm);

    if ((*value) < minval)
    {
      *value = minval;
    }
    else
    {
      *value = maxval;
    }
  }
}
void csvToMidi(std::string filenameCsv, std::string filenameMidi)
{
  struct mhead  mh;
  struct mtrack mt;
  FILE *        fp = stdin, *fo = stdout;
  int           i, n, track, rtype, etype,
      headerseen = FALSE, eofseen = FALSE, intrack = 0, ntrack = 0;
  char errm[256];
  long tl;
  fp = fopen(filenameCsv, "r");
  fo = fopen(filenameMidi, "wb");
  /* Parse command line arguments. */

  //csvline = s;
  // while ((n = getopt(argc, argv, "uvxz")) != -1)
  // {
  //   switch (n)
  //   {
  //   case 'u':
  //     fprintf(stderr, "Usage: %s [ options ] [ csv_file ] [ midi_file ]\n", progname);
  //     fprintf(stderr, "       Options:\n");
  //     fprintf(stderr, "           -u            Print this message\n");
  //     fprintf(stderr, "           -v            Verbose: dump header and track information\n");
  //     fprintf(stderr, "           -x            Expand running status\n");
  //     fprintf(stderr, "           -z            Abort on any warning message\n");
  //     fprintf(stderr, "Version %s\n", VERSION);
  //     return 0;

  //   case 'v':
  //     verbose = TRUE;
  //     break;

  //   case 'x':
  //     optimiseStatus = FALSE;
  //     break;

  //   case 'z':
  //     zerotol = TRUE;
  //     break;

  //   case '?':
  //     fprintf(stderr, "%s: undefined option -%c specified.\n",
  //             PROG, n);
  //     return 2;
  //   }
  // }

  /* Open input and output files, if supplied.  Otherwise
       standard input and output are used.  A file name of
       "-" is equivalent to no specification. */


#ifdef _WIN32

  /*  If output is to standard output, set the output
    	file mode to binary.  */

  if (fo == stdout)
  {
    _setmode(_fileno(fo), _O_BINARY);
  }
#endif

  /* Allocate initial track assembly buffer.	The buffer is
       expanded as necessary. */

  trackbufp = trackbuf = (byte*)malloc(trackbufl = 16384);
  memcpy(mt.chunktype, MIDI_Track_Sentinel, sizeof mt.chunktype);

#define Nfields(n) \
  { \
    if (nfields(n)) \
    { \
      if (zerotol) \
      { \
        exit(1); \
      } \
      else \
      { \
        continue; \
      } \
    } \
  }
#define Clamp(fld, low, high, what) clamp(&nfld[fld - 4], low, high, "Field " #fld " (" what ")")

  while (getCSVline(fp))
  {
    char* p = s + strlen(s);

    lineno++;

    /* Trim any white space from the end of the record. */

    while (p >= s && isspace(*(p - 1)))
    {
      *(--p) = 0;
    }

    /* Test for and ignore blank lines and comments.  A comment
	   is any line whose first character (ignoring leading white
	   space) is one of the COMMENT_DELIMITERS).  Note that we
	   do not permit in-line comments, although the user is free
	   to supply extra CSV fields which we ignore for that
	   purpose. */

    p = s;
    while ((*p != 0) && isspace(*p))
    {
      p++;
    }
    if ((*p == 0) || (strchr(COMMENT_DELIMITERS, *p) != NULL))
    {
      continue;
    }

    CSVscanInit(s);

    /* Scan track, absolute time, and record type.	These are
	   present in all records. */

    if (CSVscanField(&f, &flen))
    {
      track = atoi(f);
    }
    else
    {
      Warn("Missing track number (field 1)");
      continue;
    }
    tl = track;
    clamp(&tl, 0, 65535, "Field 1 (Track)");
    track = (int)tl;

    if (CSVscanField(&f, &flen))
    {
      abstime = atol(f);
    }
    else
    {
      Warn("Missing absolute time (field 2)");
      continue;
    }
    clamp(&abstime, 0, 0x7FFFFFFF, "Field 2 (Time)");

    if (!CSVscanField(&f, &flen))
    {
      Warn("Missing record type (field 3)");
      continue;
    }

    /* Look up record type and dispatch to appropriate handler. */

    rtype = -1;
    for (i = 0; i < ELEMENTS(mitems); i++)
    {
      if (strcasecmp(f, mitems[i].name) == 0)
      {
        rtype = mitems[i].icode;
        break;
      }
    }

    if (rtype < 0)
    {
      sprintf(errm, "Unknown record type: \"%s\"", f);
      Warn(errm);
      continue;
    }

    etype = rtype & 0xFF;

    /* File structure pseudo-events.  These records do
	   not correspond to items in the MIDI file. */

    switch (rtype)
    {
    case Marker(Header):
      if (headerseen)
      {
        Warn("Duplicate header record");
        continue;
      }
      Nfields(3);
      memcpy(mh.chunktype, MIDI_Header_Sentinel, sizeof mh.chunktype);
      mh.length = 6;
      clamp(&(nfld[0]), 0, 2, "Field 4 (Format)");
      mh.format = (short)nfld[0];
      clamp(&(nfld[1]), 0, 65535, "Field 5 (Number of tracks)");
      mh.ntrks = (short)nfld[1];
      clamp(&(nfld[2]), 0, 65535, "Field 6 (Pulses per quarter note)");
      mh.division = (short)nfld[2];
      writeMidiFileHeader(fo, &mh);
      if (verbose)
      {
        fprintf(stderr, "Format %d MIDI file.  %d tracks, %d ticks per quarter note.\n",
                mh.format, mh.ntrks, mh.division);
      }
      headerseen = TRUE;
      continue;

    case Marker(Start_track):
      if (intrack != 0)
      {
        Warn("Previous track end missing");
        continue;
      }
      intrack  = track;
      tabstime = 0;
      continue;

    case Marker(End_of_file):
      eofseen = TRUE;
      if (intrack != 0)
      {
        Warn("Last track end missing");
      }
      continue;
    }

    /* Verify events occur within Start_track / End_track
	   brackets and that track number is correct. */

    if (track != intrack)
    {
      if (intrack == 0)
      {
        Warn("Event not within track");
        continue;
      }
      else
      {
        Warn("Incorrect track number in event");
        continue;
      }
    }

    /* Make sure absolute time isn't less than that
	   in previous event in this track. */

    if (abstime < tabstime)
    {
      Warn("Events out of order; this event is before the previous.");
      continue;
    }

    switch (rtype)
    {
#define ClampChannel Clamp(4, 0, 15, "Channel")
    case Event(NoteOff):
    case Event(NoteOn):
    case Event(PolyphonicKeyPressure):
    case Event(ControlChange):
      Nfields(3);
      ClampChannel;
      Clamp(5, 0, 127, "Note number");
      Clamp(6, 0, 127, "Value");
      outevent((byte)(etype | nfld[0]));
      outbyte((byte)nfld[1]);
      outbyte((byte)nfld[2]);
      break;

    case Event(ProgramChange):
    case Event(ChannelPressure):
      Nfields(2);
      ClampChannel;
      Clamp(5, 0, 127, "Value");
      outevent((byte)(etype | nfld[0]));
      outbyte((byte)nfld[1]);
      break;

    case Event(PitchBend):
      Nfields(2);
      ClampChannel;
      Clamp(5, 0, (1 << 14) - 1, "Value");
      outevent((byte)(etype | nfld[0]));
      /* Note that the pitch bend event consists of two
		   bytes each containing 7 bits of data with a
		   zero MSB.  Consequently, we cannot use outshort()
		   for the pitch bend data and must generate and emit
		   the two 7 bit values here.  The values are output
		   with the least significant 7 bits first, followed
		   by the most significant 7 bits. */
      outbyte((byte)(nfld[1] & 0x7F));
      outbyte((byte)((nfld[1] >> 7) & 0x7F));
      break;

      /* System-exclusive messages */

    case Event(SystemExclusive):
    case Event(SystemExclusivePacket):
      Nfields(1);
      Clamp(4, 0, (1 << 28) - 1, "Length");
      lastStatus = -1; /* Running status not used for sysex,
				    	       and sysex cancels any running
				    	       status in effect. */
      n          = nfld[0];
      if (checkBytes(5, n))
      {
        continue;
      }
      outevent((byte)etype);
      outVarLen(n); /* Length of following data */
      for (i = 0; i < n; i++)
      {
        /* We must call xfields() inside the loop for
		       each individual system exclusive field since
		       the number of fields is unlimited and may
		       exceed MAX_NFIELDS. */
        if (xfields(1, i + 5))
        {
          abort(); /* Can't happen, thanks to checkBytes() above */
        }
        clamp(&nfld[0], 0, 255, "Sysex data byte");
        outbyte((byte)nfld[0]);
      }
      break;

      /* File meta-events */

    case Meta(SequenceNumberMetaEvent):
      Nfields(1);
      Clamp(4, 0, (1 << 16) - 1, "Sequence number");
      outmeta((byte)etype);
      outbyte(2);
      outshort((short)nfld[0]);
      break;

    case Meta(TextMetaEvent):
    case Meta(CopyrightMetaEvent):
    case Meta(TrackTitleMetaEvent):
    case Meta(TrackInstrumentNameMetaEvent):
    case Meta(LyricMetaEvent):
    case Meta(MarkerMetaEvent):
    case Meta(CuePointMetaEvent):
      if (!CSVscanField(&f, &flen))
      {
        Warn("Missing field 4.");
      }
      outmeta((byte)etype);
      outbytes((byte*)f, CSVfieldLength);
      break;

    case Meta(ChannelPrefixMetaEvent):
    case Meta(PortMetaEvent):
      Nfields(1);
      Clamp(4, 0, 255, "Number");
      outmeta((byte)etype);
      of[0] = (byte)nfld[0];
      outbytes(of, 1);
      break;

    case Meta(EndTrackMetaEvent):
      outmeta((byte)etype);
      outbyte(0); /* All meta events must include length */
      mt.length = trackbufp - trackbuf;
      writeMidiTrackHeader(fo, &mt);
      if (verbose)
      {
        fprintf(stderr, "Track %d: length %ld.\n", ntrack + 1, mt.length);
      }
      fwrite(trackbuf, trackbufp - trackbuf, 1, fo);
      trackbufp  = trackbuf;
      tabstime   = 0;
      lastStatus = -1;
      intrack    = 0;
      ntrack++;
      break;

    case Meta(SetTempoMetaEvent):
      Nfields(1);
      Clamp(4, 0, (1 << 24) - 1, "Value");
      outmeta((byte)etype);
      of[0] = (byte)((nfld[0] >> 16) & 0xFF);
      of[1] = (byte)((nfld[0] >> 8) & 0xFF);
      of[2] = (byte)(nfld[0] & 0xFF);
      outbytes(of, 3);
      break;

    case Meta(SMPTEOffsetMetaEvent):
      Nfields(5);
      Clamp(4, 0, 255, "Hour");
      Clamp(5, 0, 255, "Minute");
      Clamp(6, 0, 255, "Second");
      Clamp(7, 0, 255, "Frame");
      Clamp(8, 0, 255, "FrameFraction");
      outmeta((byte)etype);
      for (i = 0; i < 5; i++)
      {
        of[i] = (byte)nfld[i];
      }
      outbytes(of, 5);
      break;

    case Meta(TimeSignatureMetaEvent):
      Nfields(4);
      Clamp(4, 0, 255, "Numerator");
      Clamp(5, 0, 255, "Denominator");
      Clamp(6, 0, 255, "Click");
      Clamp(7, 0, 255, "NotesPerQuarter");
      outmeta((byte)etype);
      for (i = 0; i < 4; i++)
      {
        of[i] = (byte)nfld[i];
      }
      outbytes(of, 4);
      break;

    case Meta(KeySignatureMetaEvent):
      Nfields(1);
      if (!CSVscanField(&f, &flen))
      {
        Warn("Missing field 5");
      }
      outmeta((byte)etype);
      Clamp(4, -7, 7, "Key");
      of[0] = (byte)nfld[0];
      if (strcasecmp(f, "major") == 0)
      {
        of[1] = 0;
      }
      else if (strcasecmp(f, "minor") == 0)
      {
        of[1] = 1;
      }
      else
      {
        sprintf(errm, "Field 5 has invalid major/minor indicator \"%s\"", f);
        Warn(errm);
        of[1] = 0;
      }
      outbytes(of, 2);
      break;

    case Meta(SequencerSpecificMetaEvent):
      Nfields(1);
      Clamp(4, 0, (1 << 28) - 1, "Length");
      n = nfld[0];
      if (checkBytes(5, n))
      {
        continue;
      }
      outmeta((byte)etype);
      outVarLen(n); /* Length of following data */
      for (i = 0; i < n; i++)
      {
        if (xfields(1, i + 5))
        {          /* Scan data field */
          abort(); /* Can't happen, thanks to checkBytes() above */
        }
        clamp(&nfld[0], 0, 255, "Sequencer specific data byte");
        outbyte((byte)nfld[0]);
      }
      break;

      /* The code 0xFF indicates an unknown file meta
	       event.  Since meta events include a length field,
	       such events can be preserved in a CSV file by
	       including the unknown event code in the record. */

    case Meta(0xFF):
      Nfields(2); /* Get type and length */
      Clamp(4, 0, 255, "UnknownMetaType");
      Clamp(5, 0, (1 << 28) - 1, "UnknownMetaLength");
      etype = nfld[0];
      n     = nfld[1];
      if (checkBytes(6, n))
      {
        continue;
      }
      /* We output the length as a variable-length quantity
		   on the assumption that any meta event which can
		   have data longer than 127 bytes will use a variable
		   length len field. */
      outmeta((byte)etype);
      outVarLen(n);
      for (i = 0; i < n; i++)
      {
        if (xfields(1, i + 6))
        {          /* Scan data field */
          abort(); /* Can't happen, thanks to checkBytes() above */
        }
        clamp(&nfld[0], 0, 255, "Unknown meta data byte");
        outbyte((byte)nfld[0]);
      }
      break;
    }
  }
  fclose(fp);

  if (!eofseen)
  {
    fprintf(stderr, "%s: Missing End_of_file record.\n", PROG);
    return 1;
  }

  return ((errors > 0) ? 1 : 0);
}

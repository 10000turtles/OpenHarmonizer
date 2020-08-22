
#include "MidiCsv/midio.h"
#include "unistd.h"
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

typedef unsigned char byte; /* MIDI data stream byte */
typedef unsigned long vlint;

#define FALSE 0
#define TRUE 1

static char* progname;        /* Program name string */
static int   verbose = FALSE; /* Debug output */

struct mhead
{
  char  chunktype[4]; /* Chunk type: "MThd" */
  long  length;       /* Length: 6 */
  short format;       /* File format */
  short ntrks;        /* Number of tracks in file */
  short division;     /* Time division */
};
struct mtrack
{
  char chunktype[4]; /* Chunk type: "MTrk" */
  long length;       /* Length of track */
};
typedef enum
{

  /* Channel voice messages */

  NoteOff               = 0x80,
  NoteOn                = 0x90,
  PolyphonicKeyPressure = 0xA0,
  ControlChange         = 0xB0,
  ProgramChange         = 0xC0,
  ChannelPressure       = 0xD0,
  PitchBend             = 0xE0,

  /* Channel mode messages */

  ChannelMode = 0xB8,

  /* System messages */

  SystemExclusive               = 0xF0,
  SystemCommon                  = 0xF0,
  SystemExclusivePacket         = 0xF7,
  SystemRealTime                = 0xF8,
  SystemStartCurrentSequence    = 0xFA,
  SystemContinueCurrentSequence = 0xFB,
  SystemStop                    = 0xFC,

  /* MIDI file-only messages */

  FileMetaEvent = 0xFF
} midi_command;
typedef enum
{
  SequenceNumberMetaEvent      = 0,
  TextMetaEvent                = 1,
  CopyrightMetaEvent           = 2,
  TrackTitleMetaEvent          = 3,
  TrackInstrumentNameMetaEvent = 4,
  LyricMetaEvent               = 5,
  MarkerMetaEvent              = 6,
  CuePointMetaEvent            = 7,

  ChannelPrefixMetaEvent = 0x20,
  PortMetaEvent          = 0x21,
  EndTrackMetaEvent      = 0x2F,

  SetTempoMetaEvent      = 0x51,
  SMPTEOffsetMetaEvent   = 0x54,
  TimeSignatureMetaEvent = 0x58,
  KeySignatureMetaEvent  = 0x59,

  SequencerSpecificMetaEvent = 0x7F
} midifile_meta_event;
long readlong(FILE* fp)
{
  unsigned char c[4];

  fread((char*)c, 1, sizeof c, fp);
  return (long)((c[0] << 24) | (c[1] << 16) | (c[2] << 8) | c[3]);
}
short readshort(FILE* fp)
{
  unsigned char c[2];

  fread((char*)c, 1, sizeof c, fp);
  return (short)((c[0] << 8) | c[1]);
}
vlint readVarLen(FILE* fp)
{
  long value;
  int  ch;

  if ((value = getc(fp)) & 0x80)
  {
    value &= 0x7F;
    do
    {
      value = (value << 7) | ((ch = getc(fp)) & 0x7F);
    } while (ch & 0x80);
  }
  return value;
}
void readMidiFileHeader(FILE* fp, struct mhead* h)
{
  fread(h->chunktype, sizeof h->chunktype, 1, fp);
  h->length   = readlong(fp);
  h->format   = readshort(fp);
  h->ntrks    = readshort(fp);
  h->division = readshort(fp);
}
void readMidiTrackHeader(FILE* fp, struct mtrack* t)
{
  fread(t->chunktype, sizeof t->chunktype, 1, fp);
  t->length = readlong(fp);
}

void writelong(FILE* fp, const long l)
{
  putc((l >> 24) & 0xFF, fp);
  putc((l >> 16) & 0xFF, fp);
  putc((l >> 8) & 0xFF, fp);
  putc(l & 0xFF, fp);
}

void writeshort(FILE* fp, const short s)
{
  putc((s >> 8) & 0xFF, fp);
  putc(s & 0xFF, fp);
}

void writeVarLen(FILE* fp, const vlint v)
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

  while (1)
  {
    putc((int)(buffer & 0xFF), fp);
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

void writeMidiFileHeader(FILE* fp, struct mhead* h)
{
  fwrite(h->chunktype, sizeof h->chunktype, 1, fp);
  writelong(fp, h->length);
  writeshort(fp, h->format);
  writeshort(fp, h->ntrks);
  writeshort(fp, h->division);
}

void writeMidiTrackHeader(FILE* fp, struct mtrack* t)
{
  fwrite(t->chunktype, sizeof t->chunktype, 1, fp);
  writelong(fp, t->length);
}

static vlint vlength(byte** trk, long* trklen)
{
  vlint value;
  byte  ch;
  byte* cp = *trk;

  trklen--;
  if ((value = *cp++) & 0x80)
  {
    value &= 0x7F;
    do
    {
      value = (value << 7) | ((ch = *cp++) & 0x7F);
      trklen--;
    } while (ch & 0x80);
  }
#ifdef DUMP
  fprintf(stderr, "Time lapse: %d bytes, %d\n", cp - *trk, value);
#endif
  *trk = cp;
  return value;
}
static void textcsv(FILE* fo, const byte* t, const int len)
{
  byte c;
  int  i;

  putc('"', fo);
  for (i = 0; i < len; i++)
  {
    c = *t++;
    if ((c < ' ') ||
#ifdef CSV_Quote_ISO
        ((c > '~') && (c <= 160))
#else
        (c > '~')
#endif
    )
    {
      putc('\\', fo);
      fprintf(fo, "%03o", c);
    }
    else
    {
      if (c == '"')
      {
        putc('"', fo);
      }
      else if (c == '\\')
      {
        putc('\\', fo);
      }
      putc(c, fo);
    }
  }
  putc('"', fo);
}
static void trackcsv(FILE* fo, const int trackno, byte* trk, long trklen, const int ppq)
{
  int levt = 0, evt, channel, note, vel, control, value,
      type;
  vlint len;
  byte* titem;
  vlint abstime = 0; /* Absolute time in track */
#ifdef XDD
  byte* strk = trk;
#endif

  while (trklen > 0)
  {
    vlint tlapse = vlength(&trk, &trklen);
    abstime += tlapse;

    fprintf(fo, "%d, %ld, ", trackno, abstime);

    /* Handle running status; if the next byte is a data byte,
	   reuse the last command seen in the track. */

    if (*trk & 0x80)
    {
#ifdef XDD
      fprintf(fo, " (Trk: %02x NS: %02X : %d) ", *trk, evt, trk - strk);
#endif
      evt = *trk++;

      /* One subtlety: we only save channel voice messages
	       for running status.  System messages and file
	       meta-events (all of which are in the 0xF0-0xFF
	       range) are not saved, as it is possible to carry a
	       running status across them.  You may have never seen
	       this done in a MIDI file, but I have. */

      if ((evt & 0xF0) != 0xF0)
      {
        levt = evt;
      }
      trklen--;
    }
    else
    {
      evt = levt;
#ifdef XDD
      fprintf(fo, " (Trk: %02x RS: %02X : %d) ", *trk, evt, trk - strk);
#endif
    }

    channel = evt & 0xF;

    /* Channel messages */

    switch (evt & 0xF0)
    {
    case NoteOff: /* Note off */
      if (trklen < 2)
      {
        return;
      }
      trklen -= 2;
      note = *trk++;
      vel  = *trk++;
      fprintf(fo, "Note_off_c, %d, %d, %d\n", channel, note, vel);
      continue;

    case NoteOn: /* Note on */
      if (trklen < 2)
      {
        return;
      }
      trklen -= 2;
      note = *trk++;
      vel  = *trk++;
      /*  A note on with a velocity of 0 is actually a note
		    off.  We do not translate it to a Note_off record
		    in order to preserve the original structure of the
		    MIDI file.	*/
      fprintf(fo, "Note_on_c, %d, %d, %d\n", channel, note, vel);
      continue;

    case PolyphonicKeyPressure: /* Aftertouch */
      if (trklen < 2)
      {
        return;
      }
      trklen -= 2;
      note = *trk++;
      vel  = *trk++;
      fprintf(fo, "Poly_aftertouch_c, %d, %d, %d\n", channel, note, vel);
      continue;

    case ControlChange: /* Control change */
      if (trklen < 2)
      {
        return;
      }
      trklen -= 2;
      control = *trk++;
      value   = *trk++;
      fprintf(fo, "Control_c, %d, %d, %d\n", channel, control, value);
      continue;

    case ProgramChange: /* Program change */
      if (trklen < 1)
      {
        return;
      }
      trklen--;
      note = *trk++;
      fprintf(fo, "Program_c, %d, %d\n", channel, note);
      continue;

    case ChannelPressure: /* Channel pressure (aftertouch) */
      if (trklen < 1)
      {
        return;
      }
      trklen--;
      vel = *trk++;
      fprintf(fo, "Channel_aftertouch_c, %d, %d\n", channel, vel);
      continue;

    case PitchBend: /* Pitch bend */
      if (trklen < 1)
      {
        return;
      }
      trklen--;
      value = *trk++;
      value = value | ((*trk++) << 7);
      fprintf(fo, "Pitch_bend_c, %d, %d\n", channel, value);
      continue;

    default:
      break;
    }

    switch (evt)
    {
      /* System exclusive messages */

    case SystemExclusive:
    case SystemExclusivePacket:
      len = vlength(&trk, &trklen);
      fprintf(fo, "System_exclusive%s, %lu",
              evt == SystemExclusivePacket ? "_packet" : "",
              len);
      {
        vlint i;

        for (i = 0; i < len; i++)
        {
          fprintf(fo, ", %d", *trk++);
        }
        fprintf(fo, "\n");
      }
      break;

      /* File meta-events */

    case FileMetaEvent:

      if (trklen < 2)
      {
        return;
      }
      trklen -= 2;
      type  = *trk++;
      len   = vlength(&trk, &trklen);
      titem = trk;
      trk += len;
      trklen -= len;

      switch (type)
      {
      case SequenceNumberMetaEvent:
        fprintf(fo, "Sequence_number, %d\n", (titem[0] << 8) | titem[1]);
        break;

      case TextMetaEvent:
#ifdef XDD
        fprintf(fo, " (Len=%ld  Trk=%02x) ", len, *trk);
#endif
        fputs("Text_t, ", fo);
        textcsv(fo, titem, len);
        putc('\n', fo);
        break;

      case CopyrightMetaEvent:
        fputs("Copyright_t, ", fo);
        textcsv(fo, titem, len);
        putc('\n', fo);
        break;

      case TrackTitleMetaEvent:
        fputs("Title_t, ", fo);
        textcsv(fo, titem, len);
        putc('\n', fo);
        break;

      case TrackInstrumentNameMetaEvent:
        fputs("Instrument_name_t, ", fo);
        textcsv(fo, titem, len);
        putc('\n', fo);
        break;

      case LyricMetaEvent:
        fputs("Lyric_t, ", fo);
        textcsv(fo, titem, len);
        putc('\n', fo);
        break;

      case MarkerMetaEvent:
        fputs("Marker_t, ", fo);
        textcsv(fo, titem, len);
        putc('\n', fo);
        break;

      case CuePointMetaEvent:
        fputs("Cue_point_t, ", fo);
        textcsv(fo, titem, len);
        putc('\n', fo);
        break;

      case ChannelPrefixMetaEvent:
        fprintf(fo, "Channel_prefix, %d\n", titem[0]);
        break;

      case PortMetaEvent:
        fprintf(fo, "MIDI_port, %d\n", titem[0]);
        break;

      case EndTrackMetaEvent:
        fprintf(fo, "End_track\n");
        trklen = -1;
        break;

      case SetTempoMetaEvent:
        fprintf(fo, "Tempo, %d\n", (titem[0] << 16) | (titem[1] << 8) | titem[2]);
        break;

      case SMPTEOffsetMetaEvent:
        fprintf(fo, "SMPTE_offset, %d, %d, %d, %d, %d\n",
                titem[0], titem[1], titem[2], titem[3], titem[4]);
        break;

      case TimeSignatureMetaEvent:
        fprintf(fo, "Time_signature, %d, %d, %d, %d\n",
                titem[0], titem[1], titem[2], titem[3]);
        break;

      case KeySignatureMetaEvent:
        fprintf(fo, "Key_signature, %d, \"%s\"\n", ((signed char)titem[0]),
                titem[1] ? "minor" : "major");
        break;

      case SequencerSpecificMetaEvent:
        fprintf(fo, "Sequencer_specific, %lu", len);
        {
          vlint i;

          for (i = 0; i < len; i++)
          {
            fprintf(fo, ", %d", titem[i]);
          }
          fprintf(fo, "\n");
        }
        break;

      default:
        if (verbose)
        {
          fprintf(stderr, "Unknown meta event type 0x%02X, %ld bytes of data.\n",
                  type, len);
        }
        fprintf(fo, "Unknown_meta_event, %d, %lu", type, len);
        {
          vlint i;

          for (i = 0; i < len; i++)
          {
            fprintf(fo, ", %d", titem[i]);
          }
          fprintf(fo, "\n");
        }
        break;
      }
      break;

    default:
      if (verbose)
      {
        fprintf(stderr, "Unknown event type 0x%02X.\n", evt);
      }
      fprintf(fo, "Unknown_event, %02Xx\n", evt);
      break;
    }
  }
}

void midiToCsv(std::string midiFilename, std::string csvFilename)
{
  struct mhead mh;
  FILE *       fp, *fo;
  long         track1;
  int          i, n, track1l;

  fp = fopen(midiFilename.c_str(), "rb");
  fo = fopen(csvFilename.c_str(), "w");
  readMidiFileHeader(fp, &mh);

  fprintf(fo, "0, 0, Header, %d, %d, %d\n", mh.format, mh.ntrks, mh.division);

  /*	Process tracks */

  for (i = 0; i < mh.ntrks; i++)
  {
    struct mtrack  mt;
    unsigned char* trk;

    if (i == 0)
    {
      track1 = ftell(fp);
    }

    readMidiTrackHeader(fp, &mt);

    fprintf(fo, "%d, 0, Start_track\n", i + 1);

    trk = (unsigned char*)malloc(mt.length);

    fread((char*)trk, (int)mt.length, 1, fp);
    if (i == 0)
    {
      track1l = (int)(ftell(fp) - track1);
    }

    trackcsv(fo, i + 1, trk, mt.length, mh.division);
    free(trk);
  }
  fprintf(fo, "0, 0, End_of_file\n");
  fclose(fp);
  fclose(fo);
}
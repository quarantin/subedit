/*
 * Copyright (C) 2010 Corentin Delorme corentin.delorme at gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/*

2
00:00:19,452 --> 00:00:22,444
In this morning's paper,
you may come across an article

*/


/*
 * TODO
 * take a list of subtitle files on the command line instead of just one
 * fix_case: upper case first letter of an acronym
 * fix_case: upper case all 'i'
 * fix_case: upper case Mr, Mrs, Dr, etc.
 * fix_case: don't upper case first letter after an acronym
 * fix_case: add a flag to specify a dict file to help fixing case
 * problem with many srt files: x,50 instead of x,050
 * remove signature idx=9999
 * edit delay of line at index
 * help for timespec format
 * rm {y:i} when converting from sub to srt
 * man page
 */


#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <locale.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>


#define SIZE 256


#define VPRINTF(...) do {           \
  if (verbose)                      \
    fprintf(stderr, __VA_ARGS__);   \
} while (0)


#define FOPEN(ret, file, mode) {    \
  ret = fopen(file, mode);          \
  if (!ret) {                       \
    perror("fopen failed");         \
    exit(EXIT_FAILURE);             \
  }                                 \
}


int ask = 0;
int fixi = 0;
int fixc = 0;
int lower = 0;
int verbose = 0;
int upper_next = 1;

double start = 0;
double end = 10000000;
double framerate = 0;
char *name = "subedit";


void usage(char *format, ...)
{
  if (format) {
    va_list list;
    va_start(list, format);
    fprintf(stderr, format, list);
    va_end(list);
  }
  fprintf(stderr, "\nUsage:\n\t%s [options] <filename>\n\n"
                  "\t\t-h                    print this help and exit\n"
                  "\t\t-a                    ask confirmation\n"
                  "\t\t-d <+|-><timespec>    add/substract timespec to subtitles entries\n"
                  "\t\t-e <timespec>         end processing the file at <timespec>\n"
                  "\t\t-f                    fix indexes\n"
                  "\t\t-F                    fix case\n"
                  "\t\t-l                    lower case\n"
                  "\t\t-s <timespec>         start processing the file at <timespec>\n"
                  "\t\t-t <framerate>        translate given .sub file to .srt file\n"
                  "\t\t-v                    be verbose\n"
                  "\n", name);
  exit(EXIT_FAILURE);
}


void get_eol (FILE *in, char *eol)
{
  char *p = NULL;
  char buf[SIZE + 1];

  memset(eol, 0, 3);
  memset(buf, 0, SIZE + 1);
  fgets(buf, SIZE, in);
  if (!buf[0])
    goto not_found;
  p = rindex(buf, '\r');
  if (!p) {
    p = rindex(buf, '\n');
    if (!p)
      goto not_found;
  }
  strncpy(eol, p, 2);
  rewind(in);
  return;

not_found:
  strncpy(eol, "\r\n", 2);
  rewind(in);
}


void to_lower (char *line)
{
  char c;
  unsigned int i;
  unsigned int len;

  len = strlen((char *)line);
  for (i = 0; i < len; i++) {
    c = line[i];
    if ('A' <= c && c <= 'Z')
      line[i] = tolower(c);
  }
}


void fix_case (char *line)
{
  char c;
  unsigned int i;
  unsigned int len;

  len = strlen(line);
  for (i = 0; i < len; i++) {
    c = line[i];
    switch (c) {
      case '.':
      case '?':
      case '!':
      case '[':
      case ']':
      case '"':
        upper_next = 1;
        break;
    }
    if (0)
      ;
    else if ('A' <= c && c <= 'Z')
      upper_next = 0;
    else if ('a' <= c && c <= 'z' && upper_next) {
      line[i] = toupper(c);
      upper_next = 0;
    }
  }
}


void process_subtitle (FILE *in, FILE *out, double delay)
{
  int ret;
  int idx;
  double diff;
  double a, fa, ma;
  double b, fb, mb;
  double sh, sm, ss;
  double eh, em, es;
  char line[SIZE + 1];
  char eol[3];

  get_eol(in, eol);
  if (!setlocale(LC_NUMERIC, "fr_FR")) {
    perror("setlocale failed. You need fr_FR locale");
    fclose(in);
    fclose(out);
    exit(EXIT_FAILURE);
  }
    
  while (fgets(line, SIZE, in)) {
    ret = sscanf(line, "%lf:%lf:%lf --> %lf:%lf:%lf", &sh, &sm, &ss, &eh, &em, &es);
    if (ret == 6) {
      upper_next = 1;
      a = (sh * 3600) + (sm * 60) + ss;
      b = (eh * 3600) + (em * 60) + es;
      diff = b - a;
      if (start <= a && a <= end)
        a = fmax(0, a + delay);
      b = a + diff;
      fa = floor(a); ma = fmod(fa, 3600);
      fb = floor(b); mb = fmod(fb, 3600);
      snprintf(line, SIZE, "%02.0f:%02.0f:%02.0f,%03.0f --> %02.0f:%02.0f:%02.0f,%03.0f%s",
          floor(fa / 3600), floor(ma / 60), fmod(ma, 60), (a - fa) * 1000,
          floor(fb / 3600), floor(mb / 60), fmod(mb, 60), (b - fb) * 1000,
          eol);
    }
    else if (fixi) {
      ret = sscanf(line, "%d\n", &idx);
      if (ret && ret != EOF)
        snprintf(line, SIZE, "%d%s", (idx == 9999 ? 9999 : fixi++), eol);
    }

    if (lower)
      to_lower(line);

    if (fixc)
      fix_case(line);

    fprintf(out, "%s", line);
  }
}


void sub_to_srt (FILE *in, FILE *out, double framerate)
{
  int idx, ret;
  int start, end;
  double s, sf, sm;
  double e, ef, em;
  char text[SIZE + 1];
  char line[SIZE + 1];
  char *token;

  idx = 1;
  while (fgets(line, SIZE, in)) {
    memset(text, 0, SIZE + 1);
    ret = sscanf(line, "{%d}{%d}%256c", &start, &end, text);
    if (ret == 3) {
      s = start / framerate; sf = floor(s); sm = fmod(sf, 3600);
      e = end   / framerate; ef = floor(e); em = fmod(ef, 3600);
      fprintf(out, "%d\r\n", idx++);
      fprintf(out, "%02.0f:%02.0f:%02.0f,%03.0f --> %02.0f:%02.0f:%02.0f,%03.0f\r\n",
        floor(sf / 3600), floor(sm / 60), fmod(sm, 60), (s - sf) * 1000,
        floor(ef / 3600), floor(em / 60), fmod(em, 60), (e - ef) * 1000
      );
      token = strtok(text, "|\r\n");
      while (token) {
        fprintf(out, "%s\r\n", token);
        token = strtok(NULL, "|\r\n");
      }
      fprintf(out, "\r\n");
    }
    else
      fprintf(stderr, "sub_to_srt: invalid format: %s\n", line);
  }
}


double parse_timespec (const char *string)
{
  int ret;
  double h = 0, m = 0, s = 0;

  if (!string || !string[0])
    return 0;

  ret = sscanf(string, "%lf:%lf:%lf", &h, &m, &s);
  switch (ret) {
    case 1: s = h; m = 0; h = 0; break;
    case 2: s = m; m = h; h = 0; break;
    case 3: break;
    default:
      ferror(stderr);
      usage(NULL);
  }
  return (h * 3600) + (m * 60) + s;
}


double parse_signed_timespec (const char *string)
{
  char op;
  double timespec;

  if (!string || !string[0])
    return 0;

  op = string[0];
  timespec = parse_timespec(&string[1]);
  switch (op) {
    case '+': break;
    case '-': timespec *= -1; break;
    default:
      usage("No sign given for delay\n");
  }
  return timespec;
}


int main (int argc, char **argv)
{
  int i = 1;
  int c, ret;
  FILE *in, *out;
  double delay = 0;
  char buf[SIZE + 1];
  char backup[SIZE + 1];
  char output[SIZE + 1];
  char *input = NULL;
  struct stat info;

  if (argv && argv[0])
    name = argv[0];

  if (argc < 2)
    usage("Not enough arguments\n");

  opterr = 0;
  while ((c = getopt(argc, argv, "ahd:e:fFls:t:v")) != -1)
    switch (c) {
      case 'a': ask = 1; break;
      case 'd': delay = parse_signed_timespec(optarg); break;
      case 'e': end = parse_timespec(optarg); break;
      case 'f': fixi = 1; break;
      case 'F': fixc = 1; break;
      case 'l': lower = 1; break;
      case 'h': usage(NULL); break;
      case 's': start = parse_timespec(optarg); break;
      case 't': framerate = strtod(optarg, NULL); break;
      case 'v': verbose = 1; break;
      case '-': goto out;
      default:
        usage("Invalid option: -%c\n", c);
    }

out:

  if (argc - optind != 1)
    usage("Invalid number of arguments\n");

  if (!delay && !fixi && !fixc && !lower && !framerate)
    usage("You must specify at least one of -d, -f, -F, -l or -t\n");

  while (ask) {
    printf("Do you want to continue ? [Y/n]\n");
    fgets(buf, SIZE, stdin);
    if (*buf == 'n' || *buf == 'N') {
      VPRINTF("Quitting.\n");
      exit(EXIT_SUCCESS);
    }
    if (*buf == 'y' || *buf == 'Y' || *buf == '\n' || *buf == '\r') {
      VPRINTF("Proceeding ...\n");
      break;
    }
  }

  input = argv[optind];
  snprintf(output, SIZE, "%s.out", input);

  FOPEN(in,  input,  "r");
  FOPEN(out, output, "wx");

  if (framerate)
    sub_to_srt(in, out, framerate);
  else
    process_subtitle(in, out, delay);

  fflush(out);
  fclose(out);
  fclose(in);

  do {
    snprintf(backup, SIZE, "%s.%04d.bkp", input, i++);
    ret = stat(backup, &info);
  } while (!ret);

  ret = rename(input, backup);
  if (ret) {
    perror("rename failed");
    exit(EXIT_FAILURE);
  }
  ret = rename(output, input);
  if (ret) {
    perror("rename failed");
    exit(EXIT_FAILURE);
  }
  VPRINTF("Done\n");
  return 0;
}


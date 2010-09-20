/* Xsynth DSSI software synthesizer plugin and GUI
 *
 * Copyright (C) 2004, 2009, 2010 Sean Bolton and others.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include <locale.h>

#include "dssi.h"

#include "xsynth_voice.h"
#include "gui_data.h"

xsynth_patch_t xsynth_init_voice = {
    "  <-->",
    1.0f, 0, 0.5f,                            /* osc1 */
    1.0f, 0, 0.5f,                            /* osc2 */
    0,                                        /* sync */
    0.5f,                                     /* balance */
    0.1f, 0, 0.0f, 0.0f,                      /* lfo */
    0.1f, 0.1f, 1.0f, 0.1f, 0.0f, 0.0f, 0.0f, /* eg1 */
    0.1f, 0.1f, 1.0f, 0.1f, 0.0f, 0.0f, 0.0f, /* eg2 */
    50.0f, 0.0f, 0,                           /* vcf */
    0.984375f,                                /* glide */
    0.5f                                      /* volume */
};

static int
is_comment(char *buf)  /* line is blank, whitespace, or first non-whitespace character is '#' */
{
    int i = 0;

    while (buf[i]) {
        if (buf[i] == '#') return 1;
        if (buf[i] == '\n') return 1;
        if (buf[i] != ' ' && buf[i] != '\t') return 0;
        i++;
    }
    return 1;
}

static void
parse_name(const char *buf, char *name, int *inlen)
{
    int i = 0, o = 0;
    unsigned int t;

    while (buf[i] && o < 30) {
        if (buf[i] < 33 || buf[i] > 126) {
            break;
        } else if (buf[i] == '%') {
            if (buf[i + 1] && buf[i + 2] && sscanf(buf + i + 1, "%2x", &t) == 1) {
                name[o++] = (char)t;
                i += 3;
            } else {
                break;
            }
        } else {
            name[o++] = buf[i++];
        }
    }
    /* trim trailing spaces */
    while (o && name[o - 1] == ' ') o--;
    name[o] = '\0';

    if (inlen) *inlen = i;
}

/* y_sscanf.c - dual-locale sscanf
 *
 * Like the standard sscanf(3), but this version accepts floating point
 * numbers containing either the locale-specified decimal point or the
 * "C" locale decimal point (".").  That said, there are important
 * differences between this and a standard sscanf(3):
 * 
 *  - this function only skips whitespace when there is explicit ' ' in
 *      the format,
 *  - it doesn't return EOF like sscanf(3), just the number of sucessful
 *      conversions,
 *  - it doesn't match character classes,
 *  - there is only one length modifier: 'l', and it only applies to 'd'
 *      (for int64_t) and 'f' (for double),
 *  - '%s' with no field-width specified is an error,
 *  - there are no 'i, 'o', 'p', or 'u' conversions, similarly
 *  - use 'f' instead of 'a', 'A', 'e', 'E', 'F', 'g', or 'G', and
 *  - '%f' doesn't do hexadecimal, infinity or NaN.
 */

static int
_is_whitespace(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
}

static int
_is_digit(char c)
{
    return (c >= '0' && c <= '9');
}

static int
_is_hexdigit(char c)
{
    return ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F'));
}

static int _atof(const char *buffer, double *result); /* forward */

static int
y_vsscanf(const char *buffer, const char *format, va_list ap)
{
    char fc;
    const char *bp = buffer;
    int conversions = 0;

    while ((fc = *format++)) {

        if (_is_whitespace(fc)) { /* skip whitespace */

            while (_is_whitespace(*bp))
                bp++;

        } else if (fc == '%') { /* a conversion */

            int skip = 0;   /* '*' no-store modifier */
            int width = 0;  /* field width */
            int big = 0;    /* storage length modifier */

            if (*format == '*') {
                skip = 1;
                format++;
            }
            while (_is_digit(*format)) {
                width = width * 10 + (*format - '0');
                format++;
            }
            if (*format == 'l') {
                big = 1;
                format++;
            }

            if (*format == '%') {  /* '%' - literal percent character */

                if (*bp == '%')
                    bp++;
                else
                    break;
                format++;

            } else if (*format == 'c') {  /* 'c' - one or more characters */

                int i;
                char *sp = va_arg(ap, char *);

                if (width == 0) width = 1;
                for (i = 0; i < width && *bp != 0; i++, bp++)
                    if (!skip) *sp++ = *bp;
                if (i > 0 && !skip)
                    conversions++;
                format++;

            } else if (*format == 'd') {  /* 'd' - 32 or 64 bit signed decimal integer */

                int negative = 0;
                int i;
                int64_t n = 0;

                if (*bp == '-') {
                    negative = 1;
                    bp++;
                }
                for (i = negative; (width == 0 || i < width) && _is_digit(*bp); i++, bp++)
                    n = n * 10 + (*bp - '0');
                if (i == negative) /* no digits converted */
                    break;
                if (negative) n = -n;
                if (!skip) {
                    if (big)
                        *va_arg(ap, int64_t *) = n;
                    else
                        *va_arg(ap, int32_t *) = n;
                    conversions++;
                }
                format++;

            } else if (*format == 'f') {  /* 'f' - float or double */

                double d;
                int n = _atof(bp, &d);
                if (n == 0)  /* no digits converted */
                    break;
                if (!skip) {
                    if (big)
                        *va_arg(ap, double *) = d;
                    else
                        *va_arg(ap, float *) = (float)d;
                    conversions++;
                }
                bp += n;
                format++;

            } else if (*format == 'n') {  /* 'n' - store number of characters scanned so far */

                if (!skip) *va_arg(ap, int *) = bp - buffer;
                format++;

            } else if (*format == 's') {  /* 's' - string of non-whitespace characters */

                int i;
                char *sp = va_arg(ap, char *);
                if (width == 0)
                    break; /* must specify a width */
                for (i = 0; i < width && *bp != 0 && !_is_whitespace(*bp); i++, bp++)
                    if (!skip) *sp++ = *bp;
                if (i > 0) {
                    if (!skip) {
                        *sp = 0;
                        conversions++;
                    }
                } else
                    break;  /* conversion failed */
                format++;

            } else if (*format == 'x') {  /* 'x' - 32 or 64 bit signed hexidecimal integer */
                int i;
                int64_t n = 0;

                for (i = 0; (width == 0 || i < width) && _is_hexdigit(*bp); i++, bp++) {
                    n = n * 16;
                    if (*bp >= 'a') n += *bp - 'a' + 10;
                    else if (*bp >= 'A') n += *bp - 'A' + 10;
                    else n += *bp - '0';
                }
                if (i == 0) /* no digits converted */
                    break;
                if (!skip) {
                    if (big)
                        *va_arg(ap, int64_t *) = n;
                    else
                        *va_arg(ap, int32_t *) = n;
                    conversions++;
                }
                format++;

            } else {
                break; /* bad conversion character */
            }

        } else if (fc == *bp) { /* a literal match */

            bp++;

        } else {  /* match fail */
            break;
        }
    }

    return conversions;
}

/* The following function is based on sqlite3AtoF() from sqlite 3.6.18.
 * The sqlite author disclaims copyright to the source code from which
 * this was adapted. */
static int
_atof(const char *z, double *pResult){
  const char *zBegin = z;
  /* sign * significand * (10 ^ (esign * exponent)) */
  int sign = 1;   /* sign of significand */
  int64_t s = 0;  /* significand */
  int d = 0;      /* adjust exponent for shifting decimal point */
  int esign = 1;  /* sign of exponent */
  int e = 0;      /* exponent */
  double result;
  int nDigits = 0;
  struct lconv *lc = localeconv();
  int dplen = strlen(lc->decimal_point);

  /* skip leading spaces */
  /* while( _is_whitespace(*z) ) z++; */
  /* get sign of significand */
  if( *z=='-' ){
    sign = -1;
    z++;
  }else if( *z=='+' ){
    z++;
  }
  /* skip leading zeroes */
  while( z[0]=='0' ) z++, nDigits++;

  /* copy max significant digits to significand */
  while( _is_digit(*z) && s<((INT64_MAX-9)/10) ){
    s = s*10 + (*z - '0');
    z++, nDigits++;
  }
  /* skip non-significant significand digits
  ** (increase exponent by d to shift decimal left) */
  while( _is_digit(*z) ) z++, nDigits++, d++;

  /* if decimal point is present */
  if( *z=='.' || !strncmp(z, lc->decimal_point, dplen) ) {
    if (*z=='.')
      z++;
    else
      z += dplen;
    /* copy digits from after decimal to significand
    ** (decrease exponent by d to shift decimal right) */
    while( _is_digit(*z) && s<((INT64_MAX-9)/10) ){
      s = s*10 + (*z - '0');
      z++, nDigits++, d--;
    }
    /* skip non-significant digits */
    while( _is_digit(*z) ) z++, nDigits++;
  } else if (nDigits == 0)
      return 0;  /* no significand digits converted */

  /* if exponent is present */
  if( *z=='e' || *z=='E' ){
    int eDigits = 0;
    z++;
    /* get sign of exponent */
    if( *z=='-' ){
      esign = -1;
      z++;
    }else if( *z=='+' ){
      z++;
    }
    /* copy digits to exponent */
    while( _is_digit(*z) ){
      e = e*10 + (*z - '0');
      z++, eDigits++;
    }
    if (eDigits == 0)
        return 0; /* malformed exponent */
  }

  /* adjust exponent by d, and update sign */
  e = (e*esign) + d;
  if( e<0 ) {
    esign = -1;
    e *= -1;
  } else {
    esign = 1;
  }

  /* if 0 significand */
  if( !s ) {
    /* In the IEEE 754 standard, zero is signed.
    ** Add the sign if we've seen at least one digit */
    result = (sign<0 && nDigits) ? -(double)0 : (double)0;
  } else {
    /* attempt to reduce exponent */
    if( esign>0 ){
      while( s<(INT64_MAX/10) && e>0 ) e--,s*=10;
    }else{
      while( !(s%10) && e>0 ) e--,s/=10;
    }

    /* adjust the sign of significand */
    s = sign<0 ? -s : s;

    /* if exponent, scale significand as appropriate
    ** and store in result. */
    if( e ){
      double scale = 1.0;
      /* attempt to handle extremely small/large numbers better */
      if( e>307 && e<342 ){
        while( e%308 ) { scale *= 1.0e+1; e -= 1; }
        if( esign<0 ){
          result = s / scale;
          result /= 1.0e+308;
        }else{
          result = s * scale;
          result *= 1.0e+308;
        }
      }else{
        /* 1.0e+22 is the largest power of 10 than can be 
        ** represented exactly. */
        while( e%22 ) { scale *= 1.0e+1; e -= 1; }
        while( e>0 ) { scale *= 1.0e+22; e -= 22; }
        if( esign<0 ){
          result = s / scale;
        }else{
          result = s * scale;
        }
      }
    } else {
      result = (double)s;
    }
  }

  /* store the result */
  *pResult = result;

  /* return number of characters used */
  return (int)(z - zBegin);
}

static int
y_sscanf(const char *str, const char *format, ...)
{
    va_list ap;
    int conversions;

    va_start(ap, format);
    conversions = y_vsscanf(str, format, ap);
    va_end(ap);

    return conversions;
}

/* end of y_sscanf.c */

int
xsynth_data_read_patch(FILE *file, xsynth_patch_t *patch)
{
    int format, i;
    char buf[256], buf2[90];
    xsynth_patch_t tmp;

    do {
        if (!fgets(buf, 256, file)) return 0;
    } while (is_comment(buf));

    if (sscanf(buf, " xsynth-dssi patch format %d begin", &format) != 1 ||
        format < 0 || format > 1)
        return 0;

    if (!fgets(buf, 256, file)) return 0;
    if (sscanf(buf, " name %90s", buf2) != 1) return 0;
    parse_name(buf2, tmp.name, NULL);

    if (!fgets(buf, 256, file)) return 0;
    if (y_sscanf(buf, " osc1 %f %d %f", &tmp.osc1_pitch, &i,
                 &tmp.osc1_pulsewidth) != 3)
        return 0;
    tmp.osc1_waveform = (unsigned char)i;

    if (!fgets(buf, 256, file)) return 0;
    if (y_sscanf(buf, " osc2 %f %d %f", &tmp.osc2_pitch, &i,
                 &tmp.osc2_pulsewidth) != 3)
        return 0;
    tmp.osc2_waveform = (unsigned char)i;

    if (!fgets(buf, 256, file)) return 0;
    if (sscanf(buf, " sync %d", &i) != 1)
        return 0;
    tmp.osc_sync = (unsigned char)i;

    if (!fgets(buf, 256, file)) return 0;
    if (y_sscanf(buf, " balance %f", &tmp.osc_balance) != 1)
        return 0;

    if (!fgets(buf, 256, file)) return 0;
    if (y_sscanf(buf, " lfo %f %d %f %f", &tmp.lfo_frequency, &i,
                 &tmp.lfo_amount_o, &tmp.lfo_amount_f) != 4)
        return 0;
    tmp.lfo_waveform = (unsigned char)i;

    if (format == 1) {

        if (!fgets(buf, 256, file)) return 0;
        if (y_sscanf(buf, " eg1 %f %f %f %f %f %f %f",
                     &tmp.eg1_attack_time, &tmp.eg1_decay_time,
                     &tmp.eg1_sustain_level, &tmp.eg1_release_time,
                     &tmp.eg1_vel_sens, &tmp.eg1_amount_o,
                     &tmp.eg1_amount_f) != 7)
            return 0;

        if (!fgets(buf, 256, file)) return 0;
        if (y_sscanf(buf, " eg2 %f %f %f %f %f %f %f",
                     &tmp.eg2_attack_time, &tmp.eg2_decay_time,
                     &tmp.eg2_sustain_level, &tmp.eg2_release_time,
                     &tmp.eg2_vel_sens, &tmp.eg2_amount_o,
                     &tmp.eg2_amount_f) != 7)
            return 0;

    } else {

        if (!fgets(buf, 256, file)) return 0;
        if (y_sscanf(buf, " eg1 %f %f %f %f %f %f",
                     &tmp.eg1_attack_time, &tmp.eg1_decay_time,
                     &tmp.eg1_sustain_level, &tmp.eg1_release_time,
                     &tmp.eg1_amount_o, &tmp.eg1_amount_f) != 6)
            return 0;

        if (!fgets(buf, 256, file)) return 0;
        if (y_sscanf(buf, " eg2 %f %f %f %f %f %f",
                     &tmp.eg2_attack_time, &tmp.eg2_decay_time,
                     &tmp.eg2_sustain_level, &tmp.eg2_release_time,
                     &tmp.eg2_amount_o, &tmp.eg2_amount_f) != 6)
            return 0;

        tmp.eg1_vel_sens = 0.0f;
        tmp.eg2_vel_sens = 0.0f;
    }

    if (!fgets(buf, 256, file)) return 0;
    if (y_sscanf(buf, " vcf %f %f %d", &tmp.vcf_cutoff, &tmp.vcf_qres, &i) != 3)
        return 0;
    tmp.vcf_mode = (unsigned char)i;

    if (!fgets(buf, 256, file)) return 0;
    if (y_sscanf(buf, " glide %f", &tmp.glide_time) != 1)
        return 0;

    if (!fgets(buf, 256, file)) return 0;
    if (y_sscanf(buf, " volume %f", &tmp.volume) != 1)
        return 0;

    if (!fgets(buf, 256, file)) return 0;
    if (sscanf(buf, " xsynth-dssi patch %3s", buf2) != 1) return 0;
    if (strcmp(buf2, "end")) return 0;

    memcpy(patch, &tmp, sizeof(xsynth_patch_t));

    return 1;  /* -FIX- error handling yet to be implemented */
}

int
xsynth_data_decode_patches(const char *encoded, xsynth_patch_t *patches)
{
    int j, n, i0, i1, i2, i3;
    const char *ep = encoded;
    xsynth_patch_t *tmp, *pp;

    if (strncmp(ep, "Xp0 ", 4)) {
        /* fprintf(stderr, "bad header\n"); */
        return 0;  /* bad format */
    }
    ep += 4;

    tmp = (xsynth_patch_t *)malloc(32 * sizeof(xsynth_patch_t));
    if (!tmp)
        return 0;  /* out of memory */
    
    for (j = 0; j < 32; j++) {
        pp = &tmp[j];

        parse_name(ep, pp->name, &n);
        if (!n) {
            /* fprintf(stderr, "failed in name\n"); */
            break;
        }
        ep += n;

        if (y_sscanf(ep, " %f %d %f %f %d %f %d %f %f %d %f %f%n",
                     &pp->osc1_pitch, &i0, &pp->osc1_pulsewidth,
                     &pp->osc2_pitch, &i1, &pp->osc2_pulsewidth,
                     &i2, &pp->osc_balance, &pp->lfo_frequency,
                     &i3, &pp->lfo_amount_o, &pp->lfo_amount_f,
                     &n) != 12) {
            /* fprintf(stderr, "failed in oscs\n"); */
            break;
        }
        pp->osc1_waveform = (unsigned char)i0;
        pp->osc2_waveform = (unsigned char)i1;
        pp->osc_sync = (unsigned char)i2;
        pp->lfo_waveform = (unsigned char)i3;
        ep += n;

        if (y_sscanf(ep, " %f %f %f %f %f %f %f %f %f %f %f %f %f %f%n",
                     &pp->eg1_attack_time, &pp->eg1_decay_time,
                     &pp->eg1_sustain_level, &pp->eg1_release_time,
                     &pp->eg1_vel_sens, &pp->eg1_amount_o, &pp->eg1_amount_f,
                     &pp->eg2_attack_time, &pp->eg2_decay_time,
                     &pp->eg2_sustain_level, &pp->eg2_release_time,
                     &pp->eg2_vel_sens, &pp->eg2_amount_o, &pp->eg2_amount_f,
                     &n) != 14) {
            /* fprintf(stderr, "failed in egs\n"); */
            break;
        }
        ep += n;

        if (y_sscanf(ep, " %f %f %d %f %f%n",
                     &pp->vcf_cutoff, &pp->vcf_qres, &i0,
                     &pp->glide_time, &pp->volume,
                     &n) != 5) {
            /* fprintf(stderr, "failed in vcf+\n"); */
            break;
        }
        pp->vcf_mode = (unsigned char)i0;
        ep += n;

        while (*ep == ' ') ep++;
    }

    if (j != 32 || strcmp(ep, "end")) {
        /* fprintf(stderr, "decode failed, j = %d, *ep = 0x%02x\n", j, *ep); */
        free(tmp);
        return 0;  /* too few patches, or otherwise bad format */
    }

    memcpy(patches, tmp, 32 * sizeof(xsynth_patch_t));

    free(tmp);

    return 1;
}


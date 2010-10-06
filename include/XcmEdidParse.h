/** @file Xcm_edid_parse.h
 *
 *  Xcm Xorg Colour Management
 *
 *  @par Copyright:
 *            2005-2010 (C) Kai-Uwe Behrmann
 *
 *  @brief    EDID data block parsing
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            MIT <http://www.opensource.org/licenses/mit-license.php>
 *  @since    2005/01/31
 */

#ifndef XCM_EDID_PARSE_H
#define XCM_EDID_PARSE_H
#include <stddef.h> /* size_t */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @brief \internal DDC struct */
typedef struct {
  unsigned char sig[8];
  unsigned char mnft_id[2];            /* [8] manufaturer ID */
  unsigned char model_id[2];           /* [10] model ID */
  unsigned char ser_id[2];             /* [12] serial ID */
  unsigned char dummy_li[2];
  unsigned char week;                  /* [16] Week */
  unsigned char year;                  /* [17] + 1990 => Year */
  unsigned char major_version;         /* [18] */
  unsigned char minor_version;         /* [19] */
  unsigned char video_input_type;      /* [20] */
  unsigned char width;                 /* [21] */
  unsigned char height;                /* [22] */
  unsigned char gamma_factor;          /* [23] */
  unsigned char dpms;                  /* [24] */
  unsigned char rg;                    /* [25] colour information */
  unsigned char wb;                    /* [26] */
  unsigned char rY;                    /* [27] */
  unsigned char rX;                    /* [28] */
  unsigned char gY;                    /* [29] */
  unsigned char gX;                    /* [30] */
  unsigned char bY;                    /* [31] */
  unsigned char bX;                    /* [32] */
  unsigned char wY;                    /* [33] */
  unsigned char wX;                    /* [34] */
  unsigned char etiming1;              /* [35] */
  unsigned char etiming2;              /* [36] */
  unsigned char mtiming;               /* [37] */
  unsigned char stdtiming[16];         /* [38] */
  unsigned char text1[18];             /* [54] Product string */
  unsigned char text2[18];             /* [72] text 2 */
  unsigned char text3[18];             /* [90] text 3 */
  unsigned char text4[18];             /* [108] text 4 */
  unsigned char extension_blocks;      /* [126] number of following extensions*/
  unsigned char checksum;              /* [127] */
} XcmEdid_s;

typedef enum {
  XCM_EDID_OK,
  XCM_EDID_WRONG_SIGNATURE
} XCM_EDID_ERROR_e;

typedef enum {
  XCM_EDID_VALUE_TEXT,
  XCM_EDID_VALUE_INT,
  XCM_EDID_VALUE_DOUBLE
} XCM_EDID_VALUE_e;

union XcmEdidValue_u {
  char * text;
  double dbl;
  int    integer;
};

typedef struct {
  const char         * key;
  XCM_EDID_VALUE_e        type;
  union XcmEdidValue_u   value;
} XcmEdidKeyValue_s;

/* basic access functions */
XCM_EDID_ERROR_e  XcmEdidParse        ( void              * edid,
                                       XcmEdidKeyValue_s** list,
                                       int               * count );
XCM_EDID_ERROR_e  XcmEdidFree        ( XcmEdidKeyValue_s** list );
const char *      XcmEdidErrorToString(XCM_EDID_ERROR_e    error );

/* convinience functions */
XCM_EDID_ERROR_e  XcmEdidPrintString ( void              * edid,
                                       char             ** text,
                                       void             *(*alloc)(size_t sz) );

#define XCM_EDID_KEY_VENDOR               "vendor"
#define XCM_EDID_KEY_MODEL                "model"
#define XCM_EDID_KEY_SERIAL               "serial"
#define XCM_EDID_KEY_REDx                 "redx"
#define XCM_EDID_KEY_REDy                 "redy"
#define XCM_EDID_KEY_GREENx               "greenx"
#define XCM_EDID_KEY_GREENy               "greeny"
#define XCM_EDID_KEY_BLUEx                "bluex"
#define XCM_EDID_KEY_BLUEy                "bluey"
#define XCM_EDID_KEY_WHITEy               "whitex"
#define XCM_EDID_KEY_WHITEx               "whitey"
#define XCM_EDID_KEY_GAMMA                "gamma"
#define XCM_EDID_KEY_WEEK                 "week"
#define XCM_EDID_KEY_YEAR                 "year"
#define XCM_EDID_KEY_MNFT_ID              "mnft_id"
#define XCM_EDID_KEY_MODEL_ID             "model_id"
#define XCM_EDID_KEY_MNFT                 "mnft"
#define XCM_EDID_KEY_MANUFACTURER         "manufacturer"

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* XCM_EDID_PARSE_H */

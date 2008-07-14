
#ifndef __XCOLOR_H__
#define __XCOLOR_H__

#include <stdint.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>


/**
 * data structures
 */

typedef struct {
	uuid_t uuid;      /* UUID of the profile			*/
	uint32_t size;    /* number of bytes following			*/
} XColorProfile;

typedef struct {
	uint32_t region;  /* XserverRegion				*/
	uuid_t uuid;      /* UUID of the associated profile		*/
} XColorRegion;


/**
 * helper functions
 */

static inline XColorProfile *XColorProfileNext(XColorProfile *profile)
{
	unsigned char *ptr = (unsigned char *) profile;
	return (XColorProfile *) (ptr + sizeof(XColorProfile) + ntohl(profile->size));
}

static inline unsigned long XColorProfileCount(void *data, unsigned long nBytes)
{
	unsigned long count = 0;

	for (XColorProfile *ptr = data; (void *) ptr < data + nBytes; ptr = XColorProfileNext(ptr))
		++count;

	return count;
}

static inline XColorRegion *XColorRegionNext(XColorRegion *region)
{
	unsigned char *ptr = (unsigned char *) region;
	return (XColorRegion *) (ptr + sizeof(XColorRegion));
}

static inline unsigned long XColorRegionCount(void *data, unsigned long nBytes)
{
	return nBytes / sizeof(XColorRegion);
}

#endif /* __XCOLOR_H__ */


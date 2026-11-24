/* SkyLight getters the scripting-addition payload reads through out-parameters. They describe one
 * fixed, ordinary window (800x600 at 100,100, untransformed, opaque) so the handlers see defined
 * values; every other SkyLight/CoreFoundation call is a zero-returning stub from mayhem/build.sh. */
#include "apple_stubs.h"

CGError SLSGetWindowBounds(int cid, uint32_t wid, CGRect *frame)
{ (void)cid; (void)wid; *frame = CGRectMake(100, 100, 800, 600); return kCGErrorSuccess; }

CGError SLSGetWindowTransform(int cid, uint32_t wid, CGAffineTransform *t)
{ (void)cid; (void)wid; CGAffineTransform id = {1, 0, 0, 1, -100, -100}; *t = id; return kCGErrorSuccess; }

CGError SLSGetWindowAlpha(int cid, uint32_t wid, float *alpha)
{ (void)cid; (void)wid; *alpha = 1.0f; return kCGErrorSuccess; }

CGError SLSGetWindowOwner(int cid, uint32_t wid, int *window_cid)
{ (void)cid; (void)wid; *window_cid = 0; return kCGErrorSuccess; }

CGError SLSGetConnectionPSN(int cid, ProcessSerialNumber *psn)
{ (void)cid; psn->highLongOfPSN = 0; psn->lowLongOfPSN = 0; return kCGErrorSuccess; }

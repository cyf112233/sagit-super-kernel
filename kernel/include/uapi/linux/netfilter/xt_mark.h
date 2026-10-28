#ifndef _XT_MARK_H_target
#define _XT_MARK_H_target

#include <linux/types.h>

struct xt_mark_tginfo2 {
	__u32 mark, mask;
};

#endif /*_XT_MARK_H_target */

#ifndef _XT_MARK_H_match
#define _XT_MARK_H_match

#include <linux/types.h>

struct xt_mark_mtinfo1 {
	__u32 mark, mask;
	__u8 invert;
};

#endif /*_XT_MARK_H_match */

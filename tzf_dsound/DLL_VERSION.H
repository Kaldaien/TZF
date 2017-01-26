#pragma once


#define TZF_MAJOR 1
#define TZF_MINOR 8
#define TZF_BUILD 1
#define TZF_REV   0





#define _A2(a)     #a
#define  _A(a)  _A2(a)
#define _L2(w)  L ## w
#define  _L(w) _L2(w)


#if (TZF_REV > 0)
#define TZF_VERSION_STR_A    _A(TZF_MAJOR) "." _A(TZF_MINOR) "." _A(TZF_BUILD) "." _A(TZF_REV)
#else
#define TZF_VERSION_STR_A    _A(TZF_MAJOR) "." _A(TZF_MINOR) "." _A(TZF_BUILD)
#endif

#define TZF_VERSION_STR_W _L(TZF_VERSION_STR_A)


#define TZF_FILE_VERSION     TZF_MAJOR,TZF_MINOR,TZF_BUILD,TZF_REV
#define TZF_PRODUCT_VERSION  TZF_MAJOR,TZF_MINOR,TZF_BUILD,TZF_REV

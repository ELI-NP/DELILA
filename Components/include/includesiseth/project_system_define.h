

#define LINUX    // set if MAC_OSX, also  else WIN
//#define MAC_OSX



// don't change below

#ifdef MAC_OSX
	#define LINUX
#endif

#ifndef LINUX
	#define WIN
 	#define WINDOWS
#endif

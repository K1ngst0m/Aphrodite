// EntryPoint.h

// engine's entry point:
// - init logging system
// - create application
// - run the game circle


#ifndef Aphrodite_ENTRYPOINT_H
#define Aphrodite_ENTRYPOINT_H

#include "Application.h"
#include "Base.h"

#ifdef APH_PLATFORM_LINUX
int main(int argc, char **argv);
#else
#error NOT_SUPPORTING_PLATFORM
#endif

#endif

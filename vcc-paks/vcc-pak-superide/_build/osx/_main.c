#include "xDebug.h"

/*
 Called when dylib is loaded
 */
__attribute__((constructor)) static void initializer()
{
    XTRACE("SuperIDE .dylib initialization\n");
}

/*
 called when dlib is unloaded
 */
__attribute__((destructor)) static void finalizer()
{
    XTRACE("SuperIDE .dylib destruction\n");
}

/*
 Copyright 2015 by Joseph Forgione
 This file is part of VCC (Virtual Color Computer).
 
 VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 VCC macos UI
*/

/*************************************************************************/
//
//  VCCVMDocument.m
//
// TODO: convert to a package/container type, not just a file
// https://www.cocoanetics.com/2012/09/back-on-mac-os-x-tutorial-for-ios-developers/
// https://www.cocoanetics.com/2012/09/os-x-tutorial-for-ios-developers-2/
//
// TODO: dirty on new (or modified) document
//
/*************************************************************************/

#import "VCCVMDocument.h"

#import "DocumentItem.h"

/*************************************************************************/

#pragma mark -
#pragma mark --- VCC display update callback ---

/*************************************************************************/
/*************************************************************************/

/*************************************************************************/

@implementation VCCVMDocument

@synthesize items = _items;

-(id)init
{
    self = [super init];
    if ( self != nil )
    {
        _saveUrl = nil;
    }
    
    return self;
}

// override
/*
 +(BOOL) usesUbiquitousStorage
{
    return NO;
}
*/

/*************************************************************************/

#pragma mark -
#pragma mark --- class methods ---

/*************************************************************************/

#pragma mark -
#pragma mark --- accessors ---

/*************************************************************************/
/*
    accessors
 */

- (vccinstance_t *)vccInstance
{
    return vccInstance;
}

/*************************************************************************/

#if 0
// Application Support
NSError *error;
NSFileManager *manager = [NSFileManager defaultManager];
NSURL *applicationSupport = [manager URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:false error:&error];
NSString *identifier = [[NSBundle mainBundle] bundleIdentifier];
NSURL *folder = [applicationSupport URLByAppendingPathComponent:identifier];
[manager createDirectoryAtURL:folder withIntermediateDirectories:true attributes:nil error:&error];
NSURL *fileURL = [folder URLByAppendingPathComponent:@"TSPlogfile.txt"];

//NSURL * home = [[NSFileManager defaultManager] homeDirectoryForCurrentUser];

#endif


/*************************************************************************/
/**
    Get the path where to save files with the main config file
 
    Caller must free() the pointer that is passed back
 */
result_t vccGetDocumentPath(emudevice_t * pEmuDev, char ** ppPath)
{
    result_t result = XERROR_GENERIC;
    emurootdevice_t * rootDevice = emuDevGetRootDevice(pEmuDev);
    VCCVMDocument * document = (__bridge VCCVMDocument *)rootDevice->pRootObject;

    if (   pEmuDev != NULL
        && ppPath != NULL
        && document != NULL
        )
    {
        NSURL * url = [document fileURL];
        NSString * path = NULL;
        
        if ( url != NULL)
        {
            path = [url path];
            *ppPath = strdup([path cStringUsingEncoding:NSUTF8StringEncoding]);
        }
        
        //NSURL * url = [document fileURL];
        //NSString * str = [url absoluteString];
        //NSString * rel = [url relativePath];
        //NSString * relStr = [url relativeString];
        
        result = XERROR_NONE;
    }
    
    return result;
}

/**
    Get the path to the app for loading internal modules
 */
result_t vccGetAppPath(emudevice_t * pEmuDev, char ** ppPath)
{
    result_t result = XERROR_GENERIC;
    VCCVMDocument * document = (__bridge VCCVMDocument *)pEmuDev->pRoot->pRootObject;
    
    if (    pEmuDev != NULL
        && ppPath != NULL
        && document != NULL
        )
    {
        NSString * str = [[NSBundle mainBundle].bundlePath stringByDeletingLastPathComponent];
        const char * pstr = [str cStringUsingEncoding:NSUTF8StringEncoding];
        *ppPath = strdup(pstr);
        result = XERROR_NONE;
    }
    
    return result;
}

result_t vccLog(emudevice_t * pEmuDev, const char * pMessage)
{
    result_t result = XERROR_GENERIC;
    VCCVMDocument * document = (__bridge VCCVMDocument *)pEmuDev->pRoot->pRootObject;
    
    if (    pEmuDev != NULL
        && pMessage != NULL
        && document != NULL
        )
    {
        NSString * s = [NSString stringWithUTF8String:pMessage];
        
        NSLog(@"%@\n",s);
        
        result = XERROR_NONE;
    }
    
    return result;
}

/*************************************************************************/
/**
 */
-(bool)initVCCInstance
{
    //
    // initialize VM
    //
    static int  s_iCount = 1;
    char        Name[64];
    
    // TODO: Set name from config?
    // create a unique name for this VM instance
    // this is used to name the thread
    sprintf(Name,"VCC %d",s_iCount++);
    
    // TODO: handle errors starting up - report, etc
    // the vcc instance locks the emulation thread until we release it initially
    if ( vccInstance == NULL )
    {
        vccInstance = vccCreate(Name);
    }
    if ( vccInstance != NULL )
    {
        /*
            Set up callbacks for giving info to sub modules
         */
        
        // Add ourselves (document) as the root object for this instance
        vccInstance->root.pRootObject = (__bridge void *)(self);
        
        vccInstance->root.pfnGetDocumentPath    = vccGetDocumentPath;
        vccInstance->root.pfnGetAppPath         = vccGetAppPath;
        vccInstance->root.pfnLog                = vccLog;

        return true;
    }
    
    return false;
}

/*************************************************************************/
/**
    call when the view is loaded and ready
 */
- (void)startEmulation
{    
    /*
        update the user interface
     */
    if ( vccInstance->pfnUIUpdate != NULL )
    {
        (*vccInstance->pfnUIUpdate)(vccInstance);
    }

    /*
        start the emulation thread - it was locked on creation
     */
    SDL_UnlockMutex(vccInstance->hEmuMutex);
}

/*************************************************************************/

- (void)makeWindowControllers
{
    // Override to return the Storyboard file name of the document.
    [self addWindowController:[[NSStoryboard storyboardWithName:@"Main" bundle:nil] instantiateControllerWithIdentifier:@"Document Window Controller"]];
}

/*************************************************************************/
//
// Save / load
//

#pragma mark -
#pragma mark --- Save / Load ---

/*************************************************************************/

+ (BOOL)autosavesInPlace
{
    return YES;
}

/*************************************************************************/

/**
	TODO: add error handling/reporting
 */
- (BOOL)readFromURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError
{
    NSString * path = [absoluteURL path];
    const char * cstrPath = [path cStringUsingEncoding:NSUTF8StringEncoding];
    
    [self initVCCInstance];
    
    if ( vccLoad(vccInstance,cstrPath) == XERROR_NONE )
    {
        return YES;
    }
    
    return NO;
}

/*************************************************************************/

- (BOOL)writeSafelyToURL:(NSURL *)url ofType:(NSString *)typeName forSaveOperation:(NSSaveOperationType)saveOperation error:(NSError * _Nullable __autoreleasing *)outError
{
    // save real destination
    _saveUrl = url;
    
    return [super writeSafelyToURL:url ofType:typeName forSaveOperation:saveOperation error:outError];
}

- (void)saveToURL:(NSURL *)url ofType:(NSString *)typeName forSaveOperation:(NSSaveOperationType)saveOperation completionHandler:(void (^)(NSError * _Nullable))completionHandler
{
    
    [super saveToURL:url ofType:typeName forSaveOperation:saveOperation completionHandler:completionHandler];
}

/**
	TODO: add error handling/reporting
 */
- (BOOL)writeToURL:(NSURL *)absoluteURL ofType:(NSString *)typeName forSaveOperation:(NSSaveOperationType)saveOperation originalContentsURL:(NSURL *)absoluteOriginalContentsURL error:(NSError **)outError
{
    // lock the emulation thread
    SDL_LockMutex(vccInstance->hEmuMutex);

    NSString * savePath = [absoluteURL relativePath];
    const char * cSavePath = [savePath cStringUsingEncoding:NSUTF8StringEncoding];
    
    NSString * refPath = [_saveUrl path];
    const char * cRefPath = [refPath cStringUsingEncoding:NSUTF8StringEncoding];

    result_t result = vccSave(vccInstance,cSavePath,cRefPath);
    
    // clear temp ref to save location
    _saveUrl = nil;
    
    // release the emulation thread
    SDL_UnlockMutex(vccInstance->hEmuMutex);

    switch ( result )
    {
        case XERROR_NONE:
            break;
            
        default:
            // TODO: set appropriate error with user info
            if ( outError != NULL )
            {
                *outError = [NSError errorWithDomain:NSPOSIXErrorDomain code:1 userInfo:NULL];
            }
            break;
    }
    
    return (result == XERROR_NONE);
}

/**
    Get the modified state of the instance and mark the document's edit status
 */
- (void) updateDocumentDirtyStatus
{
    vccinstance_t * pInstance = [self vccInstance];
    bool dirty = emuDevConfCheckDirty(&pInstance->root.device);
    if ( dirty )
    {
        [self updateChangeCount: NSChangeDone];
    }
    else
    {
        [self updateChangeCount: NSChangeCleared];
    }
}

//- (BOOL)hasUndoManager
//{
//    return NO;
//}


/*************************************************************************/

#if 0
- (BOOL)readFromFileWrapper:(NSFileWrapper *)fileWrapper ofType:(NSString *)typeName error:(NSError **)outError
{
    if (![fileWrapper isDirectory])
    {
        NSDictionary *userInfo = [NSDictionary dictionaryWithObject:@"Illegal Document Format" forKey:NSLocalizedDescriptionKey];
        *outError = [NSError errorWithDomain:@"Shoebox" code:1 userInfo:userInfo];
        return NO;
    }
    
    // store reference for later image access
    _fileWrapper = fileWrapper;
    
    // decode the index
    NSFileWrapper *indexWrapper = [fileWrapper.fileWrappers objectForKey:@"index.plist"];
    NSArray *array = [NSKeyedUnarchiver unarchiveObjectWithData:indexWrapper.regularFileContents];
    
    // set document property for all items
    [array makeObjectsPerformSelector:@selector(setDocument:) withObject:self];
    
    // set the property
    self.items = [array mutableCopy];
    
    // YES because of successful reading
    return YES;
}

- (NSFileWrapper *)fileWrapperOfType:(NSString *)typeName error:(NSError **)outError
{
    // this holds the files to be added
    NSMutableDictionary *files = [NSMutableDictionary dictionary];
    
    // encode the index
    NSData *data = [NSKeyedArchiver archivedDataWithRootObject:self.items];
    NSFileWrapper *indexWrapper = [[NSFileWrapper alloc] initRegularFileWithContents:data];
    
    // add it to the files
    [files setObject:indexWrapper forKey:@"index.plist"];
    
    // copy all other referenced files too
    for (DocumentItem *oneItem in self.items)
    {
        NSString *fileName = oneItem.fileName;
        NSFileWrapper *existingFile = [_fileWrapper.fileWrappers objectForKey:fileName];
        [files setObject:existingFile forKey:fileName];
    }
    
    // create a new fileWrapper for the bundle
    NSFileWrapper *newWrapper = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:files];
    
    return newWrapper;
}
#endif

/*************************************************************************/

@end


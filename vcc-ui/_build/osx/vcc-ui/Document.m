//
//  Document.m
//
// test - implementing a file wrapper
//

#import "Document.h"
#import "DocumentItem.h"

@implementation Document
{
    NSFileWrapper *_fileWrapper;
    NSMutableArray *_items;
}

- (id)init
{
    self = [super init];
    if (self)
    {
        // start out with an empty array
        _items = [[NSMutableArray alloc] init];
        
        // start with an empty file wrapper
        //_fileWrapper = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:nil];
    }
    return self;
}

- (NSString *)windowNibName
{
    return @"Document";
}

- (void)windowControllerDidLoadNib:(NSWindowController *)aController
{
    [super windowControllerDidLoadNib:aController];
}

+ (BOOL)autosavesInPlace
{
    return YES;
}

// below this line our own code starts ---

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

@synthesize items = _items;

@end


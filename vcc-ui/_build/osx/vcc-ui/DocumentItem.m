//
//  DocumentItem.m
//
// test - implementing a file wrapper
//

#import "DocumentItem.h"
#import "Document.h"

@implementation DocumentItem

//- (NSImage *)thumbnailImage
//{
//    return [self.document thumbnailImageForName:self.fileName];
//}

@synthesize fileName;
@synthesize document;

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super init];
    
    if (self)
    {
        self.fileName = [aDecoder decodeObjectForKey:@"fileName"];
    }
    
    return self;
}

- (void)encodeWithCoder:(NSCoder *)aCoder
{
    [aCoder encodeObject:self.fileName forKey:@"fileName"];
}

@end


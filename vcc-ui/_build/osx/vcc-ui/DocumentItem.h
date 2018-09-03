//
//  DocumentItem.h
//

#import <Cocoa/Cocoa.h>

@class Document;

@interface DocumentItem : NSObject

@property (nonatomic, strong) NSString *fileName;
@property (nonatomic, weak) Document *document;

//- (NSImage *)thumbnailImage;

@end


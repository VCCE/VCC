//
//  VCCVMDocumentController.m
//  vcc
//
//  Created by Wes Gale on 2018-09-05.
//  Copyright © 2018 CoCo3. All rights reserved.
//

#import "VCCVMDocumentController.h"

@implementation VCCVMDocumentController

+ (void) load
{
    [VCCVMDocumentController new];
}

-(void)openDocumentWithContentsOfURL:(NSURL *)url
                             display:(BOOL)displayDocument
                   completionHandler:(void (^)(NSDocument * _Nullable, BOOL, NSError * _Nullable))completionHandler
{
    [super openDocumentWithContentsOfURL:url display:displayDocument completionHandler:completionHandler];
}

-(void)reopenDocumentForURL:(NSURL *)urlOrNil withContentsOfURL:(NSURL *)contentsURL display:(BOOL)displayDocument completionHandler:(void (^)(NSDocument * _Nullable, BOOL, NSError * _Nullable))completionHandler
{
    [super reopenDocumentForURL:urlOrNil withContentsOfURL:contentsURL display:displayDocument completionHandler:completionHandler];
}

+ (void)restoreWindowWithIdentifier:(NSUserInterfaceItemIdentifier)identifier state:(NSCoder *)state completionHandler:(void (^)(NSWindow * _Nullable, NSError * _Nullable))completionHandler
{
    [super restoreWindowWithIdentifier:identifier state:state completionHandler:completionHandler];
}
/* // deprecated
 -(id)openDocumentWithContentsOfURL:(NSURL *)url display:(BOOL)displayDocument error:(NSError * _Nullable __autoreleasing *)outError
{
    
}
*/
@end

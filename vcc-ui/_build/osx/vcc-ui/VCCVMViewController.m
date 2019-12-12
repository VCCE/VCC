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

/////////////////////////////////////////////////////////////////////////////////////
//
// VCCVMViewController
//
// Initializes the VCC instance
// Handles input to the view (mouse and keyboard) and passes it to the vcc instance
// Handles update of the video
// Handles update of the user interface
//
/////////////////////////////////////////////////////////////////////////////////////

#import "VCCVMViewController.h"
#import "VCCVMWindowController.h"
#import "VCCVMDocument.h"
#import "AppDelegate.h"

#include "Vcc.h"
#include "keyboard.h"

/*************************************************************************/

@implementation OKeyInfo

-(id)init:(const VCCKeyInfo *)keyInfo
{
    if ( (self = [super init]) != NULL )
    {
        self.keyInfo = keyInfo;
    }
    
    return self;
}

@end

/*************************************************************************/
/*************************************************************************/

#pragma mark -
#pragma mark ---VCC instance UI update callback implementation ---

/*************************************************************************/

/**
 */
void displayLockCB(void * pParam)
{
    VCCVMViewController * viewController    = (__bridge VCCVMViewController *)pParam;

    SDL_LockMutex(viewController.mutex);
}

/**
 */
void displayUnlockCB(void * pParam)
{
    VCCVMViewController * viewController    = (__bridge VCCVMViewController *)pParam;
    
    SDL_UnlockMutex(viewController.mutex);
}

/**
    The display flip callback is called when an emulator frame is done
    being rendered.  This is not called on a 'skipped' frame.

    this function makes the view dirty so the latest emulated frame will
    be displayed.  The update is dispatched by the Cocoa framework when
    it thinks it is time.
 */
void displayFlipCB(void * pParam)
{
    VCCVMViewController * viewController    = (__bridge VCCVMViewController *)pParam;
    
    // hold onto this for the dispatch
    CFBridgingRetain(viewController);
    
    /// get a snapshot of this frame from the emulator
    //[[viewController emulatorView] captureScreen:vccInstance];
    
    //NSLog(@"displayFlipCB");
    
    // trigger redraw of the view on the main thread
    dispatch_async(dispatch_get_main_queue(), ^{
    //dispatch_sync(dispatch_get_main_queue(), ^{
        
        vccinstance_t * vccInstance = viewController.vccInstance;
        if ( vccInstance != NULL )
        {
            VCCVMView * view = [viewController emulatorView];
            NSRect rctViewBounds = [view bounds];
            [view setNeedsDisplayInRect: rctViewBounds];

            // send one mouse event per frame to the instance
            {
                mouseevent_t mouseEvent;
                
                INIT_MOUSEEVENT(&mouseEvent,eEventMouseMove);
                
                mouseEvent.width    = rctViewBounds.size.width;
                mouseEvent.height   = rctViewBounds.size.height;
                mouseEvent.x        = viewController.ptMouse.x;
                mouseEvent.y        = viewController.ptMouse.y;
                
                emuRootDevDispatchEvent(&vccInstance->root, &mouseEvent.event);
            }
            
            // update status text
            [viewController.statusText setStringValue:[NSString stringWithCString:vccInstance->cStatusText encoding:NSASCIIStringEncoding]];
            
            // TODO: should not be done in the main thread dispatch - move out or have key queue in vcc-core
            // paste text handling
            [viewController handlePasteNextCharacter];
        }
        
        // release now that main queue dispatch is done
        CFBridgingRelease((__bridge CFTypeRef _Nullable)(viewController));
    });
}

/**
    Full screen toggle callback
 */
void displayFullScreenCB(void * pParam)
{
    VCCVMViewController * viewController    = (__bridge VCCVMViewController *)pParam;
    VCCVMDocument * document = [viewController document];
    //vccinstance_t * vccInstance = viewController.vccInstance;

    // go through and find the VCCVMWindowController and tell it to toggle full screen
    NSArray< NSWindowController *> * windowControllers = [document windowControllers];
    for (int i=0; i<windowControllers.count; i++)
    {
        NSWindowController * controller = windowControllers[i];
        
        if ( [controller isKindOfClass:[VCCVMWindowController class]] )
        {
            [[controller window] toggleFullScreen:viewController];
        }
    }
}

/*********************************************************************************/
/**
    emulation device enumeration callback for re-creating the UI elements each
    device needs.  Mostly this is used to re-create the Virtual Machine menu
 */
result_t updateUIEnumCB(emudevice_t * pEmuDev, void * pUser)
{
    //result_t            errResult;
    emudevice_t *        pParentDevice;
    
    ASSERT_EMUDEV(pEmuDev);
    
    //NSLog(@"updateUIEnumCB: %s",pEmuDev->Name);
    
    /* does this device want a menu? */
    if ( pEmuDev->pfnCreateMenu != NULL )
    {
        // caller has destroyed any previous existing menu
        pEmuDev->hMenu = NULL;
        
        // let the device create its own menu
        /* result_t errResult = */ (*pEmuDev->pfnCreateMenu)(pEmuDev);
        // this assertion was removed since we don't really care if the device tacks their items onto another menu
        //assert(pEmuDev->hMenu != NULL);
        
        /*
         add device menu to its parent menu if it exists
         (walk up parents until we find a menu)
         */
        pParentDevice = pEmuDev->pParent;
        while (    pParentDevice != NULL
               && pParentDevice->hMenu == NULL
               )
        {
            pParentDevice = pParentDevice->pParent;
        }
        if (    pParentDevice != NULL
            && pParentDevice->hMenu != NULL
            && pEmuDev->hMenu != NULL
            )
        {
            menuAddSubMenu(pParentDevice->hMenu,pEmuDev->hMenu);
        }
    }
    
    return XERROR_NONE;
}

/*********************************************************************************/
/*
 remove current VM instance menu from the main application menu
 */

#define VIRTUAL_MACHINE_MENU_NAME @"Virtual Machine"

// TODO: move to AppDelegate

void unhookVMInstanceMenu(vccinstance_t * pInstance)
{
    NSApplication *        pApp;
    NSMenu *            pMainMenu;
    NSMenuItem *        pVMMenuItem;
    
    // get application
    pApp = [NSApplication sharedApplication];
    
    // get main menu
    pMainMenu = [pApp mainMenu];
    
    // look for virtual machine menu
    pVMMenuItem = [pMainMenu itemWithTitle:VIRTUAL_MACHINE_MENU_NAME];
    assert(pVMMenuItem != nil && "main menu has no virtual machine menu item");
    
    /* unhook the menu from the main menu */
    [pMainMenu setSubmenu:nil forItem:pVMMenuItem];
}

/*
 add the current VM instance menu to the main application menu
 under the 'Virtual Machine' menu
 */
void hookVMInstanceMenu(vccinstance_t * pInstance)
{
    NSApplication *        pApp;
    NSMenu *            pMainMenu;
    NSMenuItem *        pVMMenuItem;
    NSMenu *            pMenu;
    
    // get application
    pApp = [NSApplication sharedApplication];
    
    // get main menu
    pMainMenu = [pApp mainMenu];
    
    // look for virtual machine menu
    pVMMenuItem = [pMainMenu itemWithTitle:VIRTUAL_MACHINE_MENU_NAME];
    assert(pVMMenuItem != nil && "main menu has no virtual machine menu item");
    
    /* hook up the VM instance menu into main menu */
    pMenu = (__bridge NSMenu *)pInstance->root.device.hMenu;
    [pMainMenu setSubmenu:pMenu forItem:pVMMenuItem];
}

/*********************************************************************************/
/**
 Update user interface callback
 
 Called from VCC instance
 
 update menu items, toolbar items, etc for this instance
 This should only be called when devices, etc change
 */
void updateInterfaceCB(vccinstance_t * pInstance)
{
    if ( pInstance != NULL )
    {
        hmenu_t     hMenu;
        NSMenu *    pMenu;
        
        // grab current menu pointer for destruction
        hMenu = pInstance->root.device.hMenu;
        
        // remove current instance menu from the main menu
        unhookVMInstanceMenu(pInstance);
        
        /*
         re-create the entire VM menu for this instance
         */
        emuDevEnumerate(&pInstance->root.device,updateUIEnumCB,NULL /* pDoc */);
        
        /*
            iterate items in new menu
            and set action/selector to
            the pApp delegate menuAction method
         */
        pMenu = (__bridge NSMenu *)pInstance->root.device.hMenu;
        assert(pMenu != NULL);
        setMenuItemsAction(pMenu);
        
        // add this instance menu back into the main menu
        hookVMInstanceMenu(pInstance);
        
        /* destroy previous VM instance menu */
        if ( hMenu != nil )
        {
            menuDestroy(hMenu);
        }
        
        VCCVMDocument * document = (__bridge VCCVMDocument *)pInstance->root.pRootObject;
        [document updateDocumentDirtyStatus];
    }
}

/**
 */
void screenShotCB(vccinstance_t * pInstance)
{
    NSBitmapImageRep * screenCopy;
    screen_t * pScreen =pInstance->pCoco3->pScreen;
    
    //
    // Capture
    //
    screenCopy = [[NSBitmapImageRep alloc]
                  initWithBitmapDataPlanes:(unsigned char **)&pScreen->surface.pBuffer
                  pixelsWide:pScreen->surface.width
                  pixelsHigh:pScreen->surface.height
                  bitsPerSample:8
                  samplesPerPixel:4
                  hasAlpha:YES
                  isPlanar:NO
                  colorSpaceName:@"NSDeviceRGBColorSpace" //@"NSCalibratedRGBColorSpace" //
                  bitmapFormat:0 //NSAlphaFirstBitmapFormat
                  bytesPerRow:pScreen->surface.linePitch
                  bitsPerPixel:32
                  ];
    
    //
    // Crop it
    //
    //int videoWidth  = pScreen->PixelsperLine;
    //int videoHeight = pScreen->LinesperScreen;    
    // set up texture coordinates based on amount of the texture used for the current video mode
    //float h = (float)videoWidth;
    //float v = (float)videoHeight;
    float h = (float)pScreen->surface.width;
    float v = (float)pScreen->surface.height;

    CGRect rect = CGRectMake(0,0,h,v);
    CGImageRef cgImg = CGImageCreateWithImageInRect([screenCopy CGImage],
                                                    rect);
    NSBitmapImageRep *result = [[NSBitmapImageRep alloc]
                                initWithCGImage:cgImg];
    CGImageRelease(cgImg);

    //
    // save it
    //
    char * path = sysGetSavePathnameFromUser(NULL, NULL);
    if ( path != NULL )
    {
        NSData * image = [result TIFFRepresentation];
        NSString * nstrPath = [NSString stringWithUTF8String:path];
        [image writeToFile:nstrPath atomically:YES];
    }
}

/*************************************************************************/
/*************************************************************************/

#pragma mark -
#pragma mark -- VCCVMViewController implementation --

/*************************************************************************/

/**
 */
@implementation VCCVMViewController
{
    CGFloat originalStatusBarConstraintConstant;
}

#pragma mark -- actions --

/**
 */
- (bool)toggleStatuBarVisible
{
    if ( self.constraintStatusBarHeight.constant == 0 )
    {
        // show status bar
        self.constraintStatusBarHeight.constant = originalStatusBarConstraintConstant;
    }
    else
    {
        // hide status bar
        self.constraintStatusBarHeight.constant = 0;
    }
    
    return (self.constraintStatusBarHeight.constant != 0);
}

#pragma mark -- accessors --

/**
    Return a reference to the document (that holds the vcc instance)
 */
- (VCCVMDocument *)document
{
    NSView * view = [self view];
    VCCVMDocument * doc = view.window.windowController.document;
    return doc;
}

#pragma mark -- VCCVMViewController --

- (void)viewDidLoad
{
    [super viewDidLoad];

    // Do any additional setup after loading the view.
    pastedCharacters = [[NSMutableArray alloc] init];
}

- (void)viewWillAppear
{
    // save original statusbar height
    originalStatusBarConstraintConstant = self.constraintStatusBarHeight.constant;
    // hide status bar initially
    // TODO: this causes a constraint error the first time
    //self.constraintStatusBarHeight.constant = 0;

    // hide the tool bar initially
    [self.view.window.toolbar setVisible:NO];

    // TODO: set state of toolbar and status bar menu items

    // we have to turn on the mouse move events from the system explicitly
    [self.view.window setAcceptsMouseMovedEvents:YES];
    
    // TODO: Update layout so emulator view is optimal size (no scaling)

    // set up to accept drag operations in the emulator view
    NSArray * types = [NSArray arrayWithObjects:
                       /* NSColorPboardType, */ NSFilenamesPboardType, nil];
    [self.emulatorView registerForDraggedTypes:types];
    
    [self.emulatorView setDelegate:self];

    m_uModifiers = 0;

    //
    // Set up emulator
    //
    VCCVMDocument * document = [self document];
    if ( [document initVCCInstance] )
    {
        self.vccInstance = document.vccInstance;
        
        //
        // initialize display update callbacks
        //
        // SDL won't let us name the mutex
        self.mutex = SDL_CreateMutex(); //("display");
        self.vccInstance->pCoco3->pScreen->pScreenCallbackParam = (__bridge void *)self;
        self.vccInstance->pCoco3->pScreen->pScreenFlipCallback  = displayFlipCB;
        self.vccInstance->pCoco3->pScreen->pScreenLockCallback  = displayLockCB;
        self.vccInstance->pCoco3->pScreen->pScreenUnlockCallback  = displayUnlockCB;
        self.vccInstance->pCoco3->pScreen->pScreenFullScreenCallback  = displayFullScreenCB;

        //
        // set emulator callback for updating the UI
        // because we need to do the actual work, when it
        // tells us to
        //
        self.vccInstance->pfnUIUpdate = updateInterfaceCB;
        self.vccInstance->pfnScreenShot = screenShotCB;

        // TODO: set state of toolbar and status bar based on conf setting and update layout

        [document startEmulation];
    }
}

- (void)viewWillDisappear
{
    [super viewWillDisappear];
    
    if ( self.vccInstance != NULL )
    {
        SDL_LockMutex(self.vccInstance->hEmuMutex);
        self.vccInstance->EmuThreadState = emuThreadStop;
        SDL_UnlockMutex(self.vccInstance->hEmuMutex);
    }
    
    // get rid of our reference, the document will destroy it
    self.vccInstance = nil;
}

#pragma mark -
#pragma mark -- VCCVMViewDelegate protocol (Drag and Drop) --

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender {
    NSPasteboard *pboard;
    NSDragOperation sourceDragMask;
    
    sourceDragMask = [sender draggingSourceOperationMask];
    pboard = [sender draggingPasteboard];
    
    if ( [[pboard types] containsObject:NSFilenamesPboardType] ) {
        NSArray * files = [pboard propertyListForType:NSFilenamesPboardType];
        
        // for now, only one file at a time
        if ( files.count == 1 )
        {
            // TODO: Check all accepted file types from any given device and pass it down for handling?
            // for now, that file must be a ROM
            const char * path = [files[0] cStringUsingEncoding:NSUTF8StringEncoding];
            filetype_e type = sysGetFileType(path);
            if ( type == COCO_PAK_ROM )
            {
                if (sourceDragMask & NSDragOperationLink) {
                    return NSDragOperationLink;
                } else if (sourceDragMask & NSDragOperationCopy) {
                    return NSDragOperationCopy;
                }
            }
        }
    }
    
    return NSDragOperationNone;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender {
    NSPasteboard *pboard;
    NSDragOperation sourceDragMask;
    
    sourceDragMask = [sender draggingSourceOperationMask];
    pboard = [sender draggingPasteboard];
    
    if ( [[pboard types] containsObject:NSFilenamesPboardType] )
    {
        NSArray * files = [pboard propertyListForType:NSFilenamesPboardType];
        
        // TODO: we know it is a ROM
        // TODO: dialog to ask if they are sure (if there is a pak already)
        //
        const char * path = [files[0] cStringUsingEncoding:NSUTF8StringEncoding];
        vccLoadPak(_vccInstance,path);
        
        // Depending on the dragging source and modifier keys,
        // the file data may be copied or linked
        if (sourceDragMask & NSDragOperationLink) {
            //[self addLinkToFiles:files];
            NSLog(@"Drop for link : %@",files);
        } else {
            //[self addDataFromFiles:files];
            NSLog(@"Drop for copy : %@",files);
        }
    }
    
    return YES;
}

/*
 #if 0 // example off of stack overflow for using dragImage
 - (void)mouseDown:(NSEvent *)theEvent {
 // Create an offset for the offset parameter. Since it is ignored, you should just use 0,0.
 NSSize dragOffset = NSMakeSize(0.0, 0.0);
 NSPasteboard *pboard;
 
 // Get the pasteboard used to hold drag-and-drop data
 pboard = [NSPasteboard pasteboardWithName:NSDragPboard];
 // Declare the data type used on the pasteboard. This sample passes a TIFF representation of an image
 [pboard declareTypes:[NSArray arrayWithObject:NSTIFFPboardType]  owner:self];
 // Add the data to the pasteboard.
 [pboard setData:[[self image] TIFFRepresentation] forType:NSTIFFPboardType];
 // Call dragImage:... with:
 // An image representing the object you are dragging. This could be a file's icon or a screenshot of a view, etc.
 [self dragImage:[self image]
 // The starting location of the image in this views coordinates. This should be close to the location of the object you are dragging.
 at:[self imageLocation]
 // An NSSize structure. This is not used.
 offset:dragOffset
 // The mousedown event
 event:theEvent
 // The pasteboard which contains the data
 pasteboard:pboard
 // The object providing the data
 source:self
 // Whether or not the image should slide to its starting location if the drag isn't completed.
 slideBack:YES];
 
 }
 
 #endif
 */


/*************************************************************************/

#pragma mark -
#pragma mark --- View dispatched event handling ---

/*************************************************************************/
/*
    special/function keys should be configurable
 */
- (bool)handleEmulatorFunctionKeys:(unichar)keyChar
{
    bool handled = false;
    
    switch ( keyChar )
    {
            // Info bar/band toggle
        case NSF3FunctionKey:
            // SetInfoBand( !SetInfoBand(QUERY) );
            // InvalidateBorder();
            break;
            
// monitor type toggle
//        case NSF4FunctionKey:
//             screenEmuDevCommand(&m_pInstance->pCoco3->pScreen->device, SCREEN_COMMAND_MONTYPE);
//             break;

            // soft reset
        case NSF5FunctionKey:
            vccEmuDevCommand(&_vccInstance->root.device,VCC_COMMAND_RESET,0);
            handled = true;
            break;
            
            // hard reset
        case NSF6FunctionKey:
            vccEmuDevCommand(&_vccInstance->root.device,VCC_COMMAND_POWERCYCLE,0);
            handled = true;
            break;
            
            // throttle toggle
        case NSF7FunctionKey:
            cc3EmuDevCommand(&_vccInstance->pCoco3->machine.device,COCO3_COMMAND_THROTTLE,0);
            handled = true;
            break;
            
// full screen toggle
//         case NSF11FunctionKey:
//            screenEmuDevCommand(&m_pInstance->pCoco3->pScreen->device,SCREEN_COMMAND_FULLSCREEN);
//            break;
    }
    
    return handled;
}

/*************************************************************************/
/*
 key down
 
 certian keys are for emulator control, the rest are passed to the emulator
 */
- (void)handleKeyDown:(NSEvent *)theEvent
{
    if (   _vccInstance == NULL
        || _vccInstance->pCoco3 == NULL
        )
    {
        return;
    }
    
    if ( _vccInstance->pCoco3->pKeyboard == NULL )
    {
        emuDevLog(&_vccInstance->root.device, "Keyboard object is NULL!");
        return;
    }
    
    NSString *        pChars;
    unichar            keyChar = 0;
    
    /*
        handle special emulator keys
     */
    pChars = [theEvent characters];
    keyChar = [pChars characterAtIndex:0];
    if ( ! [self handleEmulatorFunctionKeys:keyChar] )
    {
        /*
         emulate the key
         */
        //pChars = [theEvent characters];
        pChars = [theEvent charactersIgnoringModifiers];
        
        if ( [pChars length] == 0 )
        {
            // reject dead keys
            return;
        }
        
        if ( [pChars length] == 1 )
        {
            keyChar = [pChars characterAtIndex:0];
            
            //NSLog(@"Down: %@ - %d",pChars,keyChar);
            
            //
            // translate and pass key to the emulator
            //
            if (   _vccInstance != NULL
                && _vccInstance->EmuThreadState == emuThreadRunning
                )
            {
                keyboardEvent(_vccInstance->pCoco3->pKeyboard, keyboardTranslateSystemToCCVK(keyChar), eEventKeyDown);
            }
        }
        else
        {
            NSLog(@"Multiple characters sent on key down - not handled");
        }
    }
}

/*************************************************************************/
/*
 key down
 
 certian keys are for emulator control, the rest are passed to the emulator
 */
- (void)handleKeyUp:(NSEvent *)theEvent
{
    if (   _vccInstance == NULL
        || _vccInstance->pCoco3 == NULL
        )
    {
        return;
    }
    
    if ( _vccInstance->pCoco3->pKeyboard == NULL )
    {
        emuDevLog(&_vccInstance->root.device, "Keyboard object is NULL!");
        return;
    }
    
    NSString *        pChars;
    //unichar        keyCode;
    unichar            keyChar = 0;
    
    //keyCode        = [theEvent keyCode];
    
    /*
     emulate the key
     */
    //pChars = [theEvent characters];
    pChars = [theEvent charactersIgnoringModifiers];
    
    if ( [pChars length] == 0 )
    {
        // reject dead keys
        return;
    }
    
    if ( [pChars length] == 1 )
    {
        keyChar = [pChars characterAtIndex:0];
        
        //NSLog(@"Up: %@ - %d",pChars,keyChar);
        
        //
        // translate and pass key to the emulator
        //
        if (   _vccInstance != NULL
            && _vccInstance->EmuThreadState == emuThreadRunning
            )
        {
            keyboardEvent(_vccInstance->pCoco3->pKeyboard, keyboardTranslateSystemToCCVK(keyChar), eEventKeyUp);
        }
    }
    else
    {
        NSLog(@"Multiple characters sent on key up - not handled");
    }
}

/****************************************************************************/

- (void)handleFlagsChanged:(NSEvent *)theEvent
{
    if (    _vccInstance == NULL
        || _vccInstance->pCoco3 == NULL
        )
    {
        return;
    }
    
    if ( _vccInstance->pCoco3->pKeyboard == NULL )
    {
        emuDevLog(&_vccInstance->root.device, "Keyboard object is NULL!");
        return;
    }
    
    NSUInteger        modifiers;
    NSUInteger        changed;
    
    // get current key flags
    modifiers    = [theEvent modifierFlags];
    
    // get which ones have changed since last event
    changed = modifiers ^ m_uModifiers;
    
    if ( (changed & NSAlphaShiftKeyMask) != 0 )
    {
        // Caps lock
        
        if ( (modifiers & NSAlphaShiftKeyMask) != 0 )
        {
            //NSLog(@"NSAlphaShiftKeyMask - down");
            
            keyboardEvent(_vccInstance->pCoco3->pKeyboard, CCVK_CAPS, eEventKeyDown);
        }
        else
        {
            //NSLog(@"NSAlphaShiftKeyMask - up");
            
            keyboardEvent(_vccInstance->pCoco3->pKeyboard, CCVK_CAPS, eEventKeyUp);
        }
    }
    
    if ( (changed & NSShiftKeyMask) != 0 )
    {
        if ( (modifiers & NSShiftKeyMask) != 0 )
        {
            //NSLog(@"NSShiftKeyMask - down");
            
            keyboardEvent(_vccInstance->pCoco3->pKeyboard, CCVK_SHIFT, eEventKeyDown);
        }
        else
        {
            //NSLog(@"NSShiftKeyMask - up");
            
            keyboardEvent(_vccInstance->pCoco3->pKeyboard, CCVK_SHIFT, eEventKeyUp);
        }
    }
    
    if ( (changed & NSControlKeyMask) != 0 )
    {
        if ( (modifiers & NSControlKeyMask) != 0 )
        {
            //NSLog(@"NSControlKeyMask - down");
            
            keyboardEvent(_vccInstance->pCoco3->pKeyboard, CCVK_CTRL, eEventKeyDown);
        }
        else
        {
            //NSLog(@"NSControlKeyMask - up");
            
            keyboardEvent(_vccInstance->pCoco3->pKeyboard, CCVK_CTRL, eEventKeyUp);
        }
    }
    
    if ( (changed & NSAlternateKeyMask) != 0 )
    {
        if ( (modifiers & NSAlternateKeyMask) != 0 )
        {
            //NSLog(@"NSAlternateKeyMask - down");
            
            keyboardEvent(_vccInstance->pCoco3->pKeyboard, CCVK_ALT, eEventKeyDown);
        }
        else
        {
            //NSLog(@"NSAlternateKeyMask - up");
            
            keyboardEvent(_vccInstance->pCoco3->pKeyboard, CCVK_ALT, eEventKeyUp);
        }
    }
    
    /*
     if ( (modifiers & NSCommandKeyMask) )
     {
        NSLog(@"NSCommandKeyMask");
     }
     
     if ( (modifiers & NSNumericPadKeyMask) )
     {
        // Set if any key in the numeric keypad is pressed
        NSLog(@"NSNumericPadKeyMask");
     }
     
     if ( (modifiers & NSHelpKeyMask) )
     {
        NSLog(@"NSHelpKeyMask");
     }
     
     if ( (modifiers & NSFunctionKeyMask) )
     {
        // Set if any function key is pressed. The function keys include the F keys at the top of most keyboards
        // (F1, F2, and so on) and the navigation keys in the center of most keyboards (Help, Forward Delete, Home, End, Page Up, Page Down, and the arrow keys).
        NSLog(@"NSFunctionKeyMask");
     }
     */
    
    // save current state of keys
    m_uModifiers = modifiers;
}

/*************************************************************************/
/**
    translate an event (this is sent from the view) into an action
    for the emulator instance
 */
- (void)translateEvent:(NSEvent *)theEvent
{
    if (    _vccInstance == NULL
         || _vccInstance->pCoco3 == NULL
       )
    {
        return;
    }
    
    switch ( [theEvent type] )
    {
        case NSFlagsChanged:
            [self handleFlagsChanged:theEvent];
        break;
        case NSKeyDown:
            [self handleKeyDown:theEvent];
        break;
        case NSKeyUp:
            [self handleKeyUp:theEvent];
        break;
        
        default:
        break;
    }

    if ( _vccInstance->pCoco3->pJoystick != NULL )
    {
        mouseevent_t    mouseEvent;
        
        INIT_MOUSEEVENT(&mouseEvent, eEventMouseButton);        
        mouseEvent.button1 = -1;
        mouseEvent.button2 = -1;
        mouseEvent.button3 = -1;

        switch ( [theEvent type] )
        {
            case NSLeftMouseDown:
                mouseEvent.button1 = true;
                break;
            case NSRightMouseDown:
                mouseEvent.button3 = true;
                break;
            case NSOtherMouseDown:
                mouseEvent.button2 = true;
                break;
            
            case NSLeftMouseUp:
                mouseEvent.button1 = false;
                break;
            case NSRightMouseUp:
                mouseEvent.button3 = false;
                break;
            case NSOtherMouseUp:
                mouseEvent.button2 = false;
                break;
            
            case NSEventTypeMouseEntered:
                // TODO: handle mouse entered view
                break;
            case NSEventTypeMouseExited:
                // TODO: handle mouse left view
                break;
                
            //case NSMouseMoved:
            case NSEventTypeMouseMoved:
            {
                // save mouse move position
                self.ptMouse = [theEvent locationInWindow];
            }
            break;
            
            default:
            break;
        }
        
        // send button status change event?
        if (    mouseEvent.button1 != -1
             || mouseEvent.button2 != -1
             || mouseEvent.button3 != -1
            )
        {
            emuRootDevDispatchEvent(&_vccInstance->root, &mouseEvent.event);
        }
    }
}

#pragma mark -
#pragma mark --- First Responsder ---

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    return YES;
}

- (BOOL)resignFirstResponder
{
    return YES;
}

#pragma mark -
#pragma mark --- Keyboard event dispatch ---

- (void)keyDown:(NSEvent *)theEvent
{
    [self translateEvent:theEvent];
}

- (void)keyUp:(NSEvent *)theEvent
{
    [self translateEvent:theEvent];
}

- (void)flagsChanged:(NSEvent *)theEvent
{
    [self translateEvent:theEvent];
}

#pragma mark -
#pragma mark --- Mouse event dispatch ---

- (void)mouseDown:(NSEvent *)theEvent
{
    [self translateEvent:theEvent];
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
    [self translateEvent:theEvent];
}

- (void)mouseUp:(NSEvent *)theEvent
{
    [self translateEvent:theEvent];
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
    [self translateEvent:theEvent];
}

- (void)mouseMoved:(NSEvent *)theEvent
{
    [self translateEvent:theEvent];
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
    [self translateEvent:theEvent];
}

- (void)otherMouseUp:(NSEvent *)theEvent
{
    [self translateEvent:theEvent];
}

#pragma mark -
#pragma mark --- Paste text ---

- (IBAction)paste:sender
{
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *classArray = [NSArray arrayWithObject:[NSString class]];
    NSDictionary *options = [NSDictionary dictionary];
    
    BOOL ok = [pasteboard canReadObjectForClasses:classArray options:options];
    if (ok) {
        NSArray *objectsToPaste = [pasteboard readObjectsForClasses:classArray options:options];
        NSString * text = [objectsToPaste objectAtIndex:0];
        
        // convert pasted text to coco keys
        const char * cstr = [text UTF8String];
        for (int i=0; i<strlen(cstr); i++)
        {
            char c = cstr[i];
            
            // CR/LF
            if ( c == 0x0a || c == 0x0d )
            {
                c = CCVK_ENTER;
                
                if ( i<strlen(cstr) )
                {
                    char next = cstr[i+1];
                    if ( next == 0x0a || next == 0x0d )
                    {
                        i++;
                    }
                }
            }
            
            const VCCKeyInfo * keyBoardInfo = keyboardGetKeyInfoForCCVK(c);
            if ( keyBoardInfo != NULL )
            {
                OKeyInfo * keyInfo = [[OKeyInfo alloc] init:keyBoardInfo];
            
                [pastedCharacters addObject:keyInfo];
            }
            else{
                NSLog(@"Unknown pasted character : %c", c);
            }
        }
        
        pasteState = 0;
    }
}

//
// TODO: This is not quite working correctly not running on the vcc emulation thread.  It should push the keys into a queue in vcc-core and it should handle how/when they are applied
//
// handle any pending characters to paste via the keyboard
//
// Each iteration we handle one step for each character.  at minimum it is
//
//  0. key down
//  1. delay
//  2. key up
//  3. delay
//
// Some characters require two keys to be down to translate the character to a key press
// in that case, it does:
//
//  0. key 1 down
//  1. delay
//  2. key 2 down
//  3. delay
//  4. key 2 up
//  5. delay
//  6. key 1 up
//  7. delay
//
- (void) handlePasteNextCharacter
{
    if (    pastedCharacters != NULL
         && [pastedCharacters count] > 0
        )
    {
        switch ( pasteState )
        {
            // scan code 1 - key down
            case 0:
            {
                const OKeyInfo * keyInfo = [pastedCharacters objectAtIndex:0];
                keyboardEvent(_vccInstance->pCoco3->pKeyboard, keyInfo.keyInfo->ScanCode1, eEventKeyDown);

//                if ( keyInfo.keyInfo->ScanCode2 == 0 )
//                {
//                    // skip scan code 2
//                    pasteState = 5;
//                }
//                else
                {
                    pasteState++;
                }
            }
            break;
                
            case 1:
            {
                pasteState++;
            }
            break;
              
            // scan code 2 - key down
            case 2:
            {
                const OKeyInfo * keyInfo = [pastedCharacters objectAtIndex:0];
                if ( keyInfo.keyInfo->ScanCode2 != 0 )
                {
                    keyboardEvent(_vccInstance->pCoco3->pKeyboard, keyInfo.keyInfo->ScanCode2, eEventKeyDown);
                }
                
                pasteState++;
            }
            
            // delay
            case 3:
            {
                pasteState++;
            }
            break;
                
            // scan code 2 key-up
            case 4:
            {
                const OKeyInfo * keyInfo = [pastedCharacters objectAtIndex:0];
                if ( keyInfo.keyInfo->ScanCode2 != 0 )
                {
                    keyboardEvent(_vccInstance->pCoco3->pKeyboard, keyInfo.keyInfo->ScanCode2, eEventKeyUp);
                }
                
                pasteState++;
            }
            break;

            // delay
            case 5:
            {
                pasteState++;
            }
            break;
                
            // scan code 1 key-up
            case 6:
            {
                const OKeyInfo * keyInfo = [pastedCharacters objectAtIndex:0];
                
                keyboardEvent(_vccInstance->pCoco3->pKeyboard, keyInfo.keyInfo->ScanCode1, eEventKeyUp);
                
                [pastedCharacters removeObjectAtIndex:0];
                
                pasteState++;
            }
            break;
                
            case 7:
            {
                pasteState = 0;
            }
            break;
        }
    }
}

/*************************************************************************/

@end

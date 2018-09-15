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

//
// TODO: Maintain aspect ratio and keep entire view visible when resizing window
//

#import "VCCVMWindowController.h"

#include "screen.h"

@interface VCCVMWindowController ()

@end

@implementation VCCVMWindowController

- (VCCVMDocument *)vmDocument
{
    return (VCCVMDocument *)[self document];
}

- (void)windowWillLoad
{
    [super windowWillLoad];
    
    // [NSStoryboard mainStoryboard]
}

- (void)windowDidLoad
{
    [super windowDidLoad];
    
    // register for Window becoming main events
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(aWindowBecameMain:)
                                                 name:NSWindowDidBecomeMainNotification object:nil];
}

// window switch
- (void)aWindowBecameMain:(NSNotification *)notification {
    
    NSWindow * theWindow = [notification object];
    
    // if the notification is for us
    if ( theWindow == self.window )
    {
        // update the menu
        vccinstance_t * vccInstance = [[self document] vccInstance];
        if ( vccInstance != NULL )
        {
            vccUpdateUI(vccInstance);
            [[self document] updateDocumentDirtyStatus];
        }
    }
}
    
- (void)awakeFromNib
{
    // hide the tool bar initially
    //[self.window.toolbar setVisible:NO];

    //hacky way to get an autosave to generate an NSPersistentStore.
    NSDocument *doc = self.document;
    [doc updateChangeCount:NSChangeDone];
    [doc autosaveDocumentWithDelegate:self didAutosaveSelector:@selector(document:didAutosave:contextInfo:) contextInfo:nil];
    
}

//called by the autosave operation started in awakeFromNib.
- (void)document:(NSDocument *)document didAutosave:(BOOL)didAutosaveSuccessfully contextInfo:(void *)contextInfo
{
    NSDocument *doc = self.document;
    [doc updateChangeCount:NSChangeUndone];
}

// https://developer.apple.com/library/content/documentation/General/Conceptual/MOSXAppProgrammingGuide/FullScreenApp/FullScreenApp.html

//
- (void)updateVccFullScreenStatus
{
    VCCVMDocument * pDocument = [self vmDocument];
    vccinstance_t *  vccinstance = [pDocument vccInstance];
    
    screen_t * screen = (screen_t *)emuDevFindByDeviceID(&vccinstance->root.device, VCC_SCREEN_ID);
    ASSERT_SCREEN(screen);
    
    //int state;
    //emuDevGetCommandState(&screen->device,SCREEN_COMMAND_FULLSCREEN,&state);
    emuDevSendCommand(&screen->device,SCREEN_COMMAND_FULLSCREEN,COMMAND_STATE_ON);
    // TODO: set full screen config status of screen
    // get screen device
    // set its full screen status
    // if it is not set to full screen, set it
}

// NSWindow delegate

- (NSApplicationPresentationOptions)window:(NSWindow *)window willUseFullScreenPresentationOptions:(NSApplicationPresentationOptions)proposedOptions
{
    return (NSApplicationPresentationFullScreen |
            NSApplicationPresentationHideDock |
            NSApplicationPresentationAutoHideMenuBar |
            NSApplicationPresentationAutoHideToolbar);
}

- (void)windowWillEnterFullScreen:(NSNotification *)notification
{
    [self updateVccFullScreenStatus];
}

/**
    Notification the screen is switching full screen state
 */
- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
    [self updateVccFullScreenStatus];
}

- (void)windowWillExitFullScreen:(NSNotification *)notification
{
    [self updateVccFullScreenStatus];
}

/**
 Notification the screen is switching full screen state
 */
- (void)windowDidExitFullScreen:(NSNotification *)notification
{
    [self updateVccFullScreenStatus];
}

@end

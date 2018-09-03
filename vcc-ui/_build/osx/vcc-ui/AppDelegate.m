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

/*********************************************************************************/
//
// VCC macos UI
//
// AppDelegate.m
//
// Handles interaction with the menu and toolbar
//
/*********************************************************************************/
//
// TODO: Menu handling might make more sense in the Window controller?
//
/*********************************************************************************/

#import "AppDelegate.h"

#import "VCCVMViewController.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Insert code here to initialize your application
    
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}

/******************************************************************************************************/

/**
    Get the current  (with focus)VCC instance Window
 */
- (NSWindow *)currentWindow
{
    NSWindow * pWindow = [[NSApp sharedApplication] keyWindow];
    return pWindow;
}

/**
    Get the current (with focus) VCC instance Document
 */
- (VCCVMDocument *)currentDocument
{
    VCCVMDocument * pDocument = [[NSDocumentController sharedDocumentController] currentDocument];
    return pDocument;
}

/**
 Get the current (with focus) VCC instance
 */
- (vccinstance_t *)currentInstance
{
    VCCVMDocument *        pDocument       = [self currentDocument];
    vccinstance_t *        pInstance       = [pDocument vccInstance];
    
    return pInstance;
}

/******************************************************************************************************/

#pragma mark -
#pragma mark --- NSMenu validation protocol ---

/*
 - (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)anItem
 {
 return YES; //[super validateUserInterfaceItem:anItem];
 }
 */

/**
     Validate menu items (enable/disable)
 
     We look for emulator control items and set their state. all others
     are handled by the Cocoa framework.
 
     All menu items that we handle will have an action of our menuAction method
     and have a 'tag' which is the emulator command ID
 */
- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    vccinstance_t *         pInstance       = [self currentInstance];
    BOOL                    valid           = NO;
    SEL                     action;
    int32_t                 iState;
    
    // get the target action of the menu item
    action = [menuItem action];
    
    // validate
    if (   action == @selector(menuAction:)
        || action == @selector(toolbarAction:)
        )
    {
        // default to no state for menu item
        iState = COMMAND_STATE_NONE;
        
        // validate the command
        valid = vccCommandValidate(pInstance,(int32_t)[menuItem tag],&iState);
        
        // set desired menu item state
        menuItemSetState((__bridge hmenuitem_t)(menuItem),iState);
    }
    else
    {
        /* let Cocoa decide */
        valid = [[NSDocumentController sharedDocumentController] validateMenuItem:menuItem];
    }
    
    return valid;
}

/******************************************************************************************************/

#pragma mark -
#pragma mark --- Global menu handling ---

/**
    This is the default action for all menu items used as emulator command & control
 
    All menu items that we handle will have an action of this method
    and have a 'tag' which is the emulator command ID
 */
- (IBAction)menuAction:(id)sender
{
    NSMenuItem *        pMenuItem       = (NSMenuItem *)sender;
    vccinstance_t *     pInstance       = [self currentInstance];
    NSInteger           tag;
    
    tag = [pMenuItem tag];
    
#if (defined _DEBUG)
    NSInteger           tagHigh;
    NSInteger           tagLow;

    // for reference only
    tagHigh = ((tag & 0x7FFF0000) >> 16);    // device specific commands for menu items created dynamically
    tagLow    = ((tag & 0x0000FFFF));            // emulator commands as defined in Vcc.h
    
    NSLog(@"menuAction recevied: %ld",(long)tag);
#endif
    
    vccCommand(pInstance,(int32_t)tag);
}

/******************************************************************************************************/

#pragma mark -
#pragma mark --- action handlers ---

- (IBAction)statusbarAction:(id)sender
{
    NSWindow * pWindow = [self currentWindow];

    VCCVMViewController * controller = (VCCVMViewController *)pWindow.contentViewController;
    
    bool status = [controller toggleStatuBarVisible];
    if ( status )
    {
        self.statusbarMenuItem.title = @"Hide Status Bar";
    }
    else
    {
        self.statusbarMenuItem.title = @"Show Status Bar";
    }
}

- (IBAction)toolbarAction:(id)sender
{
    NSToolbarItem *     pMenuItem     = (NSToolbarItem *)sender;
    vccinstance_t *     pInstance       = [self currentInstance];
    NSInteger           tag;
#if (defined _DEBUG)
    NSInteger           tagHigh;
    NSInteger           tagLow;
#endif
    
    tag = [pMenuItem tag];
    
#if (defined _DEBUG)
    // for reference only
    tagHigh = ((tag & 0x7FFF0000) >> 16);    // device specific commands for menu items created dynamically
    tagLow  = ((tag & 0x0000FFFF));            // emulator commands as defined in Vcc.h
    
    NSLog(@"toolbarAction recevied: %ld",(long)tag);
#endif
    
    vccCommand(pInstance,(int32_t)tag);
}

@end

/*********************************************************************************/
/**
    walks the menu and sub menus, and sets the target/selector to the
    application delegate/menuAction method for all menu items
 */
void setMenuItemsAction(NSMenu * pMenu)
{
    NSApplication *        pApp;
    SEL                    method;
    int32_t                x;
    NSMenuItem *           pMenuItem;
    
    // get application (for delegate) and menuAction selector
    pApp = [NSApplication sharedApplication];
    
    // get selector for app delegate method
    method = @selector(menuAction:);
    
    // go through all menu items in this menu
    for (x=0; x<[pMenu numberOfItems]; x++)
    {
        // get the current menu item
        pMenuItem = [pMenu itemAtIndex:x];
        
        if ( [pMenuItem submenu] != nil )
        {
            // it is a sub menu, descend
            setMenuItemsAction([pMenuItem submenu]);
        }
        else
        {
            // set target/action for this item
            [pMenuItem setTarget:[pApp delegate]];
            [pMenuItem setAction:method];
        }
    }
}

/*********************************************************************************/

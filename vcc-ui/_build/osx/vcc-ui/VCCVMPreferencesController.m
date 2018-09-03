/************************************************************************/
/*
 Copyright (c) 2008 Matthew Ball - http://www.mattballdesign.com
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
 */
/************************************************************************/

#import "VCCVMPreferencesController.h"

#include "xDebug.h"

/************************************************************************/

NSString * VCCVMPreferencesSelectionAutosaveKey = @"VCCVMPreferencesSelection";

/************************************************************************/

@interface VCCVMPreferencesController (Private)
- (void)_setupToolbar;
- (void)_selectModule:(NSToolbarItem *)sender;
- (void)_changeToModule:(id<VCCVMPreferencesModule>)module;
@end

/************************************************************************/

@implementation VCCVMPreferencesController

/************************************************************************/

#pragma mark -
#pragma mark Property Synthesis

//@synthesize modules = _modules;

/************************************************************************/

#pragma mark -
#pragma mark Life Cycle

/************************************************************************/

//static VCCVMPreferencesController * sharedPreferencesController = nil;

/************************************************************************/

- (id)init
{
	if (self = [super init]) 
	{
		NSWindow * prefsWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 300, 200) styleMask:(NSTitledWindowMask | NSClosableWindowMask) backing:NSBackingStoreBuffered defer:YES];
		[prefsWindow setShowsToolbarButton:NO];
		self.window = prefsWindow;
		//[prefsWindow release];
		
		[self.window setDelegate:self];
		
		[self _setupToolbar];
	}
	
	return self;
}

/************************************************************************/

- (void)dealloc
{
	self.modules = nil;
	//[super dealloc];
}

/************************************************************************/

#if 0
 + (VCCVMPreferencesController *)sharedController
{
	@synchronized(self) 
	{
		if (sharedPreferencesController == nil) 
		{
			//NSLog(@"PreferenceController ---> building a shared preference controller");
			[[self alloc] init]; // assignment not done here
		}
	}
	
	return sharedPreferencesController;
}

+ (id)allocWithZone:(NSZone *)zone
{
	@synchronized(self) 
	{
		if (sharedPreferencesController == nil) 
		{
			sharedPreferencesController = [super allocWithZone:zone];
			
			return sharedPreferencesController;
		}
	}
	return nil; // on subsequent allocation attempts return nil
}

- (id)copyWithZone:(NSZone *)zone
{
	return self;
}

- (id)retain
{
	return self;
}

- (NSUInteger)retainCount
{
	return UINT_MAX; // denotes an object that cannot be released
}

- (void)release
{
	// do nothing
}

- (id)autorelease
{
	return self;
}
#endif

/************************************************************************/
 
#pragma mark -
#pragma mark VCCVMPreferencesController accessors

/************************************************************************/

/**
 * @brief       Set current vm instance this control panel will work with
 * @details     Set current vm instance this control panel will work with
 */
 - (void)setInstance:(vccinstance_t *)pInstance
{
	pVCCInstance = pInstance;
}

/************************************************************************/

#pragma mark -
#pragma mark NSWindowController Subclass

/************************************************************************/

- (void)showWindow:(id)sender
{
	assert(pVCCInstance != nil);
	assert(_currentModule != nil);
	
#if 0
	// capture current settings
	// add 'settings' to respond to on close of preferences
	assert(0);
	
	hConf = confCreate(NULL);
	assert(hConf != NULL);
	
	// save entire device tree to temp config object
	emuDevConfSave(&m_pInstance->device,hConf);
	
	// use conf object in preference panes
	
	// TODO: do on close/apply
	// apply configuration, make document dirty
	
	/* destroy the config object */
	//verify( confDestroy(hConf) == XERROR_NONE );
	
#endif
	
	// set instance
	[_currentModule setInstance:pVCCInstance];
	
	// tell the pane it will be shown
	[_currentModule willBeDisplayed];
	 
	// lock the Emu thread
	//vccEmuLock(pVCCInstance);

	//NSLog(@"PreferenceController ---> top");
	[self.window center];
	//NSLog(@"PreferenceController ---> top2");
	[super showWindow:sender];
	//NSLog(@"PreferenceController ---> top3");
}

/************************************************************************/

#pragma mark -
#pragma mark NSWindowDelegate protocol

/************************************************************************/

- (void)windowWillClose:(NSNotification *)notification
{
	// TODO: Called when the user closes the window, update settings here

	// check if anything has changed
	// TODO: warns user if reboot is required
	// make document dirty
	
	//[[VCCVMPreferencesController sharedController] setInstance:nil];
	
	// unlock/resume the Emu thread
	//vccEmuUnlock(pVCCInstance);
}

/************************************************************************/

#pragma mark -
#pragma mark NSToolbar

/************************************************************************/

- (void)_setupToolbar
{
	NSToolbar * toolbar = [[NSToolbar alloc] initWithIdentifier:@"VCCVMPreferencesToolbar"];
	[toolbar setDisplayMode:NSToolbarDisplayModeIconAndLabel];
	[toolbar setAllowsUserCustomization:NO];
	[toolbar setDelegate:self];
	[toolbar setAutosavesConfiguration:NO];
	[self.window setToolbar:toolbar];
}

- (NSArray *)toolbarAllowedItemIdentifiers:(NSToolbar *)toolbar
{
	NSMutableArray *identifiers = [NSMutableArray array];
	for (id<VCCVMPreferencesModule> module in self.modules)
	{
		[identifiers addObject:[module identifier]];
	}
	
	return identifiers;
}

- (NSArray *)toolbarDefaultItemIdentifiers:(NSToolbar *)toolbar
{
	// We start off with no items. 
	// Add them when we set the modules
	return [[NSArray alloc] init];
}

#pragma mark NSToolbarDelegate protocol

- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar itemForItemIdentifier:(NSString *)itemIdentifier willBeInsertedIntoToolbar:(BOOL)flag
{
	id<VCCVMPreferencesModule> module = [self moduleForIdentifier:itemIdentifier];
	
	NSToolbarItem *item = [[NSToolbarItem alloc] initWithItemIdentifier:itemIdentifier];
	if (!module)
		return item;
	
	
	[item setLabel:[module title]];
	[item setImage:[module image]];
	[item setTarget:self];
	[item setAction:@selector(_selectModule:)];
	return item;
}

- (NSArray *)toolbarSelectableItemIdentifiers:(NSToolbar *)toolbar
{
	return [self toolbarAllowedItemIdentifiers:toolbar];
}

- (void)toolbarWillAddItem:(NSNotification *)notification
{
	//assert(0);
}

- (void)toolbarDidRemoveItem:(NSNotification *)notification
{
	//assert(0);
}

#pragma mark -
#pragma mark Modules

- (id<VCCVMPreferencesModule>)moduleForIdentifier:(NSString *)identifier
{
	for (id<VCCVMPreferencesModule> module in self.modules) {
		if ([[module identifier] isEqualToString:identifier]) {
			return module;
		}
	}
	return nil;
}

- (NSArray *)modules
{
    return _modules;
}

- (void)setModules:(NSArray *)newModules
{
	if ( _modules )
    {
		_modules = nil;
	}
	
	if ( newModules != _modules ) 
	{
		_modules = newModules;
		
		// Reset the toolbar items
		NSToolbar *toolbar = [self.window toolbar];
		if (toolbar) {
			NSInteger index = [[toolbar items] count]-1;
			while (index > 0) {
				[toolbar removeItemAtIndex:index];
				index--;
			}
			
			// Add the new items
			for (id<VCCVMPreferencesModule> module in self.modules)
			{
				[toolbar insertItemWithItemIdentifier:[module identifier] atIndex:[[toolbar items] count]];
			}
		}
		
		// Change to the correct module
		if ( [self.modules count] ) 
		{
			id<VCCVMPreferencesModule> defaultModule = nil;
			
			// Check the autosave info
			NSString * savedIdentifier = [[NSUserDefaults standardUserDefaults] stringForKey:VCCVMPreferencesSelectionAutosaveKey];
			
			defaultModule = [self moduleForIdentifier:savedIdentifier];
			
			if ( !defaultModule ) 
			{
				defaultModule = [self.modules objectAtIndex:0];
			}
			
			[self _changeToModule:defaultModule];
		}
	}
}

- (void)_selectModule:(NSToolbarItem *)sender
{
	if ( ![sender isKindOfClass:[NSToolbarItem class]] )
		return;
	
	id<VCCVMPreferencesModule> module = [self moduleForIdentifier:[sender itemIdentifier]];
	if ( !module )
		return;
	
	[self _changeToModule:module];
}

- (void)_changeToModule:(id<VCCVMPreferencesModule>)module
{
	[[_currentModule view] removeFromSuperview];
	
	NSView *newView = [module view];
	
	// Resize the window
	NSRect newWindowFrame = [self.window frameRectForContentRect:[newView frame]];
	newWindowFrame.origin = [self.window frame].origin;
	newWindowFrame.origin.y -= newWindowFrame.size.height - [self.window frame].size.height;
	[self.window setFrame:newWindowFrame display:YES animate:YES];
	
	[[self.window toolbar] setSelectedItemIdentifier:[module identifier]];
	[self.window setTitle:[module title]];
	
	if ([(NSObject *)module respondsToSelector:@selector(willBeDisplayed)]) 
	{
		[module setInstance:pVCCInstance];
		
		[module willBeDisplayed];
	}
	
	_currentModule = module;
	[[self.window contentView] addSubview:[_currentModule view]];
	
	// Autosave the selection
	[[NSUserDefaults standardUserDefaults] setObject:[module identifier] forKey:VCCVMPreferencesSelectionAutosaveKey];
}

@end

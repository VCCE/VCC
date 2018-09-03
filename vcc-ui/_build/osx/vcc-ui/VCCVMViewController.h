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

#import <Cocoa/Cocoa.h>

#import "VCCVMView.h"
#import "VCCVMDocument.h"

#include "keyboard.h"

/**
 */
@interface OKeyInfo : NSObject
{
}

@property () const VCCKeyInfo * keyInfo;

-(id)init:(const VCCKeyInfo *)info;

@end

/**
 */
@interface VCCVMViewController : NSViewController<VCCVMViewDelegate>
{
    NSUInteger  m_uModifiers;            // save state of function/control key state
    
    int                             pasteState;
    NSMutableArray<OKeyInfo *> *    pastedCharacters;
}

@property (weak,nonatomic) IBOutlet NSLayoutConstraint *    constraintStatusBarHeight;

@property (weak,nonatomic) IBOutlet VCCVMView *     emulatorView;
@property (weak,nonatomic) IBOutlet NSView *        statusBarView;
@property (weak,nonatomic) IBOutlet NSTextField *   statusText;
@property (weak,nonatomic) IBOutlet NSView *        indicatorView;

@property (assign,nonatomic) vccinstance_t *        vccInstance;
@property (assign,nonatomic) handle_t               mutex;
@property () NSPoint                                ptMouse;


- (VCCVMDocument *) document;
- (void) handlePasteNextCharacter;
- (bool)toggleStatuBarVisible;

@end


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
// VCCVMView - very simple OpenGL view for the CoCo 'screen'
//
// This view is created when the document is loaded and its window is made, containing
// the VCC view for displaying the emulator screen/video output using an NSOpenGLView.
//
// TODO: update to modern OpenGL API
// TODO: add support for shaders
// TODO: artifact colour shader?
// TODO: CRT shader - general and/or to show scan lines
//
/////////////////////////////////////////////////////////////////////////////////////

#import "VCCVMView.h"
#import "VCCVMDocument.h"

#include "Vcc.h"
#include "tcc1014graphics.h"

#include <OpenGL/gl.h>

/////////////////////////////////////////////////////////////////////////////////////

@implementation VCCVMView

@synthesize delegate;

#pragma mark -
#pragma mark --- Accessors ---

/////////////////////////////////////////////////////////////////////////////////////

- (VCCVMDocument *)document
{
    return _window.windowController.document;
}

/////////////////////////////////////////////////////////////////////////////////////

#pragma mark -
#pragma mark --- Cocoa ---


#pragma mark -
#pragma mark --- Rendering ---

/**
	Tell Cocoa we cover, nothing to draw underneath
 */
- (BOOL) isOpaque
{
	return YES;
}

/**
	Tell Cocoa we do not want default clipping
 */
- (BOOL) wantsDefaultClipping
{
	return NO;
}

/*
    Set up the OpenGL texture we will put the video display into
 
    This should only be called once
 */
- (void) setupTexture
{
    if ( [[self document] vccInstance] == NULL )
    {
        return;
    }
    
    screen_t * cocoScreen = [[self document] vccInstance]->pCoco3->pScreen;
    if ( cocoScreen != NULL)
    {
        glGenTextures (1, &texture);
        
        glBindTexture(GL_TEXTURE_2D, texture);

        // TODO: power of 2 texture?
        
        // Create a texture
        glTexImage2D(
             GL_TEXTURE_2D,
             0,
             GL_RGBA,
             cocoScreen->surface.width,
             cocoScreen->surface.height,
             0,
             GL_RGBA,
             GL_UNSIGNED_BYTE,
             (GLvoid*)cocoScreen->surface.pBuffer
        );
        
        // Set up the texture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        
        // Enable textures
        glEnable(GL_TEXTURE_2D);
    }
}

/**
	called by the Cocoa framework when an area of the view is dirty.
	
	currently we ignore the dirty rect and redraw the entire view
	since we invalidate the entire view each update
*/
- (void)drawRect:(NSRect)pRect
{
    if ( [[self document] vccInstance] == NULL )
    {
        return;
    }
    
    screen_t * cocoScreen = [[self document] vccInstance]->pCoco3->pScreen;
    if ( cocoScreen != NULL )
    {
        screenLock(cocoScreen);
        
        // one-time set up of the OpenGL texture
        if ( texture == 0 )
        {
            [self setupTexture];
        }
        
        int videoWidth  = cocoScreen->surface.width;
        int videoHeight = cocoScreen->surface.height;

        // set up texture coordinates based on amount of the texture used for the current video mode
        float th = (float)videoWidth/(float)cocoScreen->surface.width;
        float tv = (float)videoHeight/(float)cocoScreen->surface.height;
        
        glBindTexture(GL_TEXTURE_2D, texture);
        
        // Update texture of CoCo video display
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            cocoScreen->surface.width,
            cocoScreen->surface.height,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            (GLvoid*)cocoScreen->surface.pBuffer
        );
        
        screenUnlock(cocoScreen);

        // Enable textures
        glEnable(GL_TEXTURE_2D);
        
        // set up vertex coordinates
        // originally based on video mode / vertical/horizontal positioning and whether there is a border, this is no longer done, it just shows the entire screen now
        float xl  = -((float)(cocoScreen->surface.width)/(float)cocoScreen->surface.width);
        float yt  = -((float)(cocoScreen->surface.height)/(float)cocoScreen->surface.height);
        float xr = ((float)(cocoScreen->surface.width)/(float)cocoScreen->surface.width);
        float yb = ((float)(cocoScreen->surface.height)/(float)cocoScreen->surface.height);
        
        // set up the quad to put the screen texture on
        glBegin( GL_QUADS );
            glTexCoord2d(0.0, tv);      glVertex2f(xl,yt);
            glTexCoord2d(th, tv);       glVertex2f(xr,yt);
            glTexCoord2d(th, 0.0);      glVertex2f(xr,yb);
            glTexCoord2d(0.0, 0.0);     glVertex2f(xl,yb);
        glEnd();
        
        glFlush();
    }
} // end drawRect

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
#pragma mark Drag & Drop (from Finder)

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender {
    if ( delegate )
    {
        return [delegate draggingEntered:sender];
    }

    return NSDragOperationNone;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    if ( delegate )
    {
        return [delegate performDragOperation:sender];
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////

#pragma mark -
#pragma mark end of VCCVMView implementation

@end

/////////////////////////////////////////////////////////////////////////////////////

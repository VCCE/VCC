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

//
// events sent to emulator from the system
//

#ifndef event_h
#define event_h

#include "slinklist.h"

/* forward declaration */
typedef struct emudevice_t emudevice_t;

/**
    Main event types
 */
typedef enum event_e
{
    eEventReset = 0,
    
    eEventDevice,
    
    eEventConf,
    eEventLoad,
    eEventSave,
    
    eEventKey,
    eEventMouse,
    eEventJoystick,
    
    eEventCount
} event_e;

/**
    Base event
 */
typedef struct event_t
{
    event_e type;
    
} event_t;

/**
    Event listener
 */
typedef struct eventlistener_t
{
    slink_t         link;
    
    event_e         type;
    emudevice_t *   pDevice;
} eventlistener_t;

/**
    Config event sub types
    device get/set config value
 */
typedef enum confevent_e
{
    eEventConfGet = 0,
    eEventConfSet
} confevent_e;

/**
 */
typedef struct confevent_t
{
    event_t     event;
    
    confevent_e subType;
    
    int         value;
} confevent_t;

/**
 */
typedef struct deviceevent_t
{
    event_t         event;

    emudevice_t *   source; // sender
    id_t            target; // target device ID
    
    int             param;  // generic event parameters
    int             param2;
} deviceevent_t;

#if (defined DEBUG)
#   define ASSERT_CONFEVENT(e)     assert((e)->type == eEventConf)
#else
#   define ASSERT_CONFEVENT(e)
#endif
#define CAST_CONFEVENT(e)       (confevent_t *)(e)
#define INIT_CONFEVENT(e,t)     (e)->event.type = eEventConf; \
                                (e)->subType = (t); \
                                (e)->value=0

/**
    Key event sub types
 */
typedef enum keyevent_e
{
    eEventKeyDown = 0,
    eEventKeyUp
} keyevent_e;

/**
 */
typedef struct keyevent_t
{
    event_t event;          /**< base */
    
    keyevent_e  subType;    /**< Key event sub-type */
    
    int ccvKey;             /**< CoCo virtual key */
} keyevent_t;

#if (defined DEBUG)
#   define ASSERT_KEYEVENT(e)      assert((e)->type == eEventKey)
#else
#   define ASSERT_KEYEVENT(e)
#endif
#define CAST_KEYEVENT(e)        (keyevent_t *)(e)
#define INIT_KEYEVENT(e,t)      (e)->event.type = eEventKey; \
                                (e)->subType = (t); \
                                (e)->ccvKey=0

typedef enum mouseevent_e
{
    eEventMouseMove = 0,
    eEventMouseButton,
    eEventMouseEnter,
    eEventMouseLeave
} mouseevent_e;

/**
    Mouse event from system
 */
typedef struct mouseevent_t
{
    event_t         event;
    
    mouseevent_e    subType;
    
    int     device;     /**< device number */
    
    int     width;      /**< width of view */
    int     height;     /**< height of view */
    
    int     x;          /**< x position of mouse in view */
    int     y;          /**< */
    
    int    button1;     /**< button 1 down flag */
    int    button2;     /**< button 2 down flag */
    int    button3;     /**< button 3 down flag */
} mouseevent_t ;

#if (defined DEBUG)
#   define ASSERT_MOUSEEVENT(e)    assert((e)->type == eEventMouse)
#else
#   define ASSERT_MOUSEEVENT(e)
#endif
#define CAST_MOUSEEVENT(e)      (mouseevent_t *)(e)
#define INIT_MOUSEEVENT(e,t)    (e)->event.type = eEventMouse; \
                                (e)->subType = (t); \
                                (e)->width=0; \
                                (e)->height=0; \
                                (e)->x=0; \
                                (e)->y=0; \
                                (e)->button1 = -1; \
                                (e)->button2 = -1; \
                                (e)->button3 = -1

#endif /* event_h */

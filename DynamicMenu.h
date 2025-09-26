#pragma once

// MenuItem is an array of 100 menu items.
struct DynamicMenuItem
{
	char MenuName[512];
	int MenuId;
	int Type;
};

// Menu IDs
//  MID_BEGIN  must be used to begin the menu
//  MID_FINISH must be used to finish the menu
//  5002 thru 5099 are standalone or slave entries.
//  6000 thru 6100 are dynamic entries.
constexpr auto MID_BEGIN     = 0;
constexpr auto MID_FINISH    = 1;
constexpr auto MID_ENTRY     = 6000;
constexpr auto MID_SDYNAMENU = 5000;
constexpr auto MID_EDYNAMENU = 5100;

// Menu item types
enum MenuItemType
{
	MIT_Head,
	MIT_Slave,
	MIT_StandAlone,
	MIT_Seperator
};

// Callback pointer for CallDynamicMenu() defined in pakinterface.c
typedef void (*DYNAMICMENUCALLBACK)(const char * MenuName,int MenuId, int MenuType);

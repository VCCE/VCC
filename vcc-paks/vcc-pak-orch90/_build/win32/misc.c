/*********************************************************************/

#if 0
typedef void (*SETCART)(unsigned char);
typedef void (*SETCARTPOINTER)(SETCART);
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);

static HINSTANCE g_hinstDLL=NULL;
static void (*PakSetCart)(unsigned char)=NULL;

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
	if (fdwReason == DLL_PROCESS_DETACH ) //Clean Up 
	{
		//Put shutdown procs here
		return(1);
	}
	g_hinstDLL=hinstDLL;
	return(1);
}

extern "C" 
{          
	__declspec(dllexport) void ModuleName(char *ModName,char *CatNumber,DYNAMICMENUCALLBACK Temp)
	{
		LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
		LoadString(g_hinstDLL,IDS_CATNUMBER,CatNumber, MAX_LOADSTRING);		
		strcpy(ModName,"Orchestra-90");
		return ;
	}
}
#endif


# Compiling The VCC Sources

Currently, VCC is compiled in C/C++ using "Microsoft Visual Studio 2015 Community", which is a free download from the MSDN downloads. When downloading VS2015, you must make sure to get the Win32 and WinXP compatibility packages to ensure build compatibility with older versions of Windows.

VS2015 requires Windows 7 or greater to install. Check the VS2015 "System Requirements" before trying to install on your system.

As of the current build, there are no "extras" needed to compile VCC. This may change at any time as we are trying to eventually move the code to cross-compile for cross-platform use, making VCC available for Mac and Linux users and not just the Windows users.

Compiling any VCC sources other than the "Release" source set is not guaranteed. Any branch other than the release may at any time, be in a state of developement in which it is not compilable as we are constantly updating the sources with changes. We will try to maintain the default branch as the "Release" branch ensuring a "running" emulator. For example; the "Original Masters" branch is a copy of Joseph Forgeone's (VCC's original author) original "VS C++ 6" code that when using that platform, will compile into the equivelent of VCC 1.42. It WILL NOT compile under VS2015. These sources are stored here more for historical purposes than anything else.

We welcome all bug reports and suggestions. Post any bug reports and/or suggestions on the "Issues" page and we will be promptly notified. We do a "check list" of the issues page every-so-often to see if there's any "quick fixes" we can add while we are working on current changes. The VCC Developement Team is a small one and we work on this when we can as we all have lives and families, so VCC is NOT a priority but a hobby. Even being a hobby, it is also a work of love as we also use this software ourselves, so we try to make it as usable as possible. Sometimes progress is slow and it looks like nothing is going on (and it may not be), but usually, there's plenty going on behind the scenes and we have not committed our current work. Progress is slow, but progress is being made.

If you think you have patches or code that could contribute to the VCC project, please contact the developement team and we will see if it fits the current direction of the project, and if so, add it to our code. Be sure to add your name and the date in your comments (and please comment your code well) if you want credit for your work. Also, if your programming skills are up to "snuff" and you feel you would like to join the VCC project, contact us and we will see if you are worthy :-)

We will continue to try to make VCC the best Color Computer Emulator available.

Thank You for using VCC

The VCC Developement Team.

# Instructions 

# 6309
(D) Direct (I) Inherent (R) Relative (M) Immediate (X) Indexed (E) extened

hd6309.c, hd6309defs.h

See The_6309_Book.pdf for instructions information. 
See [Socks 6309][sock6309] page for instructions, timings, and byte value.

| Instruction   | Status	    | Notes	                                            |
|---            |---            |---                                                |
|	ADCD_D 	    |	UNEMULATED	|		                                            |
|	ADCD_E 	    |	UNEMULATED	|		                                            |
|	ADCR   	    |	UNEMULATED	|		                                            |
|	ADDE_D	    |	Untested	| 119B 6309          	                            |
|	ADDE_E	    |	Untested	| 11BB 6309          	                            |
|	ADDE_X	    |	Untested	| 11AB 6309          	                            |
|	ADDF_D	    |	Untested	| 11D8 6309          	                            |
|	ADDF_E	    |	Untested	| 11FB 6309          	                            |
|	ADDF_M	    |	Untested	| 11CB 6309          	                            |
|	ADDF_X	    |	Untested	| 11EB 6309          	                            |
|	ADDR        |   Untested    | Untested Nitros9 1030 6309 CC? NITRO 8 bit code   |
|	ADDW_E	    |	Untested	| 10BB 6309          	                            |
|	ADDW_X	    |	Untested    | TESTED NITRO CHECK no Half carry?	10AB 6309       |
|	AIM_E 	    |	Untested    | CHECK NITRO 72 6309                               |
|	ANDD_D	    |	Untested	| 1094 6309          	                            |
|	ANDD_E	    |	Untested	| 10B4 6309          	                            |
|	ANDR  	    |	Untested    | wcreate		1034 6309                           |
|	ASRD_I	    |	Untested    | TESTED NITRO MULTIVUE		1047 6309          	    |
|	BAND   	    |	UNEMULATED	|		                                            |
|	BEOR   	    |	UNEMULATED	|		                                            |
|	BIAND  	    |	UNEMULATED	|		                                            |
|	BIEOR  	    |	UNEMULATED	|		                                            |
|	BIOR   	    |	UNEMULATED	|		                                            |
|	BITD_D	    |	Untested	| 1095 6309          	                            |
|	BITD_E	    |	Untested    | CHECK NITRO	10B5 6309                           |
|	BITD_M	    |	Untested	| 1085 6309          	                            |
|	BITD_X	    |	Untested	| 10A5 6309          	                            |
|	BOR    	    |	UNEMULATED	|		                                            |
|	CLRF_I	    |	Untested    | wcreate	115F 6309          	                    |
|	CMPE_D	    |	Untested	| 1191 6309          	                            |
|	CMPE_E	    |	Untested	| 11B1 6309          	                            |
|	CMPF_D	    |	Untested	| 11D1 6309          	                            |
|	CMPF_E	    |	Untested	| 11F1 6309          	                            |
|	CMPF_X	    |	Untested	| 11E1 6309          	                            |
|	CMPW_D	    |	Untested	| 1091 6309          	                            |
|	CMPW_E	    |	Untested	| 10B1 6309          	                            |
|	COME_I	    |	Untested	| 1143 6309          	                            |
|	COMF_I	    |	Untested	| 1153 6309          	                            |
|	COMW_I	    |	Untested	| 1053 6309          	                            |
|	DIVD_E	    |	Untested	| 11BD 6309 02292008 	                            |
|	DIVD_M      |   Untested    | Untested Nitros9 118D 6309 NITRO                  |
|	DIVQ_D 	    |	DONE	    |		                                            |
|	DIVQ_E 	    |	DONE	    |		                                            |
|	DIVQ_M 	    |	DONE	    |		                                            |
|	DIVQ_X 	    |	DONE	    |		                                            |
|	EIM_D 	    |	Untested	| 6309	                                            |
|	EIM_E 	    |	Untested    | CHECK NITRO		75 6309                         |
|	EIM_X 	    |	Untested    | TESTED NITRO		65 6309                         |
|	EORD_D	    |	Untested	| 1098 6309          	                            |
|	EORD_E	    |	Untested	| 10B8 6309          	                            |
|	EORD_M	    |	Untested	| 1088 6309          	                            |
|	EORD_X	    |	Untested    | TESTED NITRO	10A8 6309                           |
|	INCF_I	    |	Untested	| 115C 6309          	                            |
|	LDBT   	    |	UNEMULATED	|		                                            |
|	LDF_D 	    |	Untested    | wcreate		11D6 6309                           |
|	LSRW_I	    |	Untested	| 1054 6309          	                            |
|	ORD_D 	    |	Untested	| 109A 6309          	                            |
|	ORD_E 	    |	Untested	| 10BA 6309          	                            |
|	ORD_M 	    |	Untested	| 108A 6309          	                            |
|	ORD_X 	    |	Untested    | wcreate	10AA 6309          	                    |
|	PSHUW 	    |	Untested	| 103A 6309          	                            |
|	PULSW 	    |	Untested    | wcreate	1039 6309          	                    |
|	PULUW 	    |	Untested	| 103B 6309          	                            |
|	ROLD_I	    |	Untested	| 1049 6309          	                            |
|	RORD_I	    |	Untested	| 1046 6309          	                            |
|	RORW_I	    |	Untested	| 1056 6309          	                            |
|	SBCD_D 	    |	UNEMULATED	|		                                            |
|	SBCD_E 	    |	UNEMULATED	|		                                            |
|	SBCR   	    |	UNEMULATED	|		                                            |
|	STBT   	    |	UNEMULATED	|		                                            |
|	SUBE_D	    |	Untested    | 1190 6309          	                            |
|	SUBE_E	    |	Untested	| 11B0 6309          	                            |
|	SUBE_M	    |	Untested	| 1180 6309          	                            |
|	SUBE_X	    |	Untested	| 11A0 6309          	                            |
|	SUBF_D	    |	Untested	| 11D0 6309          	                            |
|	SUBF_E	    |	Untested	| 11F0 6309          	                            |
|	SUBF_M	    |	Untested	| 11C0 6309          	                            |
|	SUBF_X	    |	Untested	| 11E0 6309          	                            |
|	SUBW_D	    |	Untested    | 6309	1090	                                    |
|	SUBW_E	    |	Untested	| 10B0 6309          	                            |
|	TIM_D 	    |	Untested    | wcreate	B 6309             	                    |
|	TIM_E       |   Untested    | 7B 6309 NITRO                                     |  
|	TSTE_I	    |	Untested    | TESTED NITRO	114D 6309          	                |
|	TSTW_I	    |	Untested    | 6309 wcreate	105D               	                |

## DIVQ Stack trace
vcc.exe!HD6309Exec 		z:\desktop\vcc\vcc\hd6309.c(2224)
vcc.exe!CPUCycle 		z:\desktop\vcc\vcc\coco3.cpp(234)
vcc.exe!RenderFrame 	z:\desktop\vcc\vcc\coco3.cpp(108)
vcc.exe!EmuLoop 		z:\desktop\vcc\vcc\vcc.c(731)



[sock6309]: http://users.axess.com/twilight/sock/cocofile/4mzcycle.html

/*------------------------------------------------------------------------------*/
/**
 *  \file   Stdafx.h
 *  \brief  Include for precompiled header.
 *  \author Gabriel Peyré
 *  \date   10-26-2002
 */
/*------------------------------------------------------------------------------*/
#ifndef _GW_GEODESIC_STDAFX_H_
#define _GW_GEODESIC_STDAFX_H_

#pragma once

/* 'identifier' : identifier was truncated to 'number' characters in the debug information */
#if defined(_MSC_VER)
#pragma warning( disable : 4786 )
#endif

//-------------------------------------------------------------------------
/** \name C++ STL */
//-------------------------------------------------------------------------
//@{
#include <algorithm>
#include <map>
#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <fstream>
using std::string;
using std::cerr;
using std::cout;
using std::endl;
//@}

//-------------------------------------------------------------------------
/** \name classical ANSI c libraries */
//-------------------------------------------------------------------------
//@{
#include <cstdio>
#include <cmath>
#include <float.h>
#include <cstdlib>
#include <fcntl.h>
#include <ctime>
//#include <malloc.h>
#include <memory.h>
//#include <tchar.h>
#include <fstream>
#include <iostream>
//#include <ios>
#include <cstdarg>
//#include <conio.h>
//#include <crtdbg.h>
//@}


#include "../gw_core/GW_MathsWrapper.h"

/*----------------------------------------------------------------------*/
/* win32 #include                                                       */
/*----------------------------------------------------------------------*/

#ifdef _WIN32
    /** main win32 header */
//    #include <windows.h>
#endif // _USE_WIN32_


#endif // _STDAFX_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////

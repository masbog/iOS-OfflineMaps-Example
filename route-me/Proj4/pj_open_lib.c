/******************************************************************************
 * $Id: pj_open_lib.c,v 1.9 2007/07/06 14:58:03 fwarmerdam Exp $
 *
 * Project:  PROJ.4
 * Purpose:  Implementation of pj_open_lib(), and pj_set_finder().  These
 *           provide a standard interface for opening projections support
 *           data files.
 * Author:   Gerald Evenden, Frank Warmerdam <warmerdam@pobox.com>
 *
 ******************************************************************************
 * Copyright (c) 1995, Gerald Evenden
 * Copyright (c) 2002, Frank Warmerdam <warmerdam@pobox.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log: pj_open_lib.c,v $
 * Revision 1.9  2007/07/06 14:58:03  fwarmerdam
 * improve searchpath clearning with pj_set_searchpath()
 *
 * Revision 1.8  2007/03/11 17:03:18  fwarmerdam
 * support drive letter prefixes on win32 and related fixes (bug 1499)
 *
 * Revision 1.7  2006/11/17 22:16:30  mloskot
 * Uploaded PROJ.4 port for Windows CE.
 *
 * Revision 1.6  2004/09/16 15:14:01  fwarmerdam
 * * src/pj_open_lib.c: added pj_set_searchpath() provided by Eric Miller.
 *
 * Revision 1.5  2002/12/14 20:15:30  warmerda
 * updated headers
 *
 */

#define PJ_LIB__
#include "projects.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

PJ_CVSID("$Id: pj_open_lib.c,v 1.9 2007/07/06 14:58:03 fwarmerdam Exp $");

static const char *(*pj_finder)(const char *) = NULL;
static int path_count = 0;
static char **search_path = NULL;
static char * proj_lib_name =
#ifdef PROJ_LIB
PROJ_LIB;
#else
0;
#endif

/************************************************************************/
/*                           pj_set_finder()                            */
/************************************************************************/

void pj_set_finder( const char *(*new_finder)(const char *) )

{
    pj_finder = new_finder;
}

/************************************************************************/
/*                         pj_set_searchpath()                          */
/*                                                                      */
/*      Path control for callers that can't practically provide         */
/*      pj_set_finder() style callbacks.  Call with (0,NULL) as args    */
/*      to clear the searchpath set.                                    */
/************************************************************************/

void pj_set_searchpath ( int count, const char **path )
{
    int i;

    if (path_count > 0 && search_path != NULL)
    {
        for (i = 0; i < path_count; i++)
        {
            pj_dalloc(search_path[i]);
        }
        pj_dalloc(search_path);
        path_count = 0;
        search_path = NULL;
    }

    if( count > 0 )
    {
        search_path = pj_malloc(sizeof *search_path * count);
        for (i = 0; i < count; i++)
        {
            search_path[i] = pj_malloc(strlen(path[i]) + 1);
            strcpy(search_path[i], path[i]);
        }
    }
        
    path_count = count;
}

/************************************************************************/
/*                            pj_open_lib()                             */
/************************************************************************/

FILE *
pj_open_lib(char *name, char *mode) {
    char fname[MAX_PATH_FILENAME+1];
    const char *sysname;
    FILE *fid;
    int n = 0;
    int i;
#ifdef WIN32
    static const char dir_chars[] = "/\\";
#else
    static const char dir_chars[] = "/";
#endif

#ifndef _WIN32_WCE

    /* check if ~/name */
    if (*name == '~' && strchr(dir_chars,name[1]) ) {
        sysname = getenv("HOME");
	   if (sysname) {
            (void)strcpy(fname, sysname);
            fname[n = strlen(fname)] = DIR_CHAR;
            fname[++n] = '\0';
            (void)strcpy(fname+n, name + 1);
            sysname = fname;
        } else
            return NULL;

    /* or fixed path: /name, ./name or ../name */
    } else if (
             strchr(dir_chars,*name)
             || (*name == '.' && strchr(dir_chars,name[1])) 
             || (!strncmp(name, "..", 2) && strchr(dir_chars,name[2]))
             || (name[1] == ':' && strchr(dir_chars,name[2]))
    ) {
        sysname = name;

    /* or try to use application provided file finder */
    } else if( pj_finder != NULL && pj_finder( name ) != NULL ) {
        sysname = pj_finder( name );

    /* or is environment PROJ_LIB defined */
    } else if ((sysname = getenv("PROJ_LIB")) || (sysname = proj_lib_name)) {
        (void)strcpy(fname, sysname);
        fname[n = strlen(fname)] = DIR_CHAR;
        fname[++n] = '\0';
        (void)strcpy(fname+n, name);
        sysname = fname;
    } else /* just try it bare bones */
        sysname = name;

    fid = fopen(sysname, mode);
    if (fid)
        errno = 0;

    /* If none of those work and we have a search path, try it */
    if (!fid && path_count > 0)
    {
        for (i = 0; fid == NULL && i < path_count; i++)
        {
            sprintf(fname, "%s%c%s", search_path[i], DIR_CHAR, name);
            sysname = fname;
            fid = fopen (sysname, mode);
        }
        if (fid)
            errno = 0;
    }

    if( getenv( "PROJ_DEBUG" ) != NULL )
        fprintf( stderr, "pj_open_lib(%s): call fopen(%s) - %s\n",
                 name, sysname,
                 fid == NULL ? "failed" : "succeeded" );

    return(fid);
#else
    return NULL;
#endif /* _WIN32_WCE */
}

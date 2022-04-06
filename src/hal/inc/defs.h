//
// Created by estelle on 05/04/2021.
//

#ifndef PERSO_HNZ_DEFS_H
#define PERSO_HNZ_DEFS_H


//Peut-être à suppr


/*! \addtogroup Srv
 * @{
 */
/*! \file /Inc/Srv/defs.h \brief Définition des types prédéfinis */

/*--
********************************************************************************
                                   defs.h
Module: Tous

Projet: SITRIC

Creation     nom     :   equipe
             Date    :   6/7/95
             Role    :   Diffuser les quelques types predefinis dans tous les
                        sources.

Modification nom     :
             Date    :
             But     :

********************************************************************************
--*/


#ifndef _WIN32

#define VOID		void            /* represente par 'v' */
#define UCHAR 		unsigned char
#define CHAR 		char			/* c   Pour les caracteres ASCII */
#define SHORT		short			/* s   */
#define LONG		long			/* l   */
#define INT			int				/* i   */
#define BYTE		unsigned char 	/* ub  */
#define USHORT		unsigned short	/* us  */
#define ULONG		unsigned long 	/* ul  */
#define UINT		unsigned int	/* ui  */
#define REEL 		float 			/* r   */
#define DOUBLE		double			/* d   */
// pour les champs de bit:			/* cb  */
#define FLAG		short			/* f   */

#ifndef FALSE
#define FALSE		0
#endif

#ifndef TRUE
#define TRUE		1
#endif

#define kFalse		0
#define kTrue		1

#define BOOL		int
#define WORD		unsigned short
#define DWORD		unsigned long
#define POSITION	unsigned int
#define LPCSTR		const char*
#define LPSTR		char*
#define LPVOID		void*
#define HANDLE		void*
#define AFX_DATA
#define _stdcall
#define SOCKET		int
#define closesocket close
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif
#define SOCKET_ERROR -1
#ifndef WSAEWOULDBLOCK
#define WSAEWOULDBLOCK EWOULDBLOCK
#endif
#ifndef ASSERT
#define ASSERT assert
#endif
#define _MAX_PATH 260

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>
#define _timeb timeb
#define _ftime ftime
#define _chdir chdir
#define _isnan isnan
#define _stat stat
#define _getcwd getcwd
#define _access access
#define _lfind lfind
#define _stat stat
#define _mkdir(trfgyh)  mkdir((const char*)trfgyh, 0750)
#define _popen popen
#define _pclose pclose
#define _vsnprintf vsnprintf
#define _rmdir rmdir
#define _unlink unlink
#define _copysign copysign

#define stricmp	 strcasecmp
#define strnicmp strncasecmp

#define _strupr(a) for (int hjuik=0; hjuik<(int)strlen(a); hjuik++) a[hjuik]=toupper(a[hjuik])

#define Sleep(A) usleep(A*1000);

#ifndef ALLPERMS
#define ALLPERMS (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO)
#endif

#ifndef UINT64
#define UINT64 unsigned long long int
#define INT64  long long int
#endif

#endif // UNIX

#define FORMAT_I64 "%lld"

/*--
        Un nom de tableau comporte un a dans son pr‚fixe. Un type de tableau
	aussi. Nous eviterons d'utiliser des types chaines de caracteres, leur
	utilisation necessite de savoir qu'il s'agit d'un tableau, sur modification
	de type (passage a une structure) le code doit etre beaucoup modifie.
        Une chaŒne est rep‚r‚e par sz.
        Un tableau de CHAR donne acXxxx ou abXxxx selon qu'il re‡oit
        respectivement des code ASCII ou autre chose sign‚. Nous aurons aubXxx
        pour autre chose non sign‚.
        Un tableau de pointeur de chaŒne (argc par exemple) sera apszArgc

    Les types de base:
        v           void
        b           byte (octet sign‚)
        c           car ASCII (non sign‚)
        s           short (16 bits)
        l           long
        r           float
        d           double
        f           flag (unsigned short)

        e_	    pour les differentes valeurs d'enum
	k           pour les constantes
        m           pour les macros
	a           array (tableau)
        sz          string (‚quivallent … 'ac' termin‚ par '\0')
        p           pointeur
	st          structure a usage local
--*/

#endif //PERSO_HNZ_DEFS_H

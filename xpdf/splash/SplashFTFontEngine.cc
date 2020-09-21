//========================================================================
//
// SplashFTFontEngine.cc
//
//========================================================================

#include <aconf.h>

#if HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdio.h>
#ifndef WIN32
#  include <unistd.h>
#endif
#include "gmem.h"
#include "GString.h"
#include "gfile.h"
#include "FoFiTrueType.h"
#include "FoFiType1C.h"
#include "SplashFTFontFile.h"
#include "SplashFTFontEngine.h"

#ifdef VMS
#if (__VMS_VER < 70000000)
extern "C" int unlink(char *filename);
#endif
#endif

//------------------------------------------------------------------------

static void fileWrite(void *stream, const char *data, int len) {
  fwrite(data, 1, len, (FILE *)stream);
}

//------------------------------------------------------------------------
// SplashFTFontEngine
//------------------------------------------------------------------------

SplashFTFontEngine::SplashFTFontEngine(GBool aaA, Guint flagsA,
				       FT_Library libA) {
  FT_Int major, minor, patch;

  aa = aaA;
  flags = flagsA;
  lib = libA;

  // as of FT 2.1.8, CID fonts are indexed by CID instead of GID
  FT_Library_Version(lib, &major, &minor, &patch);
  useCIDs = major > 2 ||
            (major == 2 && (minor > 1 || (minor == 1 && patch > 7)));
}

SplashFTFontEngine *SplashFTFontEngine::init(GBool aaA, Guint flagsA) {
  FT_Library libA;

  if (FT_Init_FreeType(&libA)) {
    return NULL;
  }
  return new SplashFTFontEngine(aaA, flagsA, libA);
}

SplashFTFontEngine::~SplashFTFontEngine() {
  FT_Done_FreeType(lib);
}

SplashFontFile *SplashFTFontEngine::loadType1Font(SplashFontFileID *idA,
						  char *fileName,
						  GBool deleteFile,
						  const char **enc) {
  return SplashFTFontFile::loadType1Font(this, idA, fileName, deleteFile, enc);
}

SplashFontFile *SplashFTFontEngine::loadType1CFont(SplashFontFileID *idA,
						   char *fileName,
						   GBool deleteFile,
						   const char **enc) {
  return SplashFTFontFile::loadType1Font(this, idA, fileName, deleteFile, enc);
}

SplashFontFile *SplashFTFontEngine::loadOpenTypeT1CFont(SplashFontFileID *idA,
							char *fileName,
							GBool deleteFile,
							const char **enc) {
  return SplashFTFontFile::loadType1Font(this, idA, fileName, deleteFile, enc);
}

SplashFontFile *SplashFTFontEngine::loadCIDFont(SplashFontFileID *idA,
						char *fileName,
						GBool deleteFile) {
  FoFiType1C *ff;
  int *cidToGIDMap;
  int nCIDs;
  SplashFontFile *ret;

  // check for a CFF font
  if (useCIDs) {
    cidToGIDMap = NULL;
    nCIDs = 0;
  } else if ((ff = FoFiType1C::load(fileName))) {
    cidToGIDMap = ff->getCIDToGIDMap(&nCIDs);
    delete ff;
  } else {
    cidToGIDMap = NULL;
    nCIDs = 0;
  }
  ret = SplashFTFontFile::loadCIDFont(this, idA, fileName, deleteFile,
				      cidToGIDMap, nCIDs);
  if (!ret) {
    gfree(cidToGIDMap);
  }
  return ret;
}

SplashFontFile *SplashFTFontEngine::loadOpenTypeCFFFont(SplashFontFileID *idA,
							char *fileName,
							GBool deleteFile,
							int *codeToGID,
							int codeToGIDLen) {
  FoFiTrueType *ff;
  GBool isCID;
  int *cidToGIDMap;
  int nCIDs;
  SplashFontFile *ret;

  cidToGIDMap = NULL;
  nCIDs = 0;
  isCID = gFalse;
  if (!codeToGID) {
    if (!useCIDs) {
      if ((ff = FoFiTrueType::load(fileName))) {
	if (ff->isOpenTypeCFF()) {
	  cidToGIDMap = ff->getCIDToGIDMap(&nCIDs);
	}
	delete ff;
      }
    }
  }
  ret = SplashFTFontFile::loadCIDFont(this, idA, fileName, deleteFile,
				      codeToGID ? codeToGID : cidToGIDMap,
				      codeToGID ? codeToGIDLen : nCIDs);
  if (!ret) {
    gfree(cidToGIDMap);
  }
  return ret;
}

SplashFontFile *SplashFTFontEngine::loadTrueTypeFont(SplashFontFileID *idA,
						     char *fileName,
						     int fontNum,
						     GBool deleteFile,
						     int *codeToGID,
						     int codeToGIDLen) {
  FoFiTrueType *ff;
  GString *tmpFileName;
  FILE *tmpFile;
  SplashFontFile *ret;

  //~ this should use fontNum to load the correct font
  if (!(ff = FoFiTrueType::load(fileName))) {
    return NULL;
  }
  tmpFileName = NULL;
  if (!openTempFile(&tmpFileName, &tmpFile, "wb", NULL)) {
    delete ff;
    return NULL;
  }
  ff->writeTTF(&fileWrite, tmpFile);
  delete ff;
  fclose(tmpFile);
  ret = SplashFTFontFile::loadTrueTypeFont(this, idA,
					   tmpFileName->getCString(), fontNum,
					   gTrue, codeToGID, codeToGIDLen);
  if (ret) {
    if (deleteFile) {
#ifdef WINCE
      wchar_t *ws = new wchar_t[strlen(fileName)+1];
	  MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,fileName,-1,ws, strlen(fileName)+1);

      DeleteFile(ws);
      delete[] ws;
#else
      unlink(fileName);
#endif
    }
  } else {
#ifdef WINCE
    DeleteFile( tmpFileName->getCStringW() );
#else
    unlink(tmpFileName->getCString());
#endif
  }
  delete tmpFileName;
  return ret;
}

#endif // HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H

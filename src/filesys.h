#ifndef _FILESYS_
#define _FILESYS_

#include <libBAG.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FILES 1024
#define MAX_DIRS 64

#define TYPES_AUDIO_COUNT 4
#define TYPES_AUDIO_EXT "mp3\0","ogg\0","raw\0","wav\0"
#define TYPES_AUDIO TYPES_AUDIO_COUNT,TYPES_AUDIO_EXT

#define TYPES_IMAGE_COUNT 3
#define TYPES_IMAGE_EXT "bmp\0","jpg\0","png\0"
#define TYPES_IMAGE TYPES_IMAGE_COUNT,TYPES_IMAGE_EXT

#define TYPES_BINARY_COUNT 3
#define TYPES_BINARY_EXT "nds\0","nzip\0","plg\0"
#define TYPES_BINARY TYPES_BINARY_COUNT,TYPES_BINARY_EXT

#define TYPES_INTERNAL_COUNT (TYPES_AUDIO_COUNT + TYPES_IMAGE_COUNT + TYPES_BINARY_COUNT)
//alphabetically sorted
#define TYPES_INTERNAL_EXT "bmp\0","jpg\0","mp3\0","nds\0","nzip\0","ogg\0","plg\0","png\0","raw\0","wav\0"
#define TYPES_INTERNAL TYPES_INTERNAL_COUNT,TYPES_INTERNAL_EXT

typedef enum{
	DIRTYPE_FOLDER,
	DIRTYPE_TEXT,
	DIRTYPE_ZIP,
	DIRTYPE_LIST,
}DirectoryTypes;

typedef enum{
	ISDIR = (1<<0),
	CUSTOMDIR = (1<<1),//file has a special directory
}Entry_Flags;

typedef struct _Entry_s{
	char *name, *ext, *dir;//dir not used in file browser
	unsigned char flags,
				  error, 
				  levels;//leves for folders, determine what order to remove in when deleting
}Entry;

//formating for entry string
typedef enum{
	ENTRY_PATH = (1<<0),
	ENTRY_NAME = (1<<1),
	ENTRY_EXT = (1<<2),
	ENTRY_SYMBOLS = (1<<3),//display folder dividers and extension periods (symbols)
}GetEntry_Format;

typedef enum{
	LIST_DIR = (1<<0),
	LIST_TXT = (1<<2),
}EntryList_Format;

typedef struct EntryList_s{
	//files
	unsigned long fileCount, fileMax;
	Entry *Files;
	//directory
	unsigned long dirCount, dirMax;
	Entry *Directory;
	//curent directory the file list originates
	char curDir[MAX_PATH>>1], namebuf[MAX_PATH];
	//any errors
	unsigned char error, flags;
}EntryList;

//supported files
//keep track of supported files to display
typedef struct SupportedExt_s{
	//list of extensions
	char **ext;
	//count
	unsigned char count, error;
}SupportedExt;

//core filebrowser
typedef enum{
	HIDEDIRS = (1 << 0),//do not list directories
	HIDEFILES = (1 << 1),//do not list files
	SHOWHIDDEN = (1 << 2),//show hidden files
	SKIPPREVDIR = (1 << 3),//skip listing previous directory entry
	LISTDEAD = (1 << 4),//show files that don't exist (for txt file listings)
	WALKDIR = (1 << 5),//list all files in all directories in a target directory
}FileBrowseCore_Flags;

typedef enum{
	WALK_NOTHING = (1 << 0),
	WALK_RM = (1 << 1),
}DirWalk_Flags;


typedef struct FileBrowseCore_s{
	EntryList *List;
	SupportedExt *Types;
	u16 flags;
}FileBrowseCore;


//================================================================
typedef struct dirWalk{
	char type;//directory walk type
	struct stat st;
	DIR* dir;

    char filename[MAX_PATH];//current file to return
	char  CurDir[MAX_PATH];//current director to scan
                
	int Folder_Count,//number of folders scanned
		File_Count,//number of files scanned
		curDirPos,//current directory position
		//keep track of directory positioning
		Dir_Levels,
		DirPos[32];//max 32 levels deep
    //size in bytes of all files scanned
    unsigned long long total_bytes;
}DirWalk;
//===================================================================
//regards to file types supported
extern SupportedExt *fsys_setFileTypes(char *ext[], unsigned char count);
extern SupportedExt *fsys_setFileTypesEx(unsigned char count, ...);
extern void fsys_freeFileTypes(SupportedExt *types);
extern int fsys_getSupportedType(const SupportedExt *types, const char *ext);

//core setup
extern FileBrowseCore *fsys_Init(SupportedExt *ext, unsigned char flags);
extern void fsys_DeInit(FileBrowseCore *core);


//directory handling
extern int fsys_OpenDir(const char *dir, FileBrowseCore *fb, char walk);
extern void fsys_CloseDir(FileBrowseCore *fb);
extern int fsys_ChangeDir(const char *dir, FileBrowseCore *fb);
extern int fsys_BackDir(FileBrowseCore *fb);

//get file or directory entry in file system
extern char *fsys_getEntryString(Entry *temp, FileBrowseCore *fb, unsigned char format);
extern char *fsys_getEntryStringByNum(unsigned long fileNum, FileBrowseCore *fb, unsigned char format);

//file settings
extern void fsys_hideEntry(Entry *file, FileBrowseCore *fb, unsigned char hide);
extern void fsys_hideEntryByNum(unsigned long fileNum, FileBrowseCore *fb, unsigned char hide);

extern void fsys_remove(unsigned long fileNum, FileBrowseCore *fb);
extern int fsys_dumpListToText(const char *destFile, FileBrowseCore *fb);
extern unsigned long fsys_getEntryCount(FileBrowseCore *fb);
extern char fsys_isEntryDirFromNum(FileBrowseCore *fb, unsigned long fileNum);
extern u16 *fsys_GetFlags(FileBrowseCore *core);
extern int fsys_getEntryNumber(FileBrowseCore *fb, const char *searchName);
extern Entry *fsys_getEntryFromNumber(FileBrowseCore *fb, unsigned long fileNum);

#ifdef __cplusplus
}
#endif


#endif

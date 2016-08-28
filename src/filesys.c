/*
*Wrappers for filebrowsing with BAGASM
*/ 
#include "filesys.h"

static char linebuf[MAX_PATH];
/*===================================================================================
*   Misc functions
* 
*
*
===================================================================================*/
//Converts upper case characters to lower case
static char getSmallChar(char letter){
	if((letter >= 'A')&&(letter <= 'Z')) 
		return (letter-'A'+'a');
	
   return letter;
}

//count how many levels a path leads through
static unsigned char dirLevels(char *path){
	unsigned char count = 0;
	char *pch = path;
	while((pch = strchr (pch, '/')) != NULL)
		count++;
	return count - 1;//do not count the root /
}


static long dirBack(char *origPath){
	long len = strlen(origPath) - 1;
	do{
		origPath[len] = '\0';len--;
	}while(origPath[len] != '/' && len > 0);
	//hit root directory
	if(len <= 0){
		len = 0;
		origPath[0] = '/';
		origPath[1] = '\0';
	}
	return len + 1;
}

static void rmdir_ex(char *path){
    //strip trailing characters from folder path
    int len = strlen(path);
    while(path[len] == '/' || path[len] == '\0')
        path[len--]= '\0';
        
    rmdir(path);
}

static void hideFile(const char *name, u8 hide){
	fat_setHidden(name, hide);
}

//determine if a file is hidden or not based on attributes
static unsigned char isFileHid(char *file, struct stat *st){
	if(!strcmp(file, ".."))
		return 0;
	else if(file[0] == '.' && file[1] != '.')
		return 1;
	
	return fat_isHidden(st);
}

//separate directory path from a full file path
static int splitNamePath(const char *fullPath, char *outDir, char *outFile){
	char tempPath[MAX_PATH];
	strcpy(tempPath, fullPath);
	long length = dirBack(tempPath);
	strcpy(outFile, &fullPath[length]);
	strcat(outFile, "\0");
	strcpy(outDir, tempPath);
	strcat(outDir, "\0");
	return 1;
}

//get a file extension from a name to a buffer
static unsigned long nameGetExt(const char *filename, char *ext_buf, int bufsize)
{
	// go to end of name
	int len = strlen(filename);
	int i = len;
	while(filename[i] != '.' && i > 0)
		i--;
	if(i <= 0)
		return -2;

	int j = i + 1;
	i = j;
	do {
        if( (i - j) >= bufsize)
			return -2;
		
        ext_buf[i-j] = getSmallChar(filename[i]);
	}while( i++ < len);
	ext_buf[i - j] = '\0';
	return j - 1;//the . for the extension
}

//grab a line from a text file
static char *fat_getLine(char *buffer, unsigned long size, FILE *file){
	char *line = fgets (buffer, size, file);
	if(line == NULL || *line == EOF)
		return NULL;

	//remove trailing characters from path name
	unsigned long i = strlen(buffer);
	while(i>0 && (buffer[i] == '\n' || buffer[i] == '\0' || buffer[i] == '\r'))
		buffer[i--] = '\0';
	return line;
}

/*===================================================================================
*   Collecting and Searching of Supported file types
* 
*
*
===================================================================================*/

//create a list to check for supported file types, list must be alphabetically sorted
SupportedExt *fsys_setFileTypes(char *ext[], unsigned char count){
	//first allocate the types struct
	SupportedExt *types = calloc(1, sizeof(SupportedExt));
	if(types == NULL)
		return NULL;
	
	//now allocate enough space in the list for all the types
	types->count = count;
	types->ext = calloc(types->count, sizeof(char*));
	if(types->ext == NULL){
		free(types);
		return NULL;
	}

	//now allocate each type to hold the text
	int i = 0;
	for(; i< count; i++){
		types->ext[i] = calloc(strlen(ext[i]) + 1, sizeof(char));
		if(types->ext[i] == NULL){
			types->error = 2;
			break;
		}
		//copy the text from the given list
		strcpy(types->ext[i], ext[i]);
	}
	if(types->error){
		for(i = 0; i < count; i++){
			if(types->ext[i])
				free(types->ext[i]);
		}
		free(types->ext);
		free(types);
		return NULL;
	}
	return types;
}

SupportedExt *fsys_setFileTypesEx(unsigned char count, ...){
	char *types[count];
	unsigned char i = 0, exit = 0;
	va_list ext;
	//collect extensions
    va_start(ext, count);
	for(; i < count; i++){
		types[i] = va_arg(ext, char*);
		//check if all types should be shown
		if(!strcmp(types[i], "*"))
			exit++;
	}
	va_end(ext);
	if(exit)
		return NULL;
	//try to add them to list and return results
	return fsys_setFileTypes(types, count);
}
//free list of supported file types
void fsys_freeFileTypes(SupportedExt *types){
	if(types == NULL || types->count == 0 || types->ext == NULL)
		return;

	int i = 0;
	for(; i < types->count; i++){
		if(types->ext[i])
			free(types->ext[i]);
		types->ext[i] = NULL;
	}

	free(types->ext);
	types->ext = NULL;
	types->count = 0;
}
//return a positive value if type is on list
int fsys_getSupportedType(const SupportedExt *types, const char *ext){
    register int min = 0, max = types->count - 1, mid = 0, cmp = 0;
    if(max < 0)
        return -2;
    do{
        mid = (min + max) >> 1;
        //don't go out of bounds
        if(mid > types->count - 1)
        	return -1;

        if((cmp = strcasecmp(types->ext[mid], ext)) == 0)
            return mid;
        else if(cmp < 0)
            min = mid + 1;
        else
            max = mid - 1;
    }while(min <= max);
    return -1;  
}
/*===================================================================================
*   Scanning and collecting of entries in a directory to a list
*		-directories and files each have their own list 
*
*
===================================================================================*/

//free individual file allocations
static void entryReset(Entry *file){
	if(file == NULL)
		return;
	if(file->dir)
		free(file->dir);
	file->dir = NULL;
	if(file->name)
		free(file->name);
	file->name = NULL;
	if(!GET_FLAG(file->flags, ISDIR)){
		if(file->ext)
			free(file->ext);
	}
	file->ext = NULL;
	file->flags = file->error = 0;
}

//free a list of file allocations
static void entryListReset(EntryList *list){
	if(list == NULL)
		return;

	int i = 0;
	//reset directories
	if(list->Directory && list->dirCount > 0){
		for(i = 0; i < list->dirCount; i++)
			entryReset((Entry*)&list->Directory[i]);
		free(list->Directory);
	}
	list->Directory = NULL;
	//reset files
	if(list->Files && list->fileCount > 0){
		for(i = 0; i < list->fileCount; i++)
			entryReset(&list->Files[i]);
		free(list->Files);
	}
	list->Files = NULL;

	memset(list->curDir, 0, sizeof(list->curDir));
	memset(list->namebuf, 0, sizeof(list->namebuf));
	list->dirCount = list->fileCount = 0;
	list->dirMax = list->fileMax = 0;
}

static char *entryAddInfo(const char *string){
	unsigned long len = strlen(string);
	char *info = calloc( len + 1, sizeof(char));
	if(info != NULL){
		strcpy(info, string);
		strcat(info, "\0");
	}
	return info;
}

static char *entryGetPath(Entry *file){
	return file->dir;
}

static char *entryGetName(Entry *file){
	return file->name;
}

static char *entryGetExt(Entry *file){
	return file->ext;
}

static unsigned char entryGetFlag(Entry *file){
	return file->flags;
}

static unsigned char entryAddToList(Entry *file, const char *dir, const char *name, const char *ext, unsigned char flags){
	file->flags = flags;
	file->error = 0;
	file->dir = NULL;
	file->name = NULL;
	file->ext = NULL;

	//collect directory if specified
	if(dir != NULL){
		if((file->dir = entryAddInfo(dir)) == NULL)
			file->error = 1;
		else//set flag if a custom directory is specified
			SET_FLAG(file->flags, CUSTOMDIR);
	}
	//collect name
	if((file->name = entryAddInfo(name)) == NULL)
		file->error = 2;

	//collect extension if not a folder
	if(!GET_FLAG(flags, ISDIR)){
		if((file->ext = entryAddInfo(ext)) == NULL)
			file->error = 3;
	}

	return file->error;
}

static Entry *entryResizeList(Entry *originalList, unsigned long size){
	Entry *temp = realloc(originalList, sizeof(Entry) *(size +1));
	return temp;
}

static unsigned long entryRemoveFromList(Entry *file, unsigned long num, unsigned long total){
	entryReset(&file[num]);
	total--;
	unsigned long i = num;
	for(; i < total; i++)
		file[i] = file[i + 1];
	return total;
}

static int _dir_Entry_Count = 0;
static void *_openDir(const char *directory, char type){
	_dir_Entry_Count = 0;
	switch(type){
		case DIRTYPE_LIST:
		case DIRTYPE_FOLDER:
			return opendir(directory);
		break;
		case DIRTYPE_TEXT:
			return fopen(directory, "rb");
		break;
		case DIRTYPE_ZIP:
		break;
	}
	return NULL;
}

static void _closeDir(void* data, char type){
	_dir_Entry_Count = 0;
	DirWalk *temp = NULL;
	switch(type){
		case DIRTYPE_LIST:
			temp = (DirWalk*)data;
			closedir(temp->dir);
		break;
		case DIRTYPE_FOLDER:
			closedir((DIR*)data);
		break;
		case DIRTYPE_TEXT:
			fclose((FILE*)data);
		break;
	}
}

//directory walking, adds all files and folders in a single directory to a list,
//rather than just the files in a specified folder
//does not work for walking text file lists!
static char *dirWalk(DirWalk *dirInfo, struct stat *st, unsigned char flags){
	if(!dirInfo->dir)//check if directory is openened
		return NULL;
	dirent *current_file = readdir_ex(dirInfo->dir, &dirInfo->st);
	if(!current_file){
		//close current open directory
		closedir(dirInfo->dir);
		//go back one directory
		if(dirInfo->Dir_Levels > 0){
			if(flags & WALK_RM)//delete directories if flag is set
				rmdir_ex(dirInfo->CurDir);
			dirBack(dirInfo->CurDir);
		}
		//return as done
		if(--dirInfo->Dir_Levels <= -1){
			if(flags & WALK_RM)//delete directory if flag is set
				rmdir_ex(dirInfo->CurDir);
			return NULL;
		}

		//otherwise...
		dirInfo->curDirPos = 0;
		dirInfo->dir = opendir(dirInfo->CurDir);
		if(dirInfo->dir == NULL)
			return NULL;//nothing more to open
	}
	//we are now in a directory ready to scan
	//skip files until we reach the last position where a folder was opened
	else if(dirInfo->curDirPos > dirInfo->DirPos[dirInfo->Dir_Levels]){
		strcpy(dirInfo->filename, current_file->d_name);
		//skip the two root paths
		if(!strcasecmp(dirInfo->filename, ".") || !strcasecmp(dirInfo->filename, ".."))
			strcpy(dirInfo->filename, "$NOENTRY$\0");
		
		//check if current entry is a folder and is not a system folder
		else if(S_ISDIR(dirInfo->st.st_mode)){	
			//the entry is a folder, so we will enter it and scan it as well	
			dirInfo->DirPos[dirInfo->Dir_Levels] = dirInfo->curDirPos;//directory level history
			//set the position in the directory back to zero
			dirInfo->curDirPos = 0;
			dirInfo->DirPos[++dirInfo->Dir_Levels] = 0;//set to zero
			
			dirInfo->Folder_Count++;//increase folder count
			//copy folder name to current folder path to open
			strcat(dirInfo->CurDir, dirInfo->filename);
			strcat(dirInfo->CurDir,"/\0");
			
			//close the old directory
			closedir(dirInfo->dir);
			//open directory from new path
			dirInfo->dir = opendir(dirInfo->CurDir);
			if(dirInfo->dir == NULL)
				return NULL;
		}
		//entry is a file
		else if(!S_ISDIR(dirInfo->st.st_mode)){
            dirInfo->total_bytes += BAG_Filesystem_GetFileSize(dirInfo->filename);
			dirInfo->File_Count++;
			if(flags & WALK_RM)//remove file if flag is set
				remove(dirInfo->filename);
		}
		memcpy(st, &dirInfo->st, sizeof(struct stat));
	}
	else
		strcpy(dirInfo->filename, "$NOENTRY$\0");
	dirInfo->curDirPos++;
    return dirInfo->filename;
}

//get file information from a directory or text file
static int _readDir(void *data, char *nameBuf, struct stat *st, char type){
	char *current_file_txt = NULL;
	dirent *current_file_dir = NULL;
	int rtrn = 0;

	switch(type){//return 1 == universal success
		case DIRTYPE_FOLDER://returns 0 or 1
			//we are given just the file name from the readdir_ex function
			current_file_dir = readdir_ex((DIR*)data, st);
			if(current_file_dir == NULL)
				return 0;
			strcpy(nameBuf, current_file_dir->d_name);
			rtrn = 1;
		break;
		case DIRTYPE_TEXT://returns 0, 1, 2 or 3
			st->st_mode &= ~S_IFDIR;//reset dir flag for now
			//add the "up one dir" path since it's not native to text files
			if(_dir_Entry_Count == 0){
				strcpy(nameBuf, "..\0");
				st->st_mode |= S_IFDIR;//set mode as directory
				rtrn = 3;
				goto COUNT;
			}
			else{
				//grab a line from a text file and make ensure it's filled
				current_file_txt = fat_getLine(nameBuf, MAX_PATH, (FILE*)data);
				//check if there is a file listed
				if(current_file_txt == NULL || *current_file_txt == '\n' || 
				   *current_file_txt == ' ' || *current_file_txt == '\r' || *current_file_txt == '\t'){
					rtrn = 0;
					goto COUNT;
				}
			}
			rtrn = 1;
			if(lstat(nameBuf, st) < 0)
				rtrn = 2;//file doesn't exist, but we can choose to list it anyway
		break;
		//list all files in all directorys of source diriectory
		case DIRTYPE_LIST:
			current_file_txt = dirWalk((DirWalk*)data, st, 0);
			if(current_file_txt == NULL)
				return 0;
			strcpy(nameBuf, current_file_txt);
			rtrn = 1;
		break;
	}
	COUNT:
	_dir_Entry_Count++;
	return rtrn;
}


//adds file information to a list
static int _dir_getEntry(void *dir, char dirType, EntryList *list, SupportedExt *types, unsigned char flags){
	DirWalk *tempWalk = NULL;
	Entry *tempEntry = NULL;//for reallocing a list
	//clear some unused buffers at this point
	memset(list->curDir, 0, sizeof(list->curDir));
	memset(list->namebuf, 0, sizeof(list->namebuf));

	char *tempPath = &list->curDir[0],//directory gets its own whole buffer
		 *tempName = &list->namebuf[0],//name and extension will share the other buffer 
		 *tempEXT = &list->namebuf[MAX_PATH>>1];

	struct stat st;
	int tempFlags = 0, file = 0;

	if((file = _readDir(dir, tempPath, &st, dirType)) > 0){
		//repeated entry when directory walking, skip it
		if(!strncmp(tempPath, "$NOENTRY$", 9))
			goto SKIPENTRY;

		//if file is hidden, skip entry then
		if(!GET_FLAG(flags, SHOWHIDDEN) && isFileHid(tempPath, &st))
			goto SKIPENTRY;

		//grab the file names appropriately
		switch(dirType){
			case DIRTYPE_FOLDER:
				//copy name to tempName buffer
				strncpy(tempName, tempPath, MAX_PATH>>1);
				//then don't store the file path (it's consistant in a directory)
				tempPath = NULL;
			break;
			case DIRTYPE_TEXT:
				memset(linebuf, 0, sizeof(linebuf));
				//determine whether to skip dead entries or not
				if(file == 2 && !GET_FLAG(flags, LISTDEAD))
					goto SKIPENTRY;
				//handle the .. entry for text files
				if(file == 3){
					tempName = &list->curDir[0];
					tempPath = NULL;
				}
				else if(splitNamePath(tempPath, linebuf, tempName) < 0)
					goto SKIPENTRY;
				tempPath = &linebuf[0];
			break;
			case DIRTYPE_LIST:
				tempWalk = (DirWalk*)dir;
				tempName = tempWalk->filename;
				tempPath = tempWalk->CurDir;
			break;
		}
		//now to collect the info in a nice list
		//current file is actually a folder
		if(S_ISDIR(st.st_mode) && list->dirCount < list->dirMax){
			//skip directories if flag is set
			if(GET_FLAG(flags, HIDEDIRS) || (GET_FLAG(flags, SKIPPREVDIR) && !strcasecmp(tempName, "..")))
				goto SKIPENTRY;

			SET_FLAG(tempFlags, ISDIR);
			//if not hidden, add it to the folder list
			if((tempEntry = entryResizeList(list->Directory, list->dirCount)) == NULL)
				return 0;
			list->Directory = tempEntry;
			//folders do not have file extensions to store
			if(!entryAddToList(&list->Directory[list->dirCount], tempPath, tempName, NULL, tempFlags))
				list->dirCount++;
		}

		//if not a directory then its a file
		else if(!GET_FLAG(flags, HIDEFILES) && !S_ISDIR(st.st_mode) && list->fileCount < list->fileMax){
			int length = nameGetExt(tempName, tempEXT, MAX_PATH);
			//if file type isn't supported, or if there is no extension when doing a check, skip file
			if(types && (fsys_getSupportedType(types, tempEXT) < 0 || length <= 0))
				goto SKIPENTRY;
			if(length>0)//strip extension off the name, if needed
				memset(&tempName[length], 0, ((MAX_PATH>>1) - length)-1);
			
			//add file to list
			if((tempEntry = entryResizeList(list->Files, list->fileCount)) == NULL)
				return 0;
			list->Files = tempEntry;
			if(!entryAddToList(&list->Files[list->fileCount], tempPath, tempName, tempEXT, tempFlags))
				list->fileCount++;
		}
		SKIPENTRY:;
		//exit once the file entry limit is hit
		if(list->fileCount >= list->fileMax)
			return 0;
	}
	else
		return 0;
	return 1;
}

static EntryList *entryMakeList(const char *directory, SupportedExt *ext, unsigned char flags){
	EntryList *list = calloc(1, sizeof(EntryList));
	if(list == NULL)
		return NULL;

	list->dirCount = 0;
	list->dirMax = MAX_DIRS;
	list->fileCount = 0;
	list->fileMax = MAX_FILES;
	list->Files = NULL;
	list->Directory = NULL;
	char type = 0;
	struct stat st;
	DirWalk walk;
	//check if path is a directory and not a file list
	//walk the directory instead

	//if we aren't walking, then use regular methods
	if(!GET_FLAG(flags, WALKDIR)){
		lstat(directory, &st);
		if(S_ISDIR(st.st_mode)){
			type = DIRTYPE_FOLDER;
			SET_FLAG(list->flags, LIST_DIR);
		}
		else{//text file with list of files to browse
			type = DIRTYPE_TEXT;
			SET_FLAG(list->flags, LIST_TXT);
		}	
	}
	else{//prepare for directory walking
		type = DIRTYPE_LIST;
		SET_FLAG(list->flags, LIST_DIR);
		memset(&walk, 0, sizeof(DirWalk));
		//open the directory and read the contents
		strcpy(walk.CurDir, directory);
		strcat(walk.CurDir, "/\0");
		walk.curDirPos = 1;//start the walk
	}

	//standard open directory for reading
	void *dir = _openDir(directory, type);
	//open the directory and read the contents
	if(dir == NULL){
		list->error = 1;
		return list;
	}

	//prepare for directory walking
	if(GET_FLAG(flags, WALKDIR)){
		walk.dir = (DIR*)dir;
		dir = (void*)&walk;
	}

	//populate file list
	while(_dir_getEntry(dir, type, list, ext, flags));
	_closeDir(dir, type);

	//copy directory path as the lists current dir
	strcpy(list->curDir, directory);
	return list;
}

/*===================================================================================
*   Entry list Sorting and retrieval
* 
*
*
===================================================================================*/
//sorts folders on top in alphabetical order, then files underneath in alphabetical order
static int sortEntryCheck(const Entry *dest_str_ptr, const Entry *src_str_ptr)
{
  char *dest_str = dest_str_ptr->name;
  char *src_str = src_str_ptr->name;
 
  if(src_str[0] == '.')
    return 1;

  if(dest_str[0] == '.')
    return -1;

  return strcasecmp(dest_str, src_str);
}

static int sortEntryPartition(Entry *array, int left, int right){
    Entry pivot = *((Entry*)array + left);

    while(left < right){
        while(sortEntryCheck((void*)((Entry*)array+left), (void*)((Entry*)array+right)) < 0) {
            right--;
        }

        if(right== left) break;
        *((Entry*)array + left) = *((Entry*)array + right);
        *((Entry*)array + right) = pivot;

        if(left < right){
            left++;
            if(right== left) break;
        }

        while(sortEntryCheck((void*)((Entry*)array+right), (void*)((Entry*)array+left)) > 0) {
            left++;
        }

        if(left== right) break;
        *((Entry*)array + right) = *((Entry*)array + left);
        *((Entry*)array + left) = pivot;
        right--;
    }
    return left;
}

static void sortEntryList(void *array, int left, int right){
    if(left < right){
        int mid= sortEntryPartition(array, left, right);
        sortEntryList(array, left, mid-1);
        sortEntryList(array, mid+1, right);
    }
}


static long getEntrySmallNumber(unsigned long fileNum, FileBrowseCore *fb){
	//directories represented by positive numbers
	if(fileNum < fb->List->dirCount)
		return fileNum + 1;
	else{
		//files are indicated by negative numbers
		register unsigned long temp = fileNum - fb->List->dirCount;
		if(temp < fb->List->fileCount)
			return -(temp + 1);		
	}
	//no entry returns zero
	return 0;
}

static Entry *getEntryFromNum(unsigned long fileNum, FileBrowseCore *fb){
	long temp = getEntrySmallNumber(fileNum, fb);
	if(temp < 0)
		return &fb->List->Files[((-1*temp)-1)];
	else if(temp > 0)
		return &fb->List->Directory[temp - 1];
	return NULL;
}

static unsigned long getEntryNum(const Entry *list, unsigned long total, const char *name){
    register int min = 0, max = total - 1, cmp = 0;
    unsigned long mid = 0;
    if(max < 0)
        return -2;

    char nameBuf[MAX_PATH]; 
    do{
        mid = (min + max) >> 1;
        //don't go out of bounds
        if(mid >= total)
        	return -1;
        //build name to check
        strcpy(nameBuf, list[mid].name);
        if(list[mid].ext){
	        strcat(nameBuf, ".");
	        strcat(nameBuf, list[mid].ext);
	    }
        if((cmp = strcasecmp(nameBuf, name)) == 0)
            return mid;
        else if(cmp < 0)
            min = mid + 1;
        else
            max = mid - 1;
    }while(min <= max);
    return -1;  
}


static void removeEntry(unsigned long fileNum, FileBrowseCore *fb){
	long temp = getEntrySmallNumber(fileNum, fb);
	if(temp < 0){
		fb->List->fileCount = entryRemoveFromList(fb->List->Files, (-1*temp)-1, fb->List->dirCount);
		fb->List->Files = entryResizeList(fb->List->Files, fb->List->fileCount);
		if(fb->List->Files){
			printf("error removing entry\n");
			while(1);
		}
	}
	else if(temp > 0){
		fb->List->dirCount = entryRemoveFromList(fb->List->Directory, temp, fb->List->dirCount);
		fb->List->Directory = entryResizeList(fb->List->Directory, fb->List->dirCount);
		if(fb->List->Files){
			printf("error removing entry\n");
			while(1);
		}
	}
}

static void rmdir_r(char *directory){
	DirWalk walk;
	memset(&walk, 0, sizeof(DirWalk));
	strcpy(walk.CurDir, directory);
	walk.curDirPos = 1;//start the walk
	walk.dir =  (DIR*)_openDir(directory, DIRTYPE_LIST);
	if(walk.dir == NULL)
		return;

	struct stat st;
	while(dirWalk(&walk, &st, WALK_RM) != NULL);
	_closeDir(&walk, DIRTYPE_LIST);	
}

/*===================================================================================
*   General API calls
* 
*
*
===================================================================================*/

int fsys_OpenDir(const char *dir, FileBrowseCore *fb, char walk){
	//then recollect the data
	RESET_FLAG(fb->flags, WALKDIR);
	if(walk)
		SET_FLAG(fb->flags, WALKDIR);

	fb->List = entryMakeList(dir, fb->Types, fb->flags);
	if(fb->List == NULL || fb->List->error)
		return 0;

	//sort directories
	if(fb->List->dirCount > 0)
		sortEntryList(fb->List->Directory, 0, fb->List->dirCount - 1);
	//then sort files
	if(fb->List->fileCount > 0)
		sortEntryList(fb->List->Files, 0, fb->List->fileCount - 1);
	return 1;
}

void fsys_CloseDir(FileBrowseCore *fb){
	//reset directory listing
	if(fb->List){
		entryListReset(fb->List);
		free(fb->List);
	}
	fb->List = NULL;
}

unsigned long fsys_getEntryCount(FileBrowseCore *fb){
	if(!fb->List)
		return 0;
	return fb->List->fileCount + fb->List->dirCount;
}

char fsys_isEntryDirFromNum(FileBrowseCore *fb, unsigned long fileNum){
	return GET_FLAG(entryGetFlag(getEntryFromNum(fileNum, fb)), ISDIR);
}


int fsys_ChangeDir(const char *dir, FileBrowseCore *fb){
	fsys_CloseDir(fb);
	return fsys_OpenDir(dir, fb, 0);
}

//go back one directory
int fsys_BackDir(FileBrowseCore *fb){
	//if we are on the root directory, we can't go back any more
	if(!strcasecmp(fb->List->curDir, "/"))
		return 0;
	dirBack(fb->List->curDir);
	char newPath[MAX_PATH];
	strcpy(newPath, fb->List->curDir);
	return fsys_ChangeDir(newPath, fb);
}

unsigned char fsys_getEntryFlags(unsigned long fileNum, FileBrowseCore *fb){
	Entry *temp = getEntryFromNum(fileNum, fb);
	if(temp == NULL)
		return 0;	
	return entryGetFlag(temp);
}

int fsys_getEntryNumber(FileBrowseCore *fb, const char *searchName){
	struct stat st;
	lstat(searchName, &st);
	//if searchName is a directory, scan dir list
	if(S_ISDIR(st.st_mode))
		return getEntryNum(fb->List->Directory, fb->List->dirCount, searchName);
	else
		return getEntryNum(fb->List->Files, fb->List->fileCount, searchName);	

	return -1;
}

Entry *fsys_getEntryFromNumber(FileBrowseCore *fb, unsigned long fileNum){
	return getEntryFromNum(fileNum, fb);
}

char *fsys_getEntryString(Entry *temp, FileBrowseCore *fb, unsigned char format){
	if(temp == NULL)
		return NULL;
	//clear name buffer first
	memset(fb->List->namebuf, 0, MAX_PATH);
	fb->List->namebuf[0] = '\0';

	//add directory path to string
	if(format & ENTRY_PATH){
		//check if there is a custom directory specified
		if(GET_FLAG(entryGetFlag(temp), CUSTOMDIR))
			strcat(fb->List->namebuf, entryGetPath(temp));
		else//there is no specified directory
			strcat(fb->List->namebuf, fb->List->curDir);

		int len = strlen(fb->List->namebuf);
		//remove trailing / unless requested.
		if(!(format & ENTRY_SYMBOLS)){
			while(len > 1 && (fb->List->namebuf[len] == '/' || fb->List->namebuf[len] == '\0'))
				fb->List->namebuf[len--] = '\0';
		}
		else{//make sure there is a / at the end of a path
			if(len > 2){
				while(len > 1 && fb->List->namebuf[len] == '\0')
					len--;
				if(fb->List->namebuf[len] != '/')
					strcat(fb->List->namebuf, "/");
			}
		}
	}

	//add file name to string
	if(format & ENTRY_NAME){
		strcat(fb->List->namebuf, entryGetName(temp));
		//file name is for a directory, add / to the end
		if(GET_FLAG(entryGetFlag(temp), ISDIR) && (format & ENTRY_SYMBOLS))
			strcat(fb->List->namebuf, "/");
	}

	//add file extension if available to string
	if(format & ENTRY_EXT){
		if(!GET_FLAG(entryGetFlag(temp), ISDIR)){
			if(format & ENTRY_SYMBOLS)
				strcat(fb->List->namebuf, ".");
			strcat(fb->List->namebuf, entryGetExt(temp));
		}
	}
	strcat(fb->List->namebuf, "\0");
	return fb->List->namebuf;
}

char *fsys_getEntryStringByNum(unsigned long fileNum, FileBrowseCore *fb, unsigned char format){
	return fsys_getEntryString(getEntryFromNum(fileNum, fb), fb, format);
}


void fsys_hideEntry(Entry *file, FileBrowseCore *fb, unsigned char hide){
	if(file == NULL)
		return;
	hideFile(fsys_getEntryString(file, fb, ENTRY_PATH | ENTRY_NAME | ENTRY_EXT), hide);
}

void fsys_hideEntryByNum(unsigned long fileNum, FileBrowseCore *fb, unsigned char hide){
	fsys_hideEntry(getEntryFromNum(fileNum, fb), fb, hide);
}

int fsys_dumpListToText(const char *destFile, FileBrowseCore *fb){
	//open file to dump to
	FILE *tempFile = fopen(destFile, "wb");
	if(!tempFile)
		return 0;

	unsigned long i = 0;
	char *file = NULL;
	do{
		file = fsys_getEntryStringByNum(i, fb, ENTRY_PATH | ENTRY_NAME | ENTRY_EXT | ENTRY_SYMBOLS);
		if(file)
			fprintf(tempFile, "%s\n", file);
		i++;
	}while(file != NULL);
	fclose(tempFile);
	return 1;
}

//delete a file in the file system
void fsys_remove(unsigned long fileNum, FileBrowseCore *fb){
	Entry *file = getEntryFromNum(fileNum, fb);
	if(!file)
		return;

	if(GET_FLAG(entryGetFlag(file), ISDIR)){
		//remove directory recursively
		rmdir_r(fsys_getEntryString(file, fb, ENTRY_PATH | ENTRY_NAME | ENTRY_SYMBOLS));
	}
	else{
		//remove the file from sd card
		remove(fsys_getEntryString(file, fb, ENTRY_PATH | ENTRY_NAME | ENTRY_EXT | ENTRY_SYMBOLS));
	}
	//remove from file listing
	removeEntry(fileNum, fb);
	//if file was removed from a file list, then update the list accordingly
	if(GET_FLAG(fb->List->flags, LIST_TXT))
		fsys_dumpListToText(fb->List->curDir, fb);
}


FileBrowseCore *fsys_Init(SupportedExt *ext, unsigned char flags){
	//allocate the core filebrowser
	FileBrowseCore *fb = calloc(1, sizeof(FileBrowseCore));
	if(fb == NULL)
		return NULL;
	fb->Types = ext;
	fb->flags = flags;
	fb->List = NULL;
	return fb;
}

u16 *fsys_GetFlags(FileBrowseCore *core){
	return (u16*)&core->flags;
}

void fsys_DeInit(FileBrowseCore *core){
	fsys_CloseDir(core);
	fsys_freeFileTypes(core->Types);
	core->flags = 0;
}

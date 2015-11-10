#include "file_system.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"
#include "path.hpp"
#include "thread_local_allocators.hpp"


#if defined(_WIN32)


#include <Windows.h>


bool IsValidDirectory(const char* folderPath)
{
	udtVMLinearAllocator& allocator = udtThreadLocalAllocators::GetTempAllocator();
	udtVMScopedStackAllocator allocatorScope(allocator);

	wchar_t* const wideFolderPath = udtString::ConvertToUTF16(allocator, udtString::NewConstRef(folderPath));
	const DWORD attribs = GetFileAttributesW(wideFolderPath);

	return (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));
}

bool GetDirectoryFileList(const udtFileListQuery& query)
{
	if(query.Files == NULL || 
	   query.FolderPath == NULL || 
	   query.PersistAllocator == NULL || 
	   query.TempAllocator == NULL)
	{
		return false;
	}

	if(query.Recursive && query.FolderArrayAllocator == NULL)
	{
		return false;
	}

	const udtString folderPath = udtString::NewConstRef(query.FolderPath);

	udtString queryPath;
	if(!udtPath::Combine(queryPath, *query.TempAllocator, folderPath, "*"))
	{
		return false;
	}

	wchar_t* const wideQueryPath = udtString::ConvertToUTF16(*query.TempAllocator, queryPath);
	WIN32_FIND_DATAW findData;
	const HANDLE findHandle = FindFirstFileW(wideQueryPath, &findData);
	if(findHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	udtVMArray<const char*> folders;
	if(query.Recursive)
	{
		query.FolderArrayAllocator->Clear();
		folders.SetAllocator(*query.FolderArrayAllocator);
	}
	do
	{
		// @NOTE: we can't create a temp alloc scope here because of
		// allocations necessary for sub-folder paths.

		udtString fileName = udtString::NewFromUTF16(*query.TempAllocator, findData.cFileName);
		if((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			if(query.Recursive && !udtString::Equals(fileName, ".") && !udtString::Equals(fileName, "..") != 0)
			{
				folders.Add(fileName.String);
			}
			continue;
		}

		const u64 fileSize = (u64)findData.nFileSizeLow + ((u64)findData.nFileSizeHigh << 32);
		if(query.FileFilter != NULL && !(*query.FileFilter)(fileName.String, fileSize, query.UserData))
		{
			continue;
		}

		udtString filePath;
		if(!udtPath::Combine(filePath, *query.TempAllocator, folderPath, fileName))
		{
			return false;
		}

		udtFileInfo info;
		info.Name = AllocateString(*query.PersistAllocator, fileName.String);
		info.Path = AllocateString(*query.PersistAllocator, filePath.String);
		info.Size = fileSize;
		query.Files->Add(info);
	}
	while(FindNextFileW(findHandle, &findData) != 0);

	FindClose(findHandle);

	if(query.Recursive)
	{
		for(u32 i = 0, count = folders.GetSize(); i < count; ++i)
		{
			udtString subFolderPath;
			if(!udtPath::Combine(subFolderPath, *query.TempAllocator, folderPath, folders[i]))
			{
				return false;
			}

			udtFileListQuery newQuery = query;
			newQuery.FolderPath = subFolderPath.String;
			if(!GetDirectoryFileList(newQuery))
			{
				return false;
			}
		}
	}

	return true;
}


#else


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>


bool IsValidDirectory(const char* folderPath)
{
	if(access(folderPath, 0) != 0)
	{
		return false;
	}

	struct stat status;
	stat(folderPath, &status);

	return (status.st_mode & S_IFDIR) != 0;
}

bool GetDirectoryFileList(const udtFileListQuery& query)
{
	if(query.Files == NULL || 
	   query.FolderPath == NULL || 
	   query.PersistAllocator == NULL || 
	   query.TempAllocator == NULL)
	{
		return false;
	}

	if(query.Recursive && query.FolderArrayAllocator == NULL)
	{
		return false;
	}

	DIR* const dirHandle = opendir(query.FolderPath);
	if(dirHandle == NULL)
	{
		return false;
	}

	udtVMArray<const char*> folders;
	if(query.Recursive)
	{
		query.FolderArrayAllocator->Clear();
		folders.SetAllocator(*query.FolderArrayAllocator);
	}
	const udtString folderPath = udtString::NewConstRef(query.FolderPath);
	
	struct dirent* dirEntry;
	while((dirEntry = readdir(dirHandle)) != NULL)
	{
		if((dirEntry->d_type & DT_DIR) != 0)
		{
			if(query.Recursive && strcmp(dirEntry->d_name, ".") != 0 && strcmp(dirEntry->d_name, "..") != 0)
			{
				folders.Add(AllocateString(*query.TempAllocator, dirEntry->d_name));
			}
			continue;
		}
		
		if((dirEntry->d_type & DT_REG) == 0)
		{
			// Not a regular file.
			continue;
		}
		
		udtString filePath;
		if(!udtPath::Combine(filePath, *query.TempAllocator, folderPath, dirEntry->d_name))
		{
			return false;
		}
		
		const u64 fileSize = udtFileStream::GetFileLength(filePath.String);
		if(query.FileFilter != NULL && !(*query.FileFilter)(dirEntry->d_name, fileSize, query.UserData))
		{
			continue;
		}

		udtFileInfo info;
		info.Name = AllocateString(*query.PersistAllocator, dirEntry->d_name);
		info.Path = AllocateString(*query.PersistAllocator, filePath.String);
		info.Size = fileSize;
		query.Files->Add(info);
	}

	closedir(dirHandle);

	if(query.Recursive)
	{
		for(u32 i = 0, count = folders.GetSize(); i < count; ++i)
		{
			udtString subFolderPath;
			if(!udtPath::Combine(subFolderPath, *query.TempAllocator, folderPath, folders[i]))
			{
				return false;
			}

			udtFileListQuery newQuery = query;
			newQuery.FolderPath = subFolderPath.String;
			if(!GetDirectoryFileList(newQuery))
			{
				return false;
			}
		}
	}

	return true;
}


#endif

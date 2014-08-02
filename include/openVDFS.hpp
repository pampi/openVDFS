/*
 * openVDFS - library for reading VDF archives used in Gothic, Gothic 2[+tNoR]
 * Copyright (C) 2014 pampi<at>github.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#ifndef OPENVDFS_HPP
#define OPENVDFS_HPP

#include <string>
#include <map>

#ifdef _WIN32
    #ifdef SHARED_LIBRARY_EXPORT
        #define VDFS_API    __declspec(dllexport)
    #else
        #define VDFS_API    __declspec(dllimport)
    #endif
#else
    #define VDFS_API
#endif

namespace vdfs {

#define DIRECTORY_ENTRY 0x80000000
#define LAST_ENTRY 0x40000000

    struct VolumeHeader{
        char comment[256];
        char version[16];
        unsigned int entryCount;
        unsigned int fileCount;
        unsigned int timeStamp;
        unsigned int size;
        unsigned int tableOffset;
        unsigned int entrySize;
    };

    struct VolumeEntry{
        char name[64];
        unsigned int offset;
        unsigned int size;
        unsigned int flags;
        unsigned int attributes;    //who care?
    };

    struct VDFS_API FileEntry
    {
        FileEntry(const char* file, unsigned int offset, unsigned int size, unsigned int timestamp);
        std::string file;
        unsigned int offset_in_file;
        unsigned int size;
        unsigned int timestamp;
    };

    struct VDFS_API DirectoryEntry
    {
        ~DirectoryEntry();

        void printAll(int depth=0);
        bool directoryEntryExist(const char* entry_name);
        bool fileExist(const char* path);
        const FileEntry *getFile(const char* file);

        std::map<std::string, FileEntry*> files;
        std::map<std::string, DirectoryEntry*> subdirectories;
    };

    class VDFS_API VDFS{
    public:
        VDFS(const char* real_path = nullptr);
        ~VDFS();

        bool addVDFVolume(const char* path);
        bool fileExist(const char* path);
        DirectoryEntry *getRootEntry() const;
        const FileEntry *getFileInfo(const char* file) const;
        unsigned char *getFile(const char* file);

        static VDFS* getInstance();
        static void destroyInstance();

    protected:
        static VDFS *global_instance;

    private:
        std::string sz_realPath;
        DirectoryEntry *rootDirectory;
    };
}

#endif // OPENVDFS_HPP

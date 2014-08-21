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

#include <cstdio>
#include <cstring>
#include <ctime>
#include <queue>
#include <openVDFS.hpp>

#define FREAD_BUFFER_SIZE 4096
#define COMMENT_SIZE 256
#define VALID_VERSION "PSVDSC_V2.00\r\n\r\n"

namespace vdfs {

VDFS *VDFS::global_instance = nullptr;


FileEntry::FileEntry(const char *file, unsigned int offset, unsigned int size, unsigned int timestamp)
{
    this->file = file;
    this->offset_in_file = offset;
    this->size = size;
    this->timestamp = timestamp;
}

DirectoryEntry::~DirectoryEntry()
{
    while(!files.empty())
    {
        auto del = files.begin();
        delete del->second;
        files.erase(del);
    }

    while(!subdirectories.empty())
    {
        auto del = subdirectories.begin();
        delete del->second;
        subdirectories.erase(del);
    }
}


VDFS::VDFS(const char *real_path)
{
    if (real_path)
        sz_realPath = real_path;

    rootDirectory = new DirectoryEntry;
}

VDFS::~VDFS()
{
    //do clean
    delete rootDirectory;
    sz_realPath.clear();
}

bool VDFS::addVDFVolume(const char *path)
{
    bool ret_val = false;
    VolumeHeader header;
    FILE *fh = fopen(path, "rb");

#ifdef DEBUG_LOG
    printf("VDFS: Trying to open %s vdfs volume...\n", path);
#endif

    if (fh) //volume exists
    {
        fread(header.comment, COMMENT_SIZE, 1, fh);
        for (unsigned char i=255; i>0; i--)
            header.comment[i] = (header.comment[i] == 0x1A) ? 0 : header.comment[i];

        if (!feof(fh))  //make sure we haven't reached EOF
        {
            fread(header.version, 16, 1, fh);

            ret_val = (bool)(memcmp(VALID_VERSION, header.version, 16) == 0);

            if (ret_val)
            {
                fread(&header.entryCount, 4, 1, fh);
                fread(&header.fileCount, 4, 1, fh);
                fread(&header.timeStamp, 4, 1, fh);
                fread(&header.size, 4, 1, fh);
                fread(&header.tableOffset, 4, 1, fh);
                fread(&header.entrySize, 4, 1, fh);

                if(feof(fh))
                    ret_val = false;
            }
        }

        if (ret_val)
        {
            VolumeEntry ve;
            //DirectoryEntry de;
            std::queue<DirectoryEntry*> entries_to_fill;
            entries_to_fill.push(rootDirectory);
#ifdef DEBUG_LOG
            time_t timestamp = (time_t)header.timeStamp;
            printf("---STATS---\nComment: %s\n\nEntries: %u\nFiles: %u\nTime stamp: %s\nSize: %u\nTable offset: %X\nEntry size: %u\n\n",
                   header.comment, header.entryCount, header.fileCount, ctime(&timestamp), header.size, header.tableOffset, header.entrySize);

            puts("Reading entries...");
#endif
            for(unsigned int i = 0; i < header.entryCount; i++)
            {
                DirectoryEntry *top = entries_to_fill.front();
                fread(ve.name, 64, 1, fh);
                for(unsigned int j = 0; j < 64; j++)
                    ve.name[j] = (ve.name[j] == 0x20) ? 0 : ve.name[j];

                fread(&ve.offset, 4, 1, fh);
                fread(&ve.size, 4, 1, fh);
                fread(&ve.flags, 4, 1, fh);
                fread(&ve.attributes, 4, 1, fh);

                if(ve.flags & DIRECTORY_ENTRY)
                {
                    if(!top->directoryEntryExist(ve.name))
                    {
                        std::string name = ve.name;
                        DirectoryEntry *de = new DirectoryEntry;
                        top->subdirectories[name] = de;
                        entries_to_fill.push(de);
                    }
                    else
                    {
                        std::string name = ve.name;
                        entries_to_fill.push(top->subdirectories[name]);
                    }
                }
                else
                {
                    std::string name = ve.name;

                    if(!top->fileExist(ve.name))
                    {
                        FileEntry *fe = new FileEntry(path, ve.offset, ve.size, header.timeStamp);
                        top->files[name] = fe;
                    }
                    else if (top->files[name]->timestamp < header.timeStamp)
                    {
                        delete top->files[name];
                        top->files[name] = new FileEntry(path, ve.offset, ve.size, header.timeStamp);
                    }
                }

                if(ve.flags & LAST_ENTRY)
                    entries_to_fill.pop();

#ifdef DEBUG_LOG
                printf("Entry name: %s\nOffset: %u\nSize: %u\nIs directory? %s\nLast entry? %s\nAttributes: %X\n\n",
                       ve.name, ve.offset, ve.size, (ve.flags & DIRECTORY_ENTRY) ? "YES" : "NO",
                       (ve.flags & LAST_ENTRY) ? "YES" : "NO", ve.attributes);
#endif
            }
        }
        fclose(fh);
    }

#ifdef DEBUG_LOG
    if (ret_val)
        puts("VDFS: OK!");
    else
        puts("VDFS: Invalid archive!");
#endif

    return ret_val;
}

VDFS* VDFS::getInstance()
{
    if (!global_instance)
        global_instance = new VDFS();

    return global_instance;
}

void VDFS::destroyInstance()
{
    delete global_instance;
    global_instance = nullptr;
}

bool DirectoryEntry::fileExist(const char *path)
{
    bool ret_val = false;
    std::string dir = path;

    if (dir.find('/') == std::string::npos)
    {
        ret_val = (files.find(dir) != files.end());
    }
    else
    {
        std::string dir_name = dir.substr(0, dir.find_first_of('/'));
        dir = dir.substr(dir.find_first_of('/') + 1);

        if (subdirectories.find(dir_name) != subdirectories.end())
        {
            ret_val = subdirectories[dir_name]->fileExist(dir.c_str());
        }
    }
    return ret_val;
}

bool DirectoryEntry::directoryEntryExist(const char *entry_name)
{
    std::string name = entry_name;
    return (bool)(subdirectories.find(name) != subdirectories.end());
}

bool VDFS::fileExist(const char *entry_name)
{
    std::string path = entry_name;
    while (path.find_first_of('\\') != std::string::npos)
        path[path.find_first_of('\\')] = '/';

    return (path.find('/') == std::string::npos) ? (rootDirectory->files.find(path) == rootDirectory->files.end()) : rootDirectory->fileExist(path.c_str());
}

void DirectoryEntry::printAll(int depth)
{
    for(auto& fe : files)
    {
        for(int i=0; i<depth; i++)
            printf("\t");
        printf("%s\n", fe.first.c_str());
    }

    for(auto& de : subdirectories)
    {
        for(int i=0; i<depth; i++)
            printf("\t");
        printf("%s\\\n", de.first.c_str());
        de.second->printAll(depth + 1);
    }
}

const FileEntry *DirectoryEntry::getFile(const char* file)
{
    const FileEntry *ret_val = nullptr;
    std::string path = file;

    if (path.find_first_of('/') == std::string::npos)
    {
        if(files.find(path) != files.end())
        {
            ret_val = files[path];
        }
    }
    else    //dive into directories
    {
        std::string dir = path.substr(0, path.find_first_of('/'));
        path = path.substr(path.find_first_of('/') + 1);

        if(subdirectories.find(dir) != subdirectories.end())
        {
            ret_val = subdirectories[dir]->getFile(path.c_str());
        }
    }

    return ret_val;
}

const FileEntry *VDFS::getFileInfo(const char* file) const
{
    const FileEntry *ret_val = nullptr;
    std::string path = file;

    while(path.find_first_of('\\') != std::string::npos)
        path[path.find('\\')] = '/';

    ret_val = rootDirectory->getFile(file);

    return ret_val;
}

unsigned char *VDFS::getFile(const char *file)
{
    unsigned char* ret_val = nullptr;
    const FileEntry *fe = getFileInfo(file);

    if (fe && (fe->size > 0))
    {
        FILE *fh = fopen(fe->file.c_str(), "rb");
        if (fh)
        {
            unsigned int bytes_read = 0;
            ret_val = new unsigned char[fe->size];

			fseek(fh, fe->offset_in_file, SEEK_SET);

            while (bytes_read + FREAD_BUFFER_SIZE < fe->size)
            {
                fread(ret_val + bytes_read, FREAD_BUFFER_SIZE, 1, fh);
                bytes_read += FREAD_BUFFER_SIZE;
            }

            fread(ret_val + bytes_read, fe->size - bytes_read, 1, fh);
            fclose(fh);
        }
    }

    return ret_val;
}

DirectoryEntry *VDFS::getRootEntry() const
{
    return rootDirectory;
}

}


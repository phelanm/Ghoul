/*****************************************************************************************
 *                                                                                       *
 * GHOUL                                                                                 *
 * General Helpful Open Utility Library                                                  *
 *                                                                                       *
 * Copyright (c) 2012-2014                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#include <ghoul/filesystem/filesystem>
#include <iostream>
#include <fstream>

#ifdef WIN32
#include <windows.h>
#endif

TEST(FileSystemTest, HasTestDirectory) {
    EXPECT_EQ(FileSys.directoryExists("${TEMPORARY}"), true);
}

TEST(FileSystemTest, CreateRemoveDirectory) {
    using ghoul::filesystem::FileSystem;

    const std::string tmp = absPath("${TEMPORARY}/tmp");
    const std::string tmpRecursive1 = absPath("${TEMPORARY}/tmp/tmp2");
    const std::string tmpRecursive2 = absPath("${TEMPORARY}/tmp/tmp2/tmp3");

    EXPECT_NO_THROW(FileSys.createDirectory(tmp));
    EXPECT_THROW(FileSys.createDirectory(tmpRecursive2), FileSystem::FileSystemException);
    EXPECT_NO_THROW(FileSys.createDirectory(tmpRecursive2, FileSystem::Recursive::Yes));

    EXPECT_THROW(FileSys.deleteDirectory(tmp), FileSystem::FileSystemException);
    EXPECT_NO_THROW(FileSys.deleteDirectory(tmpRecursive2));
    EXPECT_THROW(FileSys.deleteDirectory(tmp), FileSystem::FileSystemException);
    EXPECT_NO_THROW(FileSys.deleteDirectory(tmp, FileSystem::Recursive::Yes));
}

TEST(FileSystemTest, Path) {
    using ghoul::filesystem::File;

    std::string path = "${TEMPORARY}/tmpfil.txt";
    std::string abspath = absPath(path);

    File* f1 = new File(path);
    File* f2 = new File(path, File::RawPath::Yes);
    File* f3 = new File(abspath, File::RawPath::Yes);

    EXPECT_NE(f1->path(), f2->path());
    EXPECT_NE(f2->path(), f3->path());
    EXPECT_EQ(f1->path(), f3->path());

    delete f3;
    delete f2;
    delete f1;
}

TEST(FileSystemTest, OnChangeCallback) {
    using ghoul::filesystem::File;
    using ghoul::filesystem::FileSystem;

    const char* cpath = "${TEMPORARY}/tmpfil.txt";
    std::string path = absPath(cpath);
    std::ofstream f;
    f.open(path);
    f << "tmp";
    f.close();
    bool b1 = false;
    bool b2 = false;

    auto c1 = [&b1](const File&) {
        b1 = true;
    };
    auto c2 = [&b2](const File&) {
        b2 = true;
    };

    File* f1 = new File(path, File::RawPath::No, c1);
    File* f2 = new File(path, File::RawPath::No, c1);
    File* f3 = new File(path, File::RawPath::No, c2);

    // Check that the file exists
    EXPECT_EQ(FileSys.fileExists(cpath, FileSystem::RawPath::No), true);
    EXPECT_EQ(FileSys.fileExists(path), true);
    EXPECT_EQ(FileSys.fileExists(*f1), true);

    f2->setCallback(nullptr);

    // Check that b still is false so no callback has been fired
    EXPECT_EQ(b1, false);
    EXPECT_EQ(b2, false);

    // overwrite the file
    f.open(path);
    f << "tmp";
    f.close();
    FileSys.triggerFilesystemEvents();

    // Sleep the main thread to make sure the filesystem have time to respond
    const int seconds = 4;
#ifdef WIN32
    int count = 0;
    while ((b1 == false || b2 == false) && count < 100 * seconds) {
        Sleep(10);
        ++count;
    }
#else
    int count = 0;
    while ((b1 == false || b2 == false) && count < 10000 * seconds) {
        usleep(100);
        FileSys.triggerFilesystemEvents();
        ++count;
    }
#endif
    EXPECT_EQ(b1, true);
    EXPECT_EQ(b2, true);

    delete f3;
    delete f2;
    delete f1;

    // Check that we can delete the file
    EXPECT_EQ(FileSys.deleteFile(path), true);
}

TEST(FileSystemTest, OverrideNonExistingPathToken) {
    EXPECT_NO_THROW(
        FileSys.registerPathToken(
            "${AddExistingPathToken}",
            absPath("${TEMPORARY}"),
            ghoul::filesystem::FileSystem::Override::Yes
        )
    );
}

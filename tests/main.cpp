/*****************************************************************************************
 *                                                                                       *
 * GHOUL                                                                                 *
 * General Helpful Open Utility Library                                                  *
 *                                                                                       *
 * Copyright (c) 2012 Alexander Bock                                                     *
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

#include "gtest/gtest.h"

#include <ghoul/cmdparser/cmdparser>
#include <ghoul/filesystem/filesystem>
#include <ghoul/logging/logging>

#include "tests/test_common.inl"
#include "tests/test_configurationmanager.inl"
#include "tests/test_commandlineparser.inl"

using namespace ghoul::cmdparser;
using namespace ghoul::filesystem;
using namespace ghoul::logging;

int main(int argc, char** argv) {
/*    FileSystem::initialize();
    
    LogManager::initialize(LogManager::LogLevel::Info);
    LogMgr.addLog(new ConsoleLog);

    
    std::vector<std::string> additionalArguments;
    CommandlineParser* p = new CommandlineParser("GhoulTest", true);
    std::string scriptsDir = "";
    std::string testsDir = "";
    p->addCommand(new SingleCommand<std::string>(&scriptsDir, "-scripts"));
    p->addCommand(new SingleCommand<std::string>(&testsDir, "-tests"));
    p->setCommandLine(argc, argv, &additionalArguments);
    p->execute();

    if (scriptsDir == "")
#ifdef WIN32
        FileSys.registerPathToken("${SCRIPTS}", "../scripts");
#else
        FileSys.registerPathToken("${SCRIPTS}", "../../scripts");
#endif
    else
        FileSys.registerPathToken("${SCRIPTS}", scriptsDir);
    
    if (testsDir == "")
#ifdef WIN32
        FileSys.registerPathToken("${TEST_DIR}" , "../tests");
#else
        FileSys.registerPathToken("${TEST_DIR}" , "../../tests");
#endif
    else
        FileSys.registerPathToken("${TEST_DIR}", testsDir);
    
    LFATALC("", "Scripts: " << scriptsDir);
    LFATALC("", "Tests: " << testsDir);
    
    LFATALC("", "Additional Arguments");
    for (const std::string& arg : additionalArguments)
        LFATALC("", arg);

 */
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
/* KDevelop QMake Support
 *
 * Copyright 2006 Andreas Pakulat <apaku@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "qmakedriver.h"

#include <KAboutData>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

int main(int argc, char* argv[])
{
    KAboutData aboutData(QLatin1String("QMake Parser"), "qmake-parser", QLatin1String("1.0"));
    aboutData.setShortDescription("Parse QMake project files");
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.addOption(QCommandLineOption(QLatin1String("debug"), "Enable output of the debug AST"));
    parser.addPositionalArgument("files", "QMake project files");

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    if (parser.positionalArguments().isEmpty()) {
        parser.showHelp();
        return EXIT_FAILURE;
    }

    const bool debug = parser.isSet("debug");

    foreach (const auto arg, parser.positionalArguments()) {
        QMake::Driver driver;
        if (!driver.readFile(arg)) {
            exit(EXIT_FAILURE);
        }
        driver.setDebug(debug);

        QMake::ProjectAST* ast = nullptr;
        if (!driver.parse(&ast)) {
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}


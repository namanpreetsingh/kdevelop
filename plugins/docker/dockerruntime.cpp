/*
   Copyright 2017 Aleix Pol Gonzalez <aleixpol@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "dockerruntime.h"
#include "dockerpreferencessettings.h"
#include "debug_docker.h"

#include <interfaces/icore.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/iproject.h>
#include <project/interfaces/ibuildsystemmanager.h>

#include <KLocalizedString>
#include <KProcess>
#include <KActionCollection>
#include <KShell>
#include <QProcess>
#include <QDir>
#include <QDebug>
#include <outputview/outputexecutejob.h>

using namespace KDevelop;

DockerPreferencesSettings* DockerRuntime::s_settings = nullptr;

DockerRuntime::DockerRuntime(const QString &tag)
    : KDevelop::IRuntime()
    , m_tag(tag)
{
}

DockerRuntime::~DockerRuntime()
{
}

void DockerRuntime::setEnabled(bool /*enable*/)
{
}

QString ensureEndsSlash(const QString &string)
{
    return string.endsWith('/') ? string : (string + QLatin1Char('/'));
}

static QStringList projectVolumes()
{
    QStringList ret;
    const QString dir = ensureEndsSlash(DockerRuntime::s_settings->projectsVolume());
    const QString buildDir = ensureEndsSlash(DockerRuntime::s_settings->buildDirsVolume());

    for(IProject* project: ICore::self()->projectController()->projects()) {
        const Path path = project->path();
        if (path.isLocalFile()) {
            ret << "--volume" << QStringLiteral("%1:%2").arg(path.toLocalFile(), dir + path.lastPathSegment());
        }

        const auto ibsm = project->buildSystemManager();
        if (ibsm)
            ret << "--volume" << QStringLiteral("%1:%2").arg(path.toLocalFile(), buildDir + path.lastPathSegment());
    }
    return ret;
}

void DockerRuntime::startProcess(QProcess* process)
{
    const QStringList args = QStringList{QStringLiteral("run"), "--rm"} << KShell::splitArgs(s_settings->extraArguments()) << projectVolumes() << m_tag << process->program() << process->arguments();
    process->setProgram("docker");
    process->setArguments(args);

    qCDebug(DOCKER) << "starting qprocess" << process->program() << process->arguments();
    process->start();
}

void DockerRuntime::startProcess(KProcess* process)
{
    process->setProgram(QStringList{ "docker", "run", "--rm" } << KShell::splitArgs(s_settings->extraArguments()) << projectVolumes() << m_tag << process->program());

    qCDebug(DOCKER) << "starting kprocess" << process->program().join(' ');
    process->start();
}

KDevelop::Path DockerRuntime::pathInHost(const KDevelop::Path& runtimePath)
{
    Path ret = runtimePath;
    const Path projectsDir(DockerRuntime::s_settings->projectsVolume());
    if (projectsDir.isParentOf(runtimePath)) {
        const auto relPath = runtimePath.relativePath(projectsDir);
        const int index = relPath.indexOf(QLatin1Char('/'));
        auto project = ICore::self()->projectController()->findProjectByName(relPath.left(index));
        if (!project) {
            qCWarning(DOCKER) << "No project for" << relPath;
        } else {
            const auto repPathProject = relPath.mid(index+1);
            ret = Path(project->path(), repPathProject);;
        }
    }
    return ret;
}

KDevelop::Path DockerRuntime::pathInRuntime(const KDevelop::Path& localPath)
{
    auto project = ICore::self()->projectController()->findProjectForUrl(localPath.toUrl());
    if (project) {
        const Path projectsDir(DockerRuntime::s_settings->projectsVolume());
        const QString relpath = project->path().relativePath(localPath);
        const KDevelop::Path ret(projectsDir, project->name() + QLatin1Char('/') + relpath);
        qDebug() << "docker pathInRuntime..." << ret << project->path() << relpath;
        return ret;
    } else {
        qCWarning(DOCKER) << "only project files are available on the docker runtime" << localPath;
    }
    return localPath;
}


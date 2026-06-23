/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "memfilesystem.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

#include "global/containers.h"
#include "global/stringutils.h"

#include "log.h"

using namespace muse::io;

//! NOTE Very simple implementation without directory support

static bool isRC(const muse::io::path_t& path)
{
    return muse::strings::startsWith(path.toStdString(), ":/");
}

muse::Ret MemFileSystem::exists(const muse::io::path_t& path) const
{
    if (isRC(path)) {
        return QFile::exists(path.toQString());
    }
    if (muse::contains(m_files, path)) {
        return true;
    }
    // Fall through to MEMFS: locale .qm files / languages.json are injected
    // by the JS preRun via FS.writeFile and live there, not in m_files.
    return QFile::exists(path.toQString());
}

muse::Ret MemFileSystem::remove(const muse::io::path_t& path, bool onlyIfEmpty)
{
    UNUSED(onlyIfEmpty);
    if (isRC(path)) {
        UNREACHABLE;
        return muse::Ret();
    } else {
        muse::remove(m_files, path);
        return muse::make_ok();
    }
}

muse::Ret MemFileSystem::clear(const muse::io::path_t& path)
{
    if (isRC(path)) {
        UNREACHABLE;
        return muse::Ret();
    } else {
        muse::remove(m_files, path);
        return muse::make_ok();
    }
}

muse::Ret MemFileSystem::copy(const muse::io::path_t& src, const muse::io::path_t& dst, bool replace)
{
    if (isRC(src) || isRC(dst)) {
        NOT_SUPPORTED;
        return muse::Ret();
    }

    if (replace) {
        m_files[dst] = m_files.at(src);
    } else {
        if (!muse::contains(m_files, dst)) {
            m_files[dst] = m_files.at(src);
        }
    }
    return muse::make_ok();
}

muse::Ret MemFileSystem::move(const muse::io::path_t& src, const muse::io::path_t& dst, bool replace)
{
    if (isRC(src) || isRC(dst)) {
        NOT_SUPPORTED;
        return muse::Ret();
    }

    if (replace) {
        m_files[dst] = m_files.at(src);
        muse::remove(m_files, src);
    } else {
        if (!muse::contains(m_files, dst)) {
            m_files[dst] = m_files.at(src);
            muse::remove(m_files, src);
        }
    }
    return muse::make_ok();
}

muse::Ret MemFileSystem::makePath(const muse::io::path_t& path) const
{
    // Create the directory in MEMFS so writes (e.g. user locale overrides at
    // /files/data/locale/) can land there. QDir::mkpath is a no-op if it
    // already exists.
    if (QDir().mkpath(path.toQString())) {
        return muse::make_ok();
    }
    return muse::make_ret(muse::Ret::Code::UnknownError);
}

muse::Ret MemFileSystem::makeLink(const muse::io::path_t& /*targetPath*/, const muse::io::path_t& /*linkPath*/) const
{
    NOT_IMPLEMENTED;
    return muse::make_ret(muse::Ret::Code::NotImplemented);
}

muse::io::EntryType MemFileSystem::entryType(const muse::io::path_t& /*path*/) const
{
    NOT_IMPLEMENTED;
    return muse::io::EntryType::File;
}

muse::RetVal<uint64_t> MemFileSystem::fileSize(const muse::io::path_t& path) const
{
    muse::RetVal<uint64_t> rv;
    rv.ret = exists(path);
    if (!rv.ret) {
        return rv;
    }

    if (isRC(path)) {
        QFileInfo fi(path.toQString());
        return muse::RetVal<uint64_t>::make_ok(fi.size());
    } else {
        return muse::RetVal<uint64_t>::make_ok(m_files.at(path).size());
    }
}

muse::RetVal<muse::io::paths_t> MemFileSystem::scanFiles(const muse::io::path_t& rootDir, const std::vector<std::string>& filters,
                                                         muse::io::ScanMode mode) const
{
    // MEMFS-backed scan via QDirIterator — needed for LanguagesService which
    // enumerates *_<code>.qm under /files/share/locale/ at startup.
    muse::RetVal<muse::io::paths_t> result;
    muse::Ret ret = exists(rootDir);
    if (!ret) {
        result.ret = ret;
        return result;
    }

    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    QDir::Filters dirFilters = QDir::NoDotAndDotDot | QDir::Readable;

    switch (mode) {
    case muse::io::ScanMode::FilesInCurrentDir:
        dirFilters |= QDir::Files;
        break;
    case muse::io::ScanMode::FilesAndFoldersInCurrentDir:
        dirFilters |= QDir::Files | QDir::Dirs;
        break;
    case muse::io::ScanMode::FilesInCurrentDirAndSubdirs:
        flags |= QDirIterator::Subdirectories;
        dirFilters |= QDir::Files;
        break;
    }

    QStringList qnameFilters;
    for (const std::string& f : filters) {
        qnameFilters << QString::fromStdString(f);
    }

    QDirIterator it(rootDir.toQString(), qnameFilters, dirFilters, flags);
    while (it.hasNext()) {
        result.val.push_back(it.next());
    }

    result.ret = muse::make_ok();
    return result;
}

void MemFileSystem::setAttribute(const muse::io::path_t& /*path*/, Attribute /*attribute*/) const
{
    NOT_IMPLEMENTED;
}

bool MemFileSystem::setPermissionsAllowedForAll(const muse::io::path_t& /*path*/) const
{
    NOT_IMPLEMENTED;
    return false;
}

muse::RetVal<muse::ByteArray> MemFileSystem::readFile(const muse::io::path_t& path) const
{
    muse::RetVal<muse::ByteArray> rv;
    rv.ret = readFile(path, rv.val);
    return rv;
}

muse::Ret MemFileSystem::readFile(const muse::io::path_t& path, muse::ByteArray& data) const
{
    muse::Ret ret = exists(path);
    if (!ret) {
        return ret;
    }

    if (isRC(path)) {
        QFile file(path.toQString());
        file.open(QIODevice::ReadOnly);
        qint64 size = file.size();
        data.resize(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(data.data()), size);
    } else if (muse::contains(m_files, path)) {
        data = m_files.at(path);
    } else {
        // MEMFS-backed (e.g. locale assets injected by the JS preRun).
        QFile file(path.toQString());
        if (!file.open(QIODevice::ReadOnly)) {
            return muse::make_ret(muse::Ret::Code::UnknownError);
        }
        qint64 size = file.size();
        data.resize(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(data.data()), size);
    }

    return muse::make_ok();
}

muse::Ret MemFileSystem::writeFile(const muse::io::path_t& path, const muse::ByteArray& data)
{
    if (isRC(path)) {
        UNREACHABLE;
        return muse::Ret();
    }
    m_files[path] = data;
    return muse::make_ok();
}

muse::io::path_t MemFileSystem::canonicalFilePath(const muse::io::path_t& path) const
{
    return path;
}

muse::io::path_t MemFileSystem::absolutePath(const muse::io::path_t& path) const
{
    return path;
}

muse::io::path_t MemFileSystem::absoluteFilePath(const muse::io::path_t& path) const
{
    return path;
}

muse::DateTime MemFileSystem::birthTime(const muse::io::path_t& /*path*/) const
{
    NOT_IMPLEMENTED;
    return muse::DateTime();
}

muse::DateTime MemFileSystem::lastModified(const muse::io::path_t& /*path*/) const
{
    NOT_IMPLEMENTED;
    return muse::DateTime();
}

muse::Ret MemFileSystem::isWritable(const muse::io::path_t& /*path*/) const
{
    return muse::make_ok();
}

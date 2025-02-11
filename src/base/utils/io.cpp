/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2020  Mike Tzou (Chocobo1)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link this program with the OpenSSL project's "OpenSSL" library (or with
 * modified versions of it that use the same license as the "OpenSSL" library),
 * and distribute the linked executables. You must obey the GNU General Public
 * License in all respects for all of the code used other than "OpenSSL".  If you
 * modify file(s), you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version.
 */

#include "io.h"

#include <libtorrent/bencode.hpp>
#include <libtorrent/entry.hpp>

#include <QByteArray>
#include <QFileDevice>
#include <QSaveFile>
#include <QString>

#include "base/path.h"
#include "base/utils/fs.h"

Utils::IO::FileDeviceOutputIterator::FileDeviceOutputIterator(QFileDevice &device, const int bufferSize)
    : m_device {&device}
    , m_buffer {std::make_shared<QByteArray>()}
    , m_bufferSize {bufferSize}
{
    m_buffer->reserve(bufferSize);
}

Utils::IO::FileDeviceOutputIterator::~FileDeviceOutputIterator()
{
    if (m_buffer.use_count() == 1)
    {
        if (m_device->error() == QFileDevice::NoError)
            m_device->write(*m_buffer);
        m_buffer->clear();
    }
}

Utils::IO::FileDeviceOutputIterator &Utils::IO::FileDeviceOutputIterator::operator=(const char c)
{
    m_buffer->append(c);
    if (m_buffer->size() >= m_bufferSize)
    {
        if (m_device->error() == QFileDevice::NoError)
            m_device->write(*m_buffer);
        m_buffer->clear();
    }
    return *this;
}

nonstd::expected<void, QString> Utils::IO::saveToFile(const Path &path, const QByteArray &data)
{
    if (const Path parentPath = path.parentPath(); !parentPath.isEmpty())
        Utils::Fs::mkpath(parentPath);
    QSaveFile file {path.data()};
    if (!file.open(QIODevice::WriteOnly) || (file.write(data) != data.size()) || !file.flush() || !file.commit())
        return nonstd::make_unexpected(file.errorString());
    return {};
}

nonstd::expected<void, QString> Utils::IO::saveToFile(const Path &path, const lt::entry &data)
{
    QSaveFile file {path.data()};
    if (!file.open(QIODevice::WriteOnly))
        return nonstd::make_unexpected(file.errorString());

    const int bencodedDataSize = lt::bencode(Utils::IO::FileDeviceOutputIterator {file}, data);
    if ((file.size() != bencodedDataSize) || !file.flush() || !file.commit())
        return nonstd::make_unexpected(file.errorString());

    return {};
}

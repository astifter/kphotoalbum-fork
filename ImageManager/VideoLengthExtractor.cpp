/* Copyright 2012  Jesper K. Pedersen <blackie@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "VideoLengthExtractor.h"
#include <Utilities/Process.h>
#include <QDir>
#include <MainWindow/FeatureDialog.h>
#include <KDebug>
#include <QDebug>

#define STR(x) QString::fromUtf8(x)

#if 0
#  define Debug qDebug
#else
#  define Debug if(0) qDebug
#endif

ImageManager::VideoLengthExtractor::VideoLengthExtractor(QObject *parent) :
    QObject(parent), m_process(nullptr)
{
}

void ImageManager::VideoLengthExtractor::extract(const DB::FileName &fileName)
{
    m_fileName = fileName;
    if ( m_process ) {
        disconnect( m_process, SIGNAL(finished(int)), this, SLOT(processEnded()));
        m_process->kill();
        delete m_process;
        m_process = nullptr;
    }

    if (MainWindow::FeatureDialog::mplayerBinary().isEmpty()) {
        emit unableToDetermineLength();
        return;
    }

    m_process = new Utilities::Process(this);
    m_process->setWorkingDirectory(QDir::tempPath());
    connect( m_process, SIGNAL(finished(int)), this, SLOT(processEnded()));

    QStringList arguments;
    arguments << STR("-identify") << STR("-frames") << STR("0") << STR("-vc") << STR("null")
              << STR("-vo") << STR("null") << STR("-ao") << STR("null") <<  fileName.absolute();

    m_process->start(MainWindow::FeatureDialog::mplayerBinary(), arguments);
}

void ImageManager::VideoLengthExtractor::processEnded()
{
    if ( !m_process->stderr().isEmpty() )
        Debug() << m_process->stderr();

    QStringList list = m_process->stdout().split(QChar::fromLatin1('\n'));
    list = list.filter(STR("ID_LENGTH="));
    if ( list.count() == 0 ) {
        kWarning() << "Unable to find ID_LENGTH in output from MPlayer for file " << m_fileName.absolute() << "\n"
                   << "Output was:\n"
                   << m_process->stdout();
        emit unableToDetermineLength();
        return;
    }

    const QString match = list[0];
    const QRegExp regexp(STR("ID_LENGTH=([0-9.]+)"));
    bool ok = regexp.exactMatch(match);
    if ( !ok ) {
        kWarning() << STR("Unable to match regexp for string: %1 (for file %2)").arg(match).arg(m_fileName.absolute());
        emit unableToDetermineLength();
        return;
    }

    const QString cap = regexp.cap(1);

    const double length = cap.toDouble(&ok);
    if ( !ok ) {
        kWarning() << STR("Unable to convert string \"%1\"to integer (for file %2)").arg(cap).arg(m_fileName.absolute());
        emit unableToDetermineLength();
        return;
    }

    if ( length == 0 ) {
        kWarning() << "video length returned was 0 for file " << m_fileName.absolute();
        emit unableToDetermineLength();
        return;
    }

    emit lengthFound(length);
    m_process->deleteLater();
    m_process = nullptr;
}
// vi:expandtab:tabstop=4 shiftwidth=4:

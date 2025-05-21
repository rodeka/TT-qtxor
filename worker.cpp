#include "worker.h"
#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QDateTime>

Worker::Worker(const QStringList &masks, const QString &outDir, bool removeInput, bool overwrite, quint64 key)
    : m_masks(masks)
    , m_outDir(outDir)
    , m_removeInput(removeInput)
    , m_overwrite(overwrite)
    , m_key(key)
{}

quint64 static swapEndian(quint64 value) {
    return  ((value & 0xFF00000000000000ULL) >> 56) |
           ((value & 0x00FF000000000000ULL) >> 40) |
           ((value & 0x0000FF0000000000ULL) >> 24) |
           ((value & 0x000000FF00000000ULL) >> 8)  |
           ((value & 0x00000000FF000000ULL) << 8)  |
           ((value & 0x0000000000FF0000ULL) << 24) |
           ((value & 0x000000000000FF00ULL) << 40) |
           ((value & 0x00000000000000FFULL) << 56);
}

void Worker::process()
{
    QDir dir;
    QFileInfoList fileList;
    for(const QString &mask : m_masks){
        fileList += dir.entryInfoList(QStringList(mask), QDir::Files);
    }

    qint64 totalBytes = 0;
    for(const QFileInfo &info : fileList){
        totalBytes += info.size();
    }

    qint64 processedBytes = 0;
    const qint64 BUF_SIZE = 1024*1024;
    QByteArray buffer;
    buffer.resize(BUF_SIZE);

    for(const QFileInfo &info : fileList){
        QString inPath = info.absoluteFilePath();
        QString baseName = info.completeBaseName();
        QString suffix = info.suffix();
        QString outPath = QDir(m_outDir).filePath(info.fileName());

        if(!m_overwrite){
            while(QFile::exists(outPath)){
                outPath = QDir(m_outDir).filePath(
                    QString("%1_%2.%3").arg(baseName).arg(QDateTime::currentSecsSinceEpoch()).arg(suffix));
            }
        }

        QFile inFile(inPath);
        QFile outFile(outPath);

        if(!inFile.open(QIODevice::ReadOnly) || !outFile.open(QIODevice::WriteOnly)){
            continue;
        }

        while(!inFile.atEnd()){
            qint64 readBytes = inFile.read(buffer.data(), BUF_SIZE);
            quint64 *dataPtr = reinterpret_cast<quint64*>(buffer.data());
            qint64 chunks = readBytes / sizeof(quint64);
            for(qint64 i = 0; i < chunks; ++i){
                dataPtr[i] = swapEndian(swapEndian(dataPtr[i] ^ m_key));
            }
            outFile.write(buffer.constData(), readBytes);
            processedBytes += readBytes;
            emit progress(processedBytes, totalBytes);
        }

        inFile.close();
        outFile.close();

        if(m_removeInput)
            QFile::remove(inPath);
    }
    emit finished();
}

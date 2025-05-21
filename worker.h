#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QStringList>
#include <QString>

class Worker : public QObject
{
    Q_OBJECT
public:
    Worker(const QStringList &masks, const QString &outDir, bool removeInput, bool overwrite, quint64 key);

public slots:
    void process();

signals:
    void progress(quint64 processed, quint64 total);
    void finished();

private:
    QStringList m_masks;
    QString m_outDir;
    bool m_removeInput;
    bool m_overwrite;
    quint64 m_key;
};

#endif // WORKER_H

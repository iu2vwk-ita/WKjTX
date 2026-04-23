#ifndef WKJTX_QRZ_DOWNLOADER_HPP
#define WKJTX_QRZ_DOWNLOADER_HPP

// QrzDownloader — v1.2.0. HTTPS FETCH from qrz.com Logbook API.
//
// POST https://logbook.qrz.com/api with form-urlencoded body:
//     KEY=<api>&ACTION=FETCH&OPTION=TYPE:ADIF[,MODSINCE:yyyy-MM-dd HH:mm:ss]
//
// Server responds with `&`-separated key=value pairs and an ADIF blob
// in the ADIF= field. Per the qrz.com docs, RESULT=OK is set on
// success and the ADIF= field contains the concatenated records.
//
// Reference: https://www.qrz.com/docs/logbook/QRZLogbookAPI.html

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QDateTime>

class QNetworkAccessManager;

namespace wkjtx {

class QrzDownloader : public QObject
{
    Q_OBJECT
public:
    explicit QrzDownloader (QObject * parent = nullptr);
    ~QrzDownloader () override;

    void setApiKey (QString const & key);

    // Kick off a fetch. If `since` is valid, only records modified
    // after that UTC timestamp are requested (MODSINCE option);
    // otherwise the entire logbook is pulled (expensive on first run
    // for active operators).
    void fetch (QDateTime const & since = {});

signals:
    // Fired on a successful FETCH. adifData is the raw ADIF blob
    // exactly as qrz.com returned it; the caller is expected to hand
    // it to AdifImporter for dedup + local-log merge.
    // count carries the COUNT= value from the response so callers
    // can show "received N records" while the importer is still
    // working.
    void downloaded (QByteArray adifData, int count);

    void downloadFailed (QString error);

private:
    QString apiKey_;
    QNetworkAccessManager * nam_ {nullptr};
};

} // namespace wkjtx

#endif // WKJTX_QRZ_DOWNLOADER_HPP

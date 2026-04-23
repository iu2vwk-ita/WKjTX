#include "QrzDownloader.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace wkjtx {

namespace {

// Parse the `&`-delimited key=value response into a dictionary.
QString extractField (QString const & body, QString const & key)
{
    // The ADIF= value can legitimately contain `&` inside macro bodies,
    // so a blind QUrlQuery split would truncate. Use positional find:
    // find `<key>=` anchored to start-of-body or `&` separator, and
    // copy to the next `\n` (qrz.com terminates each kv with a
    // newline when the value is multi-line like ADIF=).
    QString const needle = key + QLatin1Char ('=');
    int idx = body.indexOf (needle);
    while (idx > 0 && body.at (idx - 1) != QLatin1Char ('&')
                    && body.at (idx - 1) != QLatin1Char ('\n')) {
        idx = body.indexOf (needle, idx + needle.size ());
    }
    if (idx < 0) return {};
    int const start = idx + needle.size ();
    // For ADIF blobs the server writes newlines within the body; the
    // terminator is the final `\n` before the next `KEY=` or EOF. For
    // scalar fields the value ends at `&` or `\n`. We take the simpler
    // interpretation: stop at `\n` — the ADIF value from qrz.com comes
    // on a single (very long) line when requested as TYPE:ADIF.
    int end = body.indexOf (QLatin1Char ('\n'), start);
    if (end < 0) end = body.size ();
    return body.mid (start, end - start);
}

} // namespace

QrzDownloader::QrzDownloader (QObject * parent)
    : QObject {parent}
    , nam_    {new QNetworkAccessManager {this}}
{}

QrzDownloader::~QrzDownloader () = default;

void QrzDownloader::setApiKey (QString const & key) { apiKey_ = key.trimmed (); }

void QrzDownloader::fetch (QDateTime const & since)
{
    if (apiKey_.isEmpty ()) {
        emit downloadFailed (QStringLiteral ("qrz.com: API key non impostata"));
        return;
    }

    QUrl const url {QStringLiteral ("https://logbook.qrz.com/api")};
    QNetworkRequest req {url};
    req.setHeader (QNetworkRequest::ContentTypeHeader,
                   QStringLiteral ("application/x-www-form-urlencoded"));
    req.setHeader (QNetworkRequest::UserAgentHeader,
                   QStringLiteral ("WKjTX/1.2 (+https://github.com/iu2vwk-ita/WKjTX)"));

    // OPTION is a comma-separated flag list. TYPE:ADIF forces ADIF
    // output. MODSINCE:<utc> limits to records modified after a
    // UTC timestamp; skipped on first sync to pull the whole log.
    QString option = QStringLiteral ("TYPE:ADIF");
    if (since.isValid ()) {
        option += QStringLiteral (",MODSINCE:")
                + since.toUTC ().toString (QStringLiteral ("yyyy-MM-dd HH:mm:ss"));
    }

    QUrlQuery body;
    body.addQueryItem (QStringLiteral ("KEY"),    apiKey_);
    body.addQueryItem (QStringLiteral ("ACTION"), QStringLiteral ("FETCH"));
    body.addQueryItem (QStringLiteral ("OPTION"), option);
    QByteArray const data = body.toString (QUrl::FullyEncoded).toUtf8 ();

    QNetworkReply * reply = nam_->post (req, data);
    connect (reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater ();
        if (reply->error () != QNetworkReply::NoError) {
            emit downloadFailed (
                QStringLiteral ("rete: %1").arg (reply->errorString ()));
            return;
        }
        QString const resp = QString::fromUtf8 (reply->readAll ());
        QString const result = extractField (resp, QStringLiteral ("RESULT"));
        if (result.compare (QLatin1String ("OK"), Qt::CaseInsensitive) != 0) {
            QString reason = extractField (resp, QStringLiteral ("REASON"));
            if (reason.isEmpty ()) reason = resp.left (200);
            emit downloadFailed (QStringLiteral ("qrz.com: %1").arg (reason));
            return;
        }
        // COUNT is a scalar — total records in the blob.
        int const count = extractField (resp, QStringLiteral ("COUNT")).toInt ();
        QByteArray const adif = extractField (resp, QStringLiteral ("ADIF"))
                                    .toUtf8 ();
        emit downloaded (adif, count);
    });
}

} // namespace wkjtx

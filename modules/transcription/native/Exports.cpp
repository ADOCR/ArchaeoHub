#include "Exports.h"
#include <QDir>
#include <QSaveFile>
#include <QDateTime>
#include <stdexcept>

namespace {
void save(const QString &path, const QByteArray &data) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit())
        throw std::runtime_error(("No se pudo guardar: " + path).toStdString());
}
void u16(QByteArray &out, quint16 v) { out.append(char(v)); out.append(char(v >> 8)); }
void u32(QByteArray &out, quint32 v) { u16(out, v & 0xffff); u16(out, v >> 16); }
quint32 crc32(const QByteArray &data) {
    quint32 crc = 0xffffffff;
    for (unsigned char byte : data) {
        crc ^= byte;
        for (int i=0; i<8; ++i) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1)));
    }
    return ~crc;
}
QByteArray zip(const QList<QPair<QByteArray,QByteArray>> &entries) {
    QByteArray out, central;
    for (const auto &entry : entries) {
        const auto &name=entry.first; const auto &data=entry.second;
        quint32 offset=out.size(), crc=crc32(data);
        u32(out,0x04034b50); u16(out,20); u16(out,0x800); u16(out,0); u16(out,0); u16(out,0x21);
        u32(out,crc); u32(out,data.size()); u32(out,data.size()); u16(out,name.size()); u16(out,0); out+=name; out+=data;
        u32(central,0x02014b50); u16(central,20); u16(central,20); u16(central,0x800); u16(central,0); u16(central,0); u16(central,0x21);
        u32(central,crc); u32(central,data.size()); u32(central,data.size()); u16(central,name.size());
        u16(central,0); u16(central,0); u16(central,0); u16(central,0); u32(central,0); u32(central,offset); central+=name;
    }
    quint32 start=out.size(); out+=central;
    u32(out,0x06054b50); u16(out,0); u16(out,0); u16(out,entries.size()); u16(out,entries.size());
    u32(out,central.size()); u32(out,start); u16(out,0);
    return out;
}
void docx(const QString &path, const QString &title, const QString &text) {
    QString document="<?xml version=\"1.0\" encoding=\"UTF-8\"?><w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\"><w:body>";
    auto paragraph=[&](const QString &line, bool bold) {
        document+="<w:p><w:r>";
        if(bold) document+="<w:rPr><w:b/><w:sz w:val=\"32\"/></w:rPr>";
        document+="<w:t xml:space=\"preserve\">"+line.toHtmlEscaped()+"</w:t></w:r></w:p>";
    };
    paragraph(title,true); paragraph("Gamboa Glass · Transcripción local",false);
    for(const auto &line:text.split('\n')) paragraph(line,false);
    document+="<w:sectPr><w:pgSz w:w=\"11906\" w:h=\"16838\"/><w:pgMar w:top=\"1134\" w:right=\"1134\" w:bottom=\"1134\" w:left=\"1134\"/></w:sectPr></w:body></w:document>";
    save(path,zip({
        {"[Content_Types].xml","<?xml version=\"1.0\"?><Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\"><Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/><Default Extension=\"xml\" ContentType=\"application/xml\"/><Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/></Types>"},
        {"_rels/.rels","<?xml version=\"1.0\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"word/document.xml\"/></Relationships>"},
        {"word/document.xml",document.toUtf8()}}));
}
}
QString Exports::stamp(qint64 ms, bool comma) {
    ms=qMax(qint64(0),ms);
    return QString("%1:%2:%3%4%5").arg(ms/3600000,2,10,QChar('0')).arg((ms/60000)%60,2,10,QChar('0'))
        .arg((ms/1000)%60,2,10,QChar('0')).arg(comma ? ',' : '.').arg(ms%1000,3,10,QChar('0'));
}
QString Exports::plain(const QVariantList &segments) {
    QStringList lines; for(const auto &v:segments) lines<<v.toMap().value("text").toString().trimmed();
    return lines.join('\n');
}
QVariantMap Exports::write(const QString &folder,const QString &name,const QVariantList &segments) {
    if(!QDir().mkpath(folder)) throw std::runtime_error("No se pudo crear la carpeta de salida.");
    QString srt,timed,vtt="WEBVTT\n\n"; int i=0;
    for(const auto &v:segments) {
        auto s=v.toMap(); auto text=s["text"].toString().trimmed(); auto a=s["start"].toLongLong(),b=s["end"].toLongLong();
        srt+=QString::number(++i)+"\n"+stamp(a)+" --> "+stamp(b)+"\n"+text+"\n\n";
        vtt+=stamp(a,false)+" --> "+stamp(b,false)+"\n"+text+"\n\n";
        timed+="["+stamp(a)+" → "+stamp(b)+"] "+text+"\n";
    }
    auto p=[&](const QString &suffix){ return QDir(folder).filePath(name+suffix); };
    save(p(".txt"),(plain(segments)+"\n").toUtf8()); save(p("_tiempos.txt"),timed.toUtf8());
    save(p(".srt"),srt.toUtf8()); save(p(".vtt"),vtt.toUtf8()); docx(p(".docx"),name,plain(segments));
    return {{"folder",folder},{"txt",p(".txt")},{"srt",p(".srt")},{"docx",p(".docx")},{"vtt",p(".vtt")}};
}
void Exports::writeMinutes(const QString &folder,const QString &text) {
    save(QDir(folder).filePath("Minuta.txt"),text.toUtf8()); docx(QDir(folder).filePath("Minuta.docx"),"Minuta",text);
}

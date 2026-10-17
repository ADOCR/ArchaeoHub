#include <QtTest>
#include <QTemporaryDir>
#include <QDir>
#include <QImage>
#include <QCryptographicHash>
#include <QJsonArray>
#include "modules/catalog/CatalogController.h"
#include "modules/coordinates/CoordinateController.h"
#include "modules/transcription/TranscriptionController.h"
#include "modules/documents/DocumentController.h"
#include "modules/documents/DocumentLibraryController.h"
#include "shared/LocalModels.h"
#include "modules/inventory/InventoryController.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <memory>
#include "ui/Hub.h"
#include <QDesktopServices>
#include <QScopeGuard>
using namespace workbench;
class FolderProbe final:public QObject { Q_OBJECT
public: QUrl opened;
public slots: void capture(const QUrl &url){opened=url;}
};
namespace {
QByteArray contents(const QString &path){QFile file(path);if(!file.open(QIODevice::ReadOnly))return {};return file.readAll();}
}
class NativeControllerContracts final:public QObject {
    Q_OBJECT
private slots:
    void coordinateCatalogGroupsAndPagingPreserveSelection(){
        QTemporaryDir root;Store store(root.path());TaskManager tasks(store);
        coordinates::CoordinateController controller(tasks,root.path());
        const auto source=controller.sourceCode(),target=controller.targetCode();
        controller.search("","utm-south");QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),10000);
        QVERIFY2(!controller.error(),qPrintable(controller.message()));QCOMPARE(controller.searchTotal(),60);QCOMPARE(controller.searchResults().size(),60);
        controller.search("","all");QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),10000);
        QVERIFY(!controller.error());QVERIFY(controller.searchTotal()>100);const auto first=controller.searchResults();
        controller.search("","all",100);QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),10000);
        QVERIFY(!controller.error());QCOMPARE(controller.searchOffset(),100);QCOMPARE(controller.searchResults().size(),100);
        for(const auto &a:first)for(const auto &b:controller.searchResults())QVERIFY(a.toMap()["code"]!=b.toMap()["code"]);
        const auto previous=controller.searchResults();controller.search("","invalid-group");QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),10000);
        QVERIFY(controller.error());QCOMPARE(controller.technicalCode(),QString("input.crs_group"));QCOMPARE(controller.searchResults(),previous);
        QCOMPARE(controller.sourceCode(),source);QCOMPARE(controller.targetCode(),target);QCOMPARE(tasks.activeCount(),0);
    }
    void remoteModelMetadataFailsClosed(){
        QVERIFY(isRemoteModel(QJsonObject{{"remote_host","https://example.invalid"}},"local-looking-alias"));
        QVERIFY(isRemoteModel(QJsonObject{{"remote_model","upstream"}},"local-looking-alias"));
        QVERIFY(isRemoteModel(QJsonObject{{"remote_host",true}},"invalid-metadata"));
        QVERIFY(isRemoteModel({},"provider:cloud"));
        QVERIFY(!isRemoteModel(QJsonObject{{"remote_host",""},{"remote_model",QJsonValue()}},"fixture:local"));
    }
    void documentResearchFailureKeepsConvertiblePlan(){
        QTemporaryDir root;Store store(root.path());TaskManager tasks(store);QTcpServer server;QVERIFY(server.listen(QHostAddress::LocalHost,0));
        connect(&server,&QTcpServer::newConnection,this,[&]{while(server.hasPendingConnections()){
            auto socket=server.nextPendingConnection();connect(socket,&QTcpSocket::readyRead,socket,[socket]{
                socket->readAll();const QByteArray body="{\"models\":[]}";socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: "+QByteArray::number(body.size())+"\r\n\r\n"+body);socket->disconnectFromHost();
            });connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
        }});
        const auto input=QDir(root.path()).filePath("Contexto.md");QFile file(input);QVERIFY(file.open(QIODevice::WriteOnly));file.write("Muestreo y contexto arqueologico.");file.close();
        const auto output=QDir(root.path()).filePath("Destino");QVERIFY(QDir().mkpath(output));
        documents::DocumentLibraryController controller(tasks,root.path(),nullptr,server.serverPort());QSignalSpy completed(&controller,&documents::DocumentLibraryController::completed);
        controller.addInputs({QUrl::fromLocalFile(input)});controller.analyze();QTRY_COMPARE_WITH_TIMEOUT(completed.size(),1,15000);QVERIFY(completed.last().first().toBool());QVERIFY(controller.hasPlan());
        controller.enrich("missing:fixture");QTRY_COMPARE_WITH_TIMEOUT(completed.size(),2,15000);QVERIFY(!completed.last().first().toBool());QVERIFY(controller.hasPlan());QVERIFY(!controller.hasAnalysis());QCOMPARE(tasks.activeCount(),0);
        controller.exportLibrary(QUrl::fromLocalFile(output),false);QTRY_COMPARE_WITH_TIMEOUT(completed.size(),3,15000);QVERIFY2(completed.last().first().toBool(),qPrintable(controller.message()));QVERIFY(QFileInfo::exists(QDir(controller.resultFolder()).filePath("Índice.md")));
        documents::DocumentLibraryController conversion(tasks,root.path(),nullptr,server.serverPort(),true);QSignalSpy converted(&conversion,&documents::DocumentLibraryController::completed);
        conversion.addInputs({QUrl::fromLocalFile(input)});conversion.analyze();QTRY_COMPARE_WITH_TIMEOUT(converted.size(),1,15000);QVERIFY(!converted.last().first().toBool());QVERIFY(!conversion.hasPlan());QVERIFY(controller.hasPlan());QVERIFY(QFileInfo::exists(QDir(controller.resultFolder()).filePath("Índice.md")));
    }
    void documentLibraryExportsWithoutOllamaOrOverwritingNotes(){
        QTemporaryDir root;QVERIFY(root.isValid());const auto source=QDir(root.path()).filePath("Entrada");const auto output=QDir(root.path()).filePath("Bóveda ñ");QVERIFY(QDir().mkpath(source));QVERIFY(QDir().mkpath(output));
        for(const auto &name:{"Ceramica.md","Estratigrafia.md"}){QFile file(QDir(source).filePath(name));QVERIFY(file.open(QIODevice::WriteOnly));file.write("# Campo\nCeramica estratigrafia compartida.\n");}
        const auto original=contents(QDir(source).filePath("Ceramica.md"));QFile keep(QDir(output).filePath("Mi nota.md"));QVERIFY(keep.open(QIODevice::WriteOnly));keep.write("USER NOTE");keep.close();
        const auto profile=QDir(root.path()).filePath("Perfil");Store store(profile);TaskManager tasks(store);documents::DocumentLibraryController controller(tasks,profile);QSignalSpy completed(&controller,&documents::DocumentLibraryController::completed);
        controller.addInputs({QUrl::fromLocalFile(source)});controller.analyze();QTRY_COMPARE_WITH_TIMEOUT(completed.size(),1,15000);QVERIFY2(completed.last().first().toBool(),qPrintable(controller.message()));QCOMPARE(controller.readyCount(),2);QVERIFY(controller.hasPlan());QVERIFY(!controller.topics().isEmpty());
        controller.exportLibrary(QUrl::fromLocalFile(output),true);QTRY_COMPARE_WITH_TIMEOUT(completed.size(),2,15000);QVERIFY2(completed.last().first().toBool(),qPrintable(controller.message()));const auto folder=controller.resultFolder();QVERIFY(QFileInfo::exists(QDir(folder).filePath("Índice.md")));QVERIFY(QFileInfo::exists(QDir(folder).filePath("archaeohub-library.json")));
        QCOMPARE(contents(QDir(output).filePath("Mi nota.md")),QByteArray("USER NOTE"));QCOMPARE(contents(QDir(source).filePath("Ceramica.md")),original);QCOMPARE(tasks.activeCount(),0);
        controller.analyze();controller.cancel();QTRY_COMPARE_WITH_TIMEOUT(completed.size(),3,15000);QVERIFY(!completed.last().first().toBool());QCOMPARE(controller.resultFolder(),folder);QVERIFY(QFileInfo::exists(QDir(folder).filePath("Índice.md")));QCOMPARE(tasks.activeCount(),0);
    }
    void catalogWritesAndErrorsPreserveOriginals(){
        QVERIFY2(QFileInfo::exists(QDir(QCoreApplication::applicationDirPath()).filePath("modules/catalog/engine/CatalogWorker.exe")),"Freeze the catalog engine before native module acceptance tests");
        QTemporaryDir root;QVERIFY(root.isValid());const auto source=QDir(root.path()).filePath("Originales ñ"),output=QDir(root.path()).filePath("Trabajo");
        QVERIFY(QDir().mkpath(source));QVERIFY(QDir().mkpath(output));
        const auto imagePath=QDir(source).filePath("Prueba.png");QImage image(64,48,QImage::Format_RGB32);image.fill(QColor(12,100,82));QVERIFY(image.save(imagePath));
        const auto original=contents(imagePath);Store store(QDir(root.path()).filePath("Hub"));TaskManager tasks(store);catalog::CatalogController controller(tasks,QDir(root.path()).filePath("Hub"));
        controller.openCatalog(source,output,"QA-ONLY","2");QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),15000);QVERIFY2(!controller.error(),qPrintable(controller.message()));QVERIFY(controller.opened());QCOMPARE(controller.items().size(),1);
        const auto id=controller.items().first().toMap()["id"].toString();controller.select(id);QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),5000);
        QVERIFY(!controller.selection()["preview"].toMap()["url"].toString().isEmpty());
        QVariantMap form{{"operation","2"},{"unitType","Pozo"},{"unit","7"},{"level","1"},{"subunitType","Cuadro"},{"subunit","A"},{"view","Prueba"},{"notes","Descripción ñ"}};
        controller.propose(form);QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),5000);QVERIFY(!controller.proposedName().isEmpty());
        const auto predicted=controller.proposedName();
        controller.save(form,{});QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),5000);QVERIFY2(!controller.error(),qPrintable(controller.message()));QCOMPARE(controller.state()["registered"].toInt(),1);
        const auto copy=QDir(output).filePath("Entrega_Museo/Fotos/"+predicted);QCOMPARE(contents(copy),original);
        controller.closeCatalog();QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),5000);
        controller.loadCatalog(QUrl::fromLocalFile(QDir(output).filePath("Entrega_Museo/Catalogo_fotografico_pozos.xlsx")),source);
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),10000);QVERIFY2(!controller.error(),qPrintable(controller.message()));QVERIFY(controller.opened());QCOMPARE(controller.state()["site"].toString(),QString("QA-ONLY"));QVERIFY(!controller.awaitingInput());QCOMPARE(controller.selection()["id"].toString(),id);
        form["unit"]="8";controller.correct(form);QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),5000);QVERIFY(!controller.error());QCOMPARE(controller.selection()["record"].toMap()["Unidad"].toString(),QString("8"));QCOMPARE(controller.selection()["id"].toString(),id);
        const auto report=QDir(output).filePath("GPS.xlsx");controller.exportGps(QUrl::fromLocalFile(report));QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),5000);QVERIFY2(!controller.error(),qPrintable(controller.message()));const auto gps=contents(report);QVERIFY(!gps.isEmpty());
        controller.exportGps(QUrl::fromLocalFile(report));QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),5000);QVERIFY(controller.error());QCOMPARE(contents(report),gps);
        controller.withdraw(false);QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),5000);QVERIFY(controller.error());QCOMPARE(controller.state()["registered"].toInt(),1);
        controller.withdraw(true);QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),5000);QVERIFY(!controller.error());QCOMPARE(controller.state()["registered"].toInt(),0);QCOMPARE(controller.state()["skipped"].toInt(),1);
        controller.skip();QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),5000);QVERIFY(!controller.error());QCOMPARE(controller.state()["skipped"].toInt(),0);
        catalog::CatalogController other(tasks,QDir(root.path()).filePath("OtherHub"));other.openCatalog(source,output,"QA-ONLY","2");QTRY_VERIFY_WITH_TIMEOUT(!other.busy(),10000);QVERIFY(other.error());QVERIFY(!other.opened());QVERIFY(controller.opened());
        controller.closeCatalog();QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),5000);QVERIFY(!controller.opened());QCOMPARE(contents(imagePath),original);QCOMPARE(tasks.activeCount(),0);
    }
    void coordinateResultsExportAndCancellation(){
        QTemporaryDir root;Store store(root.path());TaskManager tasks(store);coordinates::CoordinateController controller(tasks,root.path());
        QJsonObject request{{"op","convert"},{"source","EPSG:5365"},{"target","EPSG:5367"},{"rows",QJsonArray{QJsonObject{{"id","CONTROL"},{"x","-84"},{"y","10"}}}}};
        QVERIFY(controller.request(request));QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),15000);QVERIFY2(!controller.error(),qPrintable(controller.message()));QCOMPARE(controller.rows()->rowCount(),1);
        const auto result=controller.lastResult();const auto row=result["rows"].toArray().first().toObject();QCOMPARE(row["status"].toString(),QString("ok"));QVERIFY(std::abs(row["resultX"].toDouble()-500000)<1e-6);
        const auto path=QDir(root.path()).filePath("Resultado.csv");controller.exportFile(QUrl::fromLocalFile(path),"csv");QVERIFY(!controller.error());const auto bytes=contents(path);QVERIFY(bytes.contains("EPSG:5367"));
        controller.exportFile(QUrl::fromLocalFile(path),"csv");QVERIFY(controller.error());QCOMPARE(contents(path),bytes);
        const auto xlsx=QDir(root.path()).filePath("Resultado ñ.xlsx");controller.exportFile(QUrl::fromLocalFile(xlsx),"xlsx");QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),15000);QVERIFY2(!controller.error(),qPrintable(controller.message()));const auto excel=contents(xlsx);QVERIFY(excel.startsWith("PK"));
        controller.importSpreadsheet(QUrl::fromLocalFile(xlsx));QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),15000);QVERIFY(!controller.error());QVERIFY(controller.sheets().contains("Coordenadas"));
        controller.selectSheet("Coordenadas");QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),15000);QVERIFY(!controller.error());QCOMPARE(controller.inputSheet(),QString("Coordenadas"));QVERIFY(controller.tableText().contains("CONTROL"));QCOMPARE(controller.lastResult(),result);
        controller.exportFile(QUrl::fromLocalFile(xlsx),"xlsx");QVERIFY(controller.error());QCOMPARE(contents(xlsx),excel);
        controller.describe("EPSG:99999999","EPSG:4326");QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),10000);QVERIFY(controller.error());QCOMPARE(controller.lastResult(),result);QCOMPARE(controller.rows()->rowCount(),1);
        QJsonArray many;for(int i=0;i<10000;++i)many.append(QJsonObject{{"id",QString::number(i)},{"x","-84"},{"y","10"}});request["rows"]=many;
        QVERIFY(controller.request(request));controller.cancel();QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),10000);QCOMPARE(controller.technicalCode(),QString("job.cancelled"));QCOMPARE(controller.lastResult(),result);QCOMPARE(tasks.activeCount(),0);
    }
    void transcriptionIsBundledAndReportsErrorsWithoutExternalApp(){
        QTemporaryDir root;Store store(root.path());TaskManager tasks(store);qputenv("GAMBOA_CONFIG_DIR",root.path().toUtf8());
        transcription::TranscriptionController controller(tasks,root.path(),{});QVERIFY(!controller.ready());controller.initialize();
        QTRY_VERIFY_WITH_TIMEOUT(controller.ready(),15000);QCOMPARE(controller.state()["models"].toList().size(),4);QVERIFY(!controller.busy());
        QSignalSpy finished(&controller,&transcription::TranscriptionController::batchFinished);controller.start();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(),1,10000);QVERIFY(!finished.first().first().toBool());QVERIFY(!controller.message().isEmpty());QCOMPARE(tasks.activeCount(),0);
        qunsetenv("GAMBOA_CONFIG_DIR");
    }
    void folderOpeningUsesDirectoryServiceWithoutTreatingItAsFile(){
        QTemporaryDir root;Store store(root.path());TaskManager tasks(store);Volumes volumes(store);Registry registry(store,volumes,root.path());Logger logger(root.path());
        Hub hub(store,registry,volumes,tasks,logger,"QA","0.4.0");FolderProbe probe;
        QDesktopServices::setUrlHandler("file",&probe,"capture");auto cleanup=qScopeGuard([]{QDesktopServices::unsetUrlHandler("file");});
        const auto url=QUrl::fromLocalFile(root.path());hub.openFolder(url);QCOMPARE(probe.opened,url);QVERIFY(!hub.noticeError());
        probe.opened={};hub.openFolder(QUrl::fromLocalFile(QDir(root.path()).filePath("missing")));QVERIFY(probe.opened.isEmpty());QVERIFY(hub.noticeError());
    }
    void inventoryNativeBridgeUsesUtf8AndOwnDatabase(){
        QTemporaryDir root;Store store(root.path());TaskManager tasks(store);inventory::InventoryController controller(tasks,root.path());
        QSignalSpy done(&controller,&inventory::InventoryController::completed);const auto db=QDir(root.path()).filePath("Inventario ñ.ahinv");
        controller.create(QUrl::fromLocalFile(db),{{"project","Proyecto cerámica"},{"site","QA"}});
        QTRY_COMPARE_WITH_TIMEOUT(done.size(),1,15000);QVERIFY2(done.last().first().toBool(),qPrintable(controller.message()));QVERIFY(controller.opened());
        controller.add({{"operacion","2"},{"pozo","007"},{"q_cer",14},{"cantidad_original",14},{"comentarios_laboratorio","Cerámica sintética"}});
        QTRY_COMPARE_WITH_TIMEOUT(done.size(),2,15000);QVERIFY2(done.last().first().toBool(),qPrintable(controller.message()));QCOMPARE(controller.rows().size(),1);
        QCOMPARE(controller.rows().first().toMap().value("comentarios_laboratorio").toString(),QString("Cerámica sintética"));
        const auto output=QDir(root.path()).filePath("Análisis.xlsx");controller.exportFile(QUrl::fromLocalFile(output));
        QTRY_COMPARE_WITH_TIMEOUT(done.size(),3,15000);QVERIFY2(done.last().first().toBool(),qPrintable(controller.message()));QVERIFY(QFileInfo::exists(output));
        const auto before=contents(output);controller.exportFile(QUrl::fromLocalFile(output));QTRY_COMPARE_WITH_TIMEOUT(done.size(),4,15000);QVERIFY(!done.last().first().toBool());QCOMPARE(contents(output),before);QCOMPARE(tasks.activeCount(),0);
    }
    void documentSummaryPreservesSourceResultAndExport(){
        int truncate=0,generated=0;bool remote=false;
        QTemporaryDir root;Store store(root.path());TaskManager tasks(store);QTcpServer server;QVERIFY(server.listen(QHostAddress::LocalHost,0));
        connect(&server,&QTcpServer::newConnection,this,[&]{while(server.hasPendingConnections()){
            auto socket=server.nextPendingConnection();auto input=std::make_shared<QByteArray>();
            connect(socket,&QTcpSocket::readyRead,socket,[socket,input,&truncate,&generated,&remote]{*input+=socket->readAll();const auto end=input->indexOf("\r\n\r\n");if(end<0)return;
                const auto header=input->left(end);qsizetype length=0;for(const auto &line:header.split('\n'))if(line.toLower().startsWith("content-length:"))length=line.mid(15).trimmed().toLongLong();if(input->size()<end+4+length)return;
                const bool get=header.startsWith("GET"),show=header.contains("/api/show ");const bool cut=!get&&!show&&truncate>0;if(cut)--truncate;if(!get&&!show)++generated;
                const auto object=get?QJsonObject{{"models",QJsonArray{QJsonObject{{"name","fixture:local"}},QJsonObject{{"name","fixture:cloud"},{"remote_host","https://example.invalid"}}}}}:show?(remote?QJsonObject{{"remote_model","hidden-remote"},{"remote_host","https://example.invalid"}}:QJsonObject{}):QJsonObject{{"response","Resumen del pozo 7: 14 fragmentos. [D1:texto]"},{"done_reason",cut?"length":"stop"}};
                const auto body=QJsonDocument(object).toJson(QJsonDocument::Compact);socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: "+QByteArray::number(body.size())+"\r\n\r\n"+body);socket->disconnectFromHost();
            });connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
        }});
        const auto inputPath=QDir(root.path()).filePath("Notas.md");QFile file(inputPath);QVERIFY(file.open(QIODevice::WriteOnly));const QByteArray original="# Campo\nPozo 7: 14 fragmentos.\n";file.write(original);file.close();
        documents::DocumentController controller(tasks,root.path(),nullptr,server.serverPort());controller.initialize();QTRY_VERIFY_WITH_TIMEOUT(!controller.models().isEmpty(),10000);
        controller.addFiles({QUrl::fromLocalFile(inputPath)});QCOMPARE(controller.files().size(),1);QSignalSpy finished(&controller,&documents::DocumentController::completed);controller.generate();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(),1,15000);QVERIFY2(finished.first().first().toBool(),qPrintable(controller.message()));QVERIFY(controller.summary().contains("14"));QVERIFY(controller.summary().contains("Notas.md"));const auto summary=controller.summary();QCOMPARE(contents(inputPath),original);
        const auto output=QDir(root.path()).filePath("Resumen.md");controller.exportSummary(QUrl::fromLocalFile(output));const auto exported=contents(output);QVERIFY(!exported.isEmpty());controller.exportSummary(QUrl::fromLocalFile(output));QCOMPARE(contents(output),exported);
        truncate=1;controller.generate();QTRY_COMPARE_WITH_TIMEOUT(finished.size(),2,15000);QVERIFY(finished.last().first().toBool());QCOMPARE(controller.summary(),summary);QCOMPARE(truncate,0);
        truncate=2;controller.generate();QTRY_COMPARE_WITH_TIMEOUT(finished.size(),3,15000);QVERIFY(!finished.last().first().toBool());QCOMPARE(controller.summary(),summary);QVERIFY(controller.message().contains("límite"));
        controller.generate();controller.cancel();QVERIFY(!controller.busy());QCOMPARE(controller.summary(),summary);QCOMPARE(contents(inputPath),original);QCOMPARE(tasks.activeCount(),0);
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(),5000);QTest::qWait(100);
        QCOMPARE(controller.models(),QStringList{"fixture:local"});
        remote=true;const auto requests=generated;const auto completedCount=finished.size();controller.generate();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(),completedCount+1,5000);QVERIFY(!finished.last().first().toBool());
        QCOMPARE(generated,requests);QCOMPARE(controller.summary(),summary);QCOMPARE(contents(inputPath),original);QCOMPARE(tasks.activeCount(),0);
    }
};
QTEST_MAIN(NativeControllerContracts)
#include "native_controller_tests.moc"
